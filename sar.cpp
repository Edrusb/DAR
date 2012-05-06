/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: sar.cpp,v 1.28 2002/04/25 17:59:36 denis Rel $
//
/*********************************************************************/

#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include "sar.hpp"
#include "deci.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"

static bool sar_extract_num(string filename, string base_name, string ext, infinint & ret);
static bool sar_get_higher_number_in_dir(path dir ,string base_name, string ext, infinint & ret);

sar::sar(const string & base_name, const string & extension, int options, const path & dir) : generic_file(gf_read_only), archive_dir(dir)
{
    set_options(options);
    
    base = base_name;
    ext = extension;
    initial = true;
    open_file_init();
    open_file(1);
}

sar::sar(const string & base_name, const string & extension, const infinint & file_size, const infinint & first_file_size, int options, const path & dir) : generic_file(gf_write_only), archive_dir(dir)
{
    if(file_size < header::size() + 1)
	throw Erange("sar::sar", "file size too small");

    initial = true;
    set_options(options);
    base = base_name;
    ext = extension;
    size = file_size;
    first_size = first_file_size;

    open_file_init();
    open_file(1);
}

sar::~sar()
{
    close_file();
}

bool sar::skip(infinint pos)
{
    infinint byte_in_first_file = first_size - first_file_offset;
    infinint byte_per_file = size - header::size();
    infinint dest_file, offset;

	///////////////////////////
	// termination of the file to go and its offset to seek
	//
    if(pos < byte_in_first_file)
    {
	dest_file = 1;
	offset = pos + first_file_offset;
    }
    else
    {
	dest_file = ((pos - byte_in_first_file) / byte_per_file) + 2; 
	    // "+2" because file number starts to 1 and first file is to be count
	offset = ((pos - byte_in_first_file) % byte_per_file) + header::size();
    }

	///////////////////////////
	// checking wheather the required position is acceptable
	//
    if(of_last_file_known && dest_file > of_last_file_num)
    {
	    // going to EOF
	open_file(of_last_file_num);
	of_fd->skip_to_eof();
	file_offset = of_fd->get_position();
	return false;
    }
    else
    {
	try
	{
	    open_file(dest_file);
	    set_offset(offset);
	    file_offset = offset;
	    return true;
	}
	catch(Erange & e)
	{
	    return false;
	}
    }
}

bool sar::skip_to_eof()
{
    bool ret;

    open_last_file();
    ret = of_fd->skip_to_eof();
    file_offset = of_fd->get_position();
    set_offset(file_offset);

    return ret;
}

bool sar::skip_forward(unsigned int x)
{
    infinint number = of_current;
    infinint offset = file_offset + x;

    while((number == 1 ? offset >= first_size : offset >= size) 
	  && (!of_last_file_known || number <= of_last_file_num))
    {
	offset -= number == 1 ? first_size : size;
	offset += header::size();
	number++;
    }
    
    if(number == 1 ? offset < first_size : offset < size)
    {
	open_file(number);
	file_offset = offset;
	set_offset(file_offset);
	return true;
    }
    else
	return false;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: sar.cpp,v 1.28 2002/04/25 17:59:36 denis Rel $";
    dummy_call(id);
}

bool sar::skip_backward(unsigned int x)
{
    infinint number = of_current;
    infinint offset = file_offset;
    infinint offset_neg = x;

    while(number > 1 && offset_neg + header::size() > offset)
    {
	offset_neg -= offset - header::size() + 1;
	number--;
	if(number > 1)
	    offset = size - 1;
	else
	    offset = first_size - 1;
    }

    if((number > 1 ? offset_neg + header::size() : offset_neg + first_file_offset) <= offset)
    {
	open_file(number);
	file_offset = offset - offset_neg;
	set_offset(file_offset);
	return true;
    }
    else
    {   // seek to beginning of file
	open_file(1);
	set_offset(first_file_offset);
	return false;
    }
}
          
bool sar::skip_relative(signed int x)
{
    if(x > 0)
	return skip_forward(x);
    
    if(x < 0)
	return skip_backward(-x);

    return true; // when x == 0
}		

infinint sar::get_position()
{	    
    if(of_current > 1)
	return first_size - first_file_offset + (of_current-2)*(size-header::size()) + file_offset - header::size();
    else
	return file_offset - first_file_offset;
}

int sar::inherited_read(char *a, size_t sz)
{
    size_t lu = 0;
    bool loop = true;
    
    while(lu < sz && loop)
    {
	int tmp = of_fd->read(a+lu, sz-lu);
	if(tmp < 0)
	    throw Erange("sar::inherited_read", strerror(errno));
	if(tmp == 0)
	    if(of_flag == FLAG_TERMINAL)
		loop = false;
	    else
		open_file(of_current + 1);
	else
	{
	    lu += tmp;
	    file_offset += tmp;
	}
    }

    return lu;
}

int sar::inherited_write(char *a, size_t sz)
{
    infinint to_write = sz;
    infinint max_at_once;
    infinint tmp_wrote;
    int tmp;
    unsigned int micro_wrote;

    while(to_write > 0)
    {
	max_at_once = of_current == 1 ? first_size - file_offset : size - file_offset;
	tmp_wrote = max_at_once > to_write ? to_write : max_at_once;
	if(tmp_wrote > 0)
	{
	    micro_wrote = 0;
	    tmp_wrote.unstack(micro_wrote);
	    tmp = of_fd->write(a, micro_wrote);
	}
	else
	{
	    open_file(of_current + 1);
	    continue;
	}
	if(tmp < 0) // not very usefull, as generic_file::write never returns negative values ... to change
	    throw Erange("sar::inherited_write", strerror(errno));
	if(tmp == 0)
	{
	    user_interaction_pause("can't write any byte to file, filesystem is full ? Please check");
	    continue;
	}
	to_write -= tmp;
	file_offset += tmp;
	a += tmp;
    }
	
    return sz;
}

void sar::close_file()
{
    if(of_fd != NULL)
    {
	delete of_fd;
	of_fd = NULL;
    }
}

void sar::open_readonly(char *fic, const infinint &num)
{
    header h;

    while(of_fd == NULL)
    {
	    // trying to open the file
	    //
	int fd = open(fic, O_RDONLY);
	if(fd < 0)
	    if(errno == ENOENT)
	    {
		user_interaction_pause(string(fic) + " is required for furthur operation, please provide the file");
		continue; 
	    }
	    else
		throw Erange("sar::open_readonly", string("error openning ") + fic + " : " + strerror(errno));
	else
	    of_fd = new fichier(fd);
	
	    // trying to read the header
	    //
	try 
	{
	    h.read(*of_fd);
	}
	catch(Egeneric & e)
	{
	    close_file();
	    user_interaction_pause(string(fic) + string("has a bad or corrupted header, please provide the correct file"));
	    continue; 
	}
		
	    // checking agains the magic number
	    //
	if(h.magic != SAUV_MAGIC_NUMBER)
	{
	    close_file();
	    user_interaction_pause(string(fic) + " is not a valid file (wrong magic number), please provide the good file");
	    continue; 
	}
		
	    // checking the ownership to the set of file
	    //
	if(num == 1 && first_file_offset == 0)
	{
	    label_copy(of_internal_name, h.internal_name);
	    try
	    {
		first_size = of_fd->get_size();
		if(h.extension == EXTENSION_SIZE)
		    size = h.size_ext;
		else
		    size = first_size;
		first_file_offset = of_fd->get_position();
	    }
	    catch(Erange & e)
	    {
		user_interaction_pause(string("error openning ") + fic + " : " + e.get_message());
	    }
	}
	else
	    if(! header_label_is_equal(of_internal_name, h.internal_name))
	    {
		close_file();
		user_interaction_pause(string(fic) + " is a file from another set of backup file, please provide the correct file");
		continue;
	    }

	    // checking the flag
	    //
	switch(h.flag)
	{
	case FLAG_TERMINAL :
	    of_last_file_known = true;
	    of_last_file_num = num;
	    of_last_file_size = of_fd->get_size();
	    break;
	case FLAG_NON_TERMINAL :
	    break;
	default :
	    close_file();
	    user_interaction_pause(string(fic) + " has an unknown flag (neither terminal nor non_terminal file)");
	    continue;
	}
	of_flag = h.flag;
    }
}

void sar::open_writeonly(char *fic, const infinint &num)
{
    struct stat buf;
    header h;
    int open_flag = O_WRONLY;
    int open_mode = 0666; // umask will unset some bits while calling open
    int fd;

	// check if that the file exists
    if(stat(fic, &buf) < 0)
	if(errno != ENOENT) // other error than 'file does not exist' occured
	    throw Erange("sar::open_writeonly stat()", strerror(errno));
	else
	    open_flag |= O_CREAT;
    else // file exists
    {
	int fd_tmp = open(fic, O_RDONLY);

	if(fd_tmp >= 0)
	{
	    try
	    {
		try
		{
		    h.read(fd_tmp);
		}
		catch(Erange & e)
		{
		    label_copy(h.internal_name, of_internal_name);
		    h.internal_name[0] = ~h.internal_name[0];
			// this way we are shure that the file is not considered as part of the SAR
		}
		if(h.internal_name != of_internal_name)
		{
		    open_flag |= O_TRUNC;
		    if(opt_dont_erase)
			throw Erange("sar::open_writeonly", "file exists, and DONT_ERASE option is set");
		    if(opt_warn_overwrite)
			user_interaction_pause(string(fic) + " is about to be overwritten");
		}
	    }
	    catch(Egeneric & e)
	    {		 
		close(fd_tmp);
		throw;
	    }
	    close(fd_tmp);
	}
	else // file exists but could not be openned
	{
	    if(opt_dont_erase)
		throw Erange("sar::open_writeonly", "file exists, and DONT_ERASE option is set");
	    if(opt_warn_overwrite)
		user_interaction_pause(string(fic) + " is about to be overwritten");
	}
    }	 
   
    fd = open(fic, open_flag, open_mode);
    of_flag = FLAG_NON_TERMINAL;
    if(fd < 0)
	throw Erange("sar::open_writeonly open()", strerror(errno));    
    else
	of_fd = new fichier(fd);
    h = make_write_header(num, FLAG_TERMINAL);
    h.write(*of_fd);
    if(num == 1)
    {
	first_file_offset = of_fd->get_position();
	if(first_file_offset == 0)
	    throw SRC_BUG;
    }
}

void sar::open_file_init()
{
    of_max_seen = 0;
    of_last_file_known = false;
    of_fd = NULL;
    first_file_offset = 0; // means that the sizes have to be determined from file or wrote to file
}

void sar::open_file(infinint num)
{
    if(of_fd == NULL || of_current != num) 
    {
	char *fic = tools_str2charptr((archive_dir + path(sar_make_filename(base, num, ext))).display());
	
	try
	{
	    switch(get_mode())
	    {
	    case gf_read_only :
		close_file();
		open_readonly(fic, num);
		break;
	    case gf_write_only :
		if(of_fd != NULL && (num > of_current || of_max_seen > of_current))
		{    // actually openned file is not the last file of the set, thus changing the flag before closing
		    header h = make_write_header(of_current, FLAG_NON_TERMINAL);

		    of_fd->skip(0);
		    h.write(*of_fd);
		}
		close_file();
		if(opt_pause)
		{
		    if(!initial)
		    {
			deci conv = of_current;
			user_interaction_pause(string("finished writing to file ") + conv.human() + " ready to continue ? ");
		    }
		    else
			initial = false;
		}
		open_writeonly(fic, num);
		break;
	    default :
		close_file();
		throw SRC_BUG;
	    }
	    of_current = num;
	    if(of_max_seen < of_current)
		of_max_seen = of_current;
	    file_offset = of_current == 1 ? first_file_offset : header::size();
	}
	catch(Egeneric & e)
	{
	    delete fic;
	    throw;
	}
	delete fic;
    }
}

void sar::set_options(int opt)
{
    opt_warn_overwrite = (opt & SAR_OPT_WARN_OVERWRITE) != 0;
    opt_dont_erase = (opt & SAR_OPT_DONT_ERASE) != 0;
    opt_pause = (opt & SAR_OPT_PAUSE) != 0;
}

void sar::set_offset(infinint offset)
{
    if(of_fd == NULL)
	throw Erange("sar::set_offset", "file not open");
    else
	of_fd->skip(offset);
}

void sar::open_last_file()
{
    infinint num; 

    if(of_last_file_known)
	open_file(of_last_file_num);
    else
    {
	while(of_flag != FLAG_TERMINAL)
	{
	    if(sar_get_higher_number_in_dir(archive_dir, base, ext, num))
	    {
		open_file(num);
		if(of_flag != FLAG_TERMINAL)
		{
		    close_file();
		    user_interaction_pause(string("the last file of the set is not present in ") + archive_dir.display() + " , please provide it");
		}
	    }
	    else
	    {
		close_file();
		user_interaction_pause(string("no backup file is present in ") + archive_dir.display() + " , please provide the last file of the set");
	    }
	}
    }
}

header sar::make_write_header(const infinint & num, char flag)
{
    header h;

    label_copy(h.internal_name, of_internal_name);
    h.magic = SAUV_MAGIC_NUMBER;
    h.flag = flag;
    h.extension = EXTENSION_NO;
    if(num == 1)
    {
	if(first_file_offset == 0)
	{
	    header_generate_internal_filename(of_internal_name);
	    label_copy(h.internal_name, of_internal_name);
	}
	if(size != first_size)
	{
	    h.extension = EXTENSION_SIZE;
	    h.size_ext = size;
	}
    }

    return h;
}

string sar_make_filename(string base_name, infinint num, string ext)
{
    deci conv = num;

    return base_name + '.' + conv.human() + '.' + ext;
}

static bool sar_extract_num(string filename, string base_name, string ext, infinint & ret)
{
    try
    {
	if(filename.size() <= base_name.size() + ext.size() + 2) // 2 for two dots beside number
	    return false;
	
	if(filename.find(base_name) != 0) // checking that base_name is present at the beginning
	    return false;
	
	if(filename.rfind(ext) != filename.size() - ext.size()) // checking that extension is at the end
	    return false;

	deci conv = string(filename.begin()+base_name.size()+1, filename.begin() + (filename.size() - ext.size()-1));
	ret = conv.computer();
	return true;
    }
    catch(Egeneric &e)
    {
	return false;
    }
}

static bool sar_get_higher_number_in_dir(path dir, string base_name, string ext, infinint & ret)
{
    infinint cur;
    bool somme = false;
    struct dirent *entry;
    char *folder = tools_str2charptr(dir.display());
    DIR *ptr = opendir(folder);

    try
    {
	if(ptr == NULL)
	    throw Erange("sar_get_higher_number_in_dir", strerror(errno));
	
	ret = 0; 
	somme = false;
	while((entry = readdir(ptr)) != NULL)
	    if(sar_extract_num(entry->d_name, base_name, ext, cur))
	    {
		if(cur > ret)
		    ret = cur;
		somme = true;
	    }
    }
    catch(Egeneric & e)
    {
	delete folder;
	if(ptr != NULL)
	    closedir(ptr);
	throw;
    }

    delete folder;
    if(ptr != NULL)
	closedir(ptr);
    return somme;
}

trivial_sar::trivial_sar(int fd) : fichier(fd)
{
    header tete;

    tete.magic = SAUV_MAGIC_NUMBER;
    header_generate_internal_filename(tete.internal_name);
    tete.flag = FLAG_TERMINAL;
    tete.extension = EXTENSION_NO;
    tete.write(*this);
}

