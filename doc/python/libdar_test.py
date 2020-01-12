import libdar
import os, sys

# mandatory first action is to initialize libdar
# by calling libdar.get_version()

u = libdar.get_version()
print("using libdar version {}.{}.{}".format(u[0],u[1],u[2]))

# libdar.get_version() can be called at will

# now defining a very minimalist class to let libdar
# interact with the user directly. One could make use
# of graphical popup window here if a Graphical User Interface
# was used:
class myui(libdar.user_interaction):
    def __init__(self):
        libdar.user_interaction.__init__(self)
        # it is mandatory to initialize the parent
        # class: libdar.user_interaction

    def inherited_message(self, msg):
        print("LIBDAR MESSAGE:{0}".format(msg))
        # the "LIBDAR MESSAGE" is pure demonstration
        # to see when output comes through this
        # user_interaction child class

    def inherited_pause(self, msg):
        while True:
            res = input("LIBDAR QUESTION:{0} y/n ".format(msg))
            if res == "y":
                return True
            else:
                if res == "n":
                    return False
                else:
                    print("answer 'y' or 'n'")

    def inherited_get_string(self, msg, echo):
        return input(msg)
        # we should take care about the boolean value echo
        # and not show what user type when echo is False

    def inherited_get_secu_string(self, msg, echo):
        return input(msg)
        # we should take care about the boolean value echo
        # and not show what user type when echo is False

# exceptions from libdar (libdar::Egeneric, Erange, ...) in
# C++ side are all translated to libdar.darexc class introduced
# in the python binding. This class has the __str__() method to
# get the message string about the cause of the exception
# it get displayed naturally when you don't catch them from
# a python shell

# here is an example on how to handle libdar.darexc exceptions:

try:
    x = libdar.deci("not an integer")
except libdar.darexc as obj:
    print("libdar exception: {}".format(obj.__str__()))

# here follows some helper routines as illustration
# on how to manage some libdar data structures

# display libdar.statistics (not all fields are shown, see help(libdar.statistics)
def display_stats(stats):
    print("---- stats result ---")
    print("treated entries  = {}".format(stats.get_treated_str()))
    print("hard link entries = {}".format(stats.get_hard_links_str()))
    print("%skipped entries = {}".format(stats.get_skipped_str()))
    print("inode only entries  = {}".format(stats.get_inode_only_str()))
    print("ignored entries  = {}".format(stats.get_ignored_str()))
    print("too old entries  = {}".format(stats.get_tooold_str()))
    print("errored entries  = {}".format(stats.get_errored_str()))
    print("deleted entries  = {}".format(stats.get_deleted_str()))
    print("EA      entries  = {}".format(stats.get_ea_treated_str()))
    print("FSA     entries  = {}".format(stats.get_fsa_treated_str()))
    print("hard link entries = {}".format(stats.get_hard_links_str()))
    print("wasted byte amount = {}".format(stats.get_byte_amount_str()))
    print("total enries = {}".format(i2str(stats.total())))
    print("---------------------")

# displaying libdar.entree_stats there is a predefined method
# as used here that will rely on a libdar.user_interaction to
# display the contents. You may also access the different fields
# by hand. see help(libdar.entree_stats) for details
def display_entree_stats(stats, ui):
    print("--- archive content stats ---")
    stats.listing(ui)
    print("-----------------------------")

# for this later structure (libdar.entree_stats) you will probably
# want to play with libdar.infinit. I has quite all operation you
# can expect on integer (*,/,+,-,*=,+=,-=,/=,^=, >>=, <<=, %=,<,>,==,!=,...)
# the libdar.deci() class can be buil from a libdar.infinit or
# from a python string and provides two methods: human() and computer()
# that return a python string representing the number in base ten
# and computer() that returns a libdar.infinint

def f0():
    x = libdar.infinint("122")
    #
    dy = libdar.deci("28")
    y = dt.computer()
    # which is equivalent to y = libdar.infinint("28")

    z = x / y # integer division
    print("the integer division of {} by {} gives {}".format(libdar.deci(x).human(),
                                                             dy.human(),
                                                             libdar.deci(z).human()))

    # there is also the libdar.euclide(x, y) method that returns
    # the integer division and rest as a couple of their numerator and divisor
    # passed in argument:

    res = libdar.euclide(x, y)
    print("{} / {} = {} with a remain of {}".format(libdar.deci(x).human(),
                                                    dy.human(),
                                                    libdar.deci(res[0]).human(),
                                                    libdar.deci(res[1]).human()))

# libdar.infinint to string:
def i2str(infinint):
    return libdar.deci(infinint).human()

# this is a example of routine that given an open libdar.archive
# will provide its listing content. This call is recursive but
# free to you to recurse or not upon user event (expanding a directory
# in th current display for example).
# Note that the method libdar.archive.get_children_in_table returns
# a list of object of type libdar.list_entry which has a long
# list of methods to provide a very much detailed information
# for a given entry in the archive. For more about it,
# see help(libdar.list_entry)
def list_dir(archive, chem = "", indent = ""):
    content = archive.get_children_in_table(chem, True)
    # contents is a list of libdar.list_entry objects

    for ent in content:
        ligne = indent
        if ent.is_eod():
            continue

        if ent.is_hard_linked():
            ligne += "*"
        else:
            ligne += " "

        if ent.is_dir():
            ligne += "d"
        else:
            if ent.is_file():
                ligne += "f"
            else:
                 if ent.is_symlink():
                     ligne += "l"
                 else:
                     if ent.is_char_device():
                         ligne += "c"
                     else:
                         if ent.is_block_device():
                             ligne += "b"
                         else:
                             if ent.is_unix_socket():
                                 ligne += "s"
                             else:
                                 if ent.is_named_pipe():
                                     ligne += "p"
                                 else:
                                     if ent.is_door_inode():
                                         ligne += "D"
                                     else:
                                         if ent.is_removed_entry():
                                             ligne += "Removed entry which was of of type {}".format(ent.get_removed_type())
                                         else:
                                             ligne += "WHAT THIS????"
                                             continue

        ligne += ent.get_perm() + " " + ent.get_name() + " "
        ligne += ent.get_uid(True) + "/" + ent.get_gid(True) + " "
        ligne += ent.get_last_modif()
        print(ligne)

        # now peparing the recursion when we meet a directory:
        if ent.is_dir():
            if chem != "":
                nchem = (libdar.path(chem) + ent.get_name()).display()
            else:
                nchem = ent.get_name()
            nindent = indent + "   "
            list_dir(archive, nchem, nindent)

# in the following we will provide several functions that
# either create, read, test, diff or extract an archive
# all will rely on the following global variables:

ui = myui()
sauv_path = libdar.path(".")
arch_name = "arch1"
ext = "dar"

# let's create an archive. the class
# libdar.archive_options_create has a default constructor
# that set the options to their default values, the clear()
# method can also be used to reset thm to default.
# then a bunch of method are provided to modify each of them
# according to your needs. See help(libdar.archive_options_create)
# and the API reference documentation for their nature and meaning

# the libdar.path can be set from a python string but has some
# method to pop, add a sub-directory easily. the libdar.path.display()
# method provides the representative string of the path
def f1():
    opt = libdar.archive_options_create()
    opt.set_info_details(True)
    opt.set_display_treated(True, False)
    opt.set_display_finished(True)
    fsroot = libdar.path("/etc")
    print("creating the archive")
    libdar.archive(ui,
                   fsroot,
                   sauv_path,
                   arch_name,
                   ext,
                   opt)

# a the difference of C++ here several constructors and method
# like op_diff, op_test, etc. where a libdar::statistics *progresive_report
# field is present, the python binding has two equivalent methods, one
# without this field, and a second with a plain libdar.statistics field.
# this later object can be read from another thread while a libdar operation runs
# with it given as argument. This let the user see the progression of the
# operation (mainly counters on the number of inode
# treated, skipped, errored, etc.). More detail in the API reference guide

# by the way you will see user interaction in action as we
# tend to overwrite the archive created in f1(), assuming you
# run f1(), f2().... in order for the demo

def f2():
    opt = libdar.archive_options_create()
    opt.set_info_details(True)
    opt.set_display_treated(False, False)
    opt.set_display_finished(True)
    fsroot = libdar.path("/etc")
    stats = libdar.statistics()

    print("creating the archive")
    libdar.archive(ui,
                   fsroot,
                   sauv_path,
                   arch_name,
                   ext,
                   opt,
                   stats)
    display_stats(stats)


# here we read an existing archive. Then first
# phase is to create a libdar.archive object
# the second is to act upon it. Several actions
# can be done in sequence on an existing object
# open that way (extracting several time, diff, test,
# an do on
def f3():
    opt = libdar.archive_options_read()
    opt.set_info_details(True)
    arch1 = libdar.archive(ui,
                           sauv_path,
                           arch_name,
                           ext,
                           opt);
    list_dir(arch1)
    stats = arch1.get_stats()
    display_entree_stats(stats, ui)

# below we will play with mask. Most operation have to
# operations to filter the file they will apply on. Then
# first "set_selection()" applies to filenames only
# the second 'set_subtree()" applies the whole path instead
# What type of libdar.mask() you setup for these is
# completely free. Pay attention that when a directory
# is excluded (by mean of set_subtree()) all its content
# and recursively all is subdirectories are skipped.

# the list of class inheriting from libdar.mask() are:
# - bool_mask(bool) either always true or always false
# - libdar.simple_mask(string) the provided string is read as a glob
# expression, which is the syntax most shell use like bash
# - libdar.regex_mask(string) the argument is read as a
# regular expression
# - simple_path_mask(string) matches if the string to
# compare to is a subdir of the string provided to the
# constructor, or if this string is a subdr of the string
# to compare to. This is mostly adapted to select a
# given directory for an operation, as all the path leading
# to it must match and all subdirectory in that directory
# must also match.
# - same_path_mask(string) matches only the given
# argument. This is intended for directory pruning
#
# most mask have in fact a second argument in their
# constructor (a boolean) that define whether the mask
# is case sensitive (True) or not (False)

# - not_mask(mask) gives the negation of the mask
# provided in argument
# - et_mask() + add_mask(mask) + add_mask(mask) +...
# makes a logical AND between the added masks
# - ou_mask() + add_mask(mask) + add_mask(mask) +...
# makes a logical OR between the added masks
# why this French "et" and "ou" words? because at that
# time they were added this code was internal to dar
# and I frequently use French words to designate my
# own datastructure to differentiate with English symbols
# brought from outside. This code has not change since then
# so is the reason.
# of course you can add_mask() a ou_mask(), a not_mask()
# or yet a et_mask() recursively at will and make arbitrarily complex
# masks mixing them with simple_mask(), regular_mask(), and so on.

def f4():
    opt = libdar.archive_options_read()
    arch1 = libdar.archive(ui,
                           sauv_path,
                           arch_name,
                           ext,
                           opt);

    opt = libdar.archive_options_test()

    # defining which file to test bases on filename (set_selection)
    mask_file1 = libdar.simple_mask("*.*", True)
    mask_file2 = libdar.regular_mask(".*\.pub$", True)
    mask_filenames = libdar.ou_mask() # doing the logical OR between what we will add to it:
    mask_filenames.add_mask(mask_file1)
    mask_filenames.add_mask(mask_file2)
    opt.set_selection(mask_filenames)

    # reducing the testing in subdirectories
    tree1 = libdar.simple_path_mask("/etc/ssh", False)
    tree2 = libdar.simple_path_mask("/etc/grub.d", False)
    tree = libdar.et_mask() # doing the loical AND betwen what we will add to it:
    tree.add_mask(libdar.not_mask(tree1))
    tree.add_mask(libdar.not_mask(tree2))
    opt.set_subtree(tree)

    opt.set_info_details(True)
    opt.set_display_skipped(True)

    arch1.op_test(opt)

# nothing much more different as previously
# except that we compare the archive with
# the filesystem (op_diff) while we tested the
# archive coherence previously (op_test)
def f5():
    opt = libdar.archive_options_read()
    arch1 = libdar.archive(ui,
                           sauv_path,
                           arch_name,
                           ext,
                           opt);

    tree1 = libdar.simple_path_mask("/etc/ssh", False)
    tree2 = libdar.simple_path_mask("/etc/grub.d", False)
    tree = libdar.ou_mask()
    tree.add_mask(tree1)
    tree.add_mask(tree2)

    opt = libdar.archive_options_diff()
    opt.set_subtree(tree)
    opt.set_info_details(True)
    opt.set_display_treated(True, False)
    opt.set_ea_mask(libdar.bool_mask(True))
    opt.set_furtive_read_mode(False)

    arch1.op_diff(libdar.path("/etc"),
                  opt)

    rest = libdar.path("./Restore")
    try:
        os.rmdir(rest.display())
    except:
        pass
    os.mkdir(rest.display())

    opt = libdar.archive_options_extract()

    # the overwriting policy can receive
    # objects from many different crit_action_* classes
    # - crit_constant_action() used here does always the same
    # action on Data and EA+FSA when a conflict arise that
    # would lead to overwriting
    # - testing(criterium) the action depends on the evaluation
    # of the provided criterium (see below)
    # - crit_chain() + add(crit_action) performs the different
    # crit_actions added in sequence the first one that provides
    # an action for Data and/or EA+FSA is retained. If no action
    # is left undefined the following crit_action of the chain are
    # not evaluated
    #
    # for the testing crit_action inherited class, we need to provide
    # a criterium object. Here too there is a set of inherited classes
    # that come to help:
    # - crit_in_place_is_inode
    # - crit_in_place_is_dir
    # - crit_in_place_is_file
    # - ...
    # - crit_not (to take the negation of the given criterium)
    # - crit_or + add_crit() + add_crit() ... makes the logical OR
    # - crit_and + add_crit() + add_crit()... for the logical AND
    # - crit_invert for the in_place/to_be_added inversion
    # Read the manual page about overwriting policy for details
    # but in substance the criterum return true of false for each
    # file in conflict and the object if class testing that uses
    # this criterium applies the action given as "go_true" or the
    # action given as "go_false" in regard of the provided result

    over_policy = libdar.crit_constant_action(libdar.over_action_data.data_preserve,
                                           libdar.over_action_ea.EA_preserve)
    opt.set_overwriting_rules(over_policy)

    # fsa_scope is a std::set in C++ side and translates to a
    # python set on python side. Use the add() method to add
    # values to the set:
    fsa_scope = set()
    fsa_scope.add(libdar.fsa_family.fsaf_hfs_plus)
    fsa_scope.add(libdar.fsa_family.fsaf_linux_extX)
    opt.set_fsa_scope(fsa_scope)

    stats = libdar.statistics()
    arch1.op_extract(rest, opt, stats)
    display_stats(stats)


# last, all operation that interact with filesystem use by default
# a libdar.entrepot_local object (provided by the archive_options_*
# object, this makes the archive written and read from local filesystem.
# However you can replace this entrepot by an object of class
# libdar.libcurl_entrepot to read or write an archive over the network
# directly from libdar by mean of FTP of SFTP protocols. Follows an
# illustration of this possibility:
def f6():
    opt = libdar.archive_options_read()
    passwd ="joe@the.shmoe"
    secu_pass = libdar.secu_string(passwd, len(passwd))
    entrepot = libdar.entrepot_libcurl(ui,
                                       libdar.mycurl_protocol.proto_ftp,
                                       "anonymous",
                                       secu_pass,
                                       "ftp.dm3c.org",
                                       "",
                                       False,
                                       "",
                                       "",
                                       "",
                                       5)
    print(entrepot.get_url())
    opt.set_entrepot(entrepot)
    opt.set_info_details(True)

    arch2 = libdar.archive(ui,
                           libdar.path("/dar.linux.free.fr/"),
                           "exemple",
                           "dar",
                           opt)

    opt2 = libdar.archive_options_test()
    opt2.set_display_treated(True, False)
    arch2.op_test(opt2)

# other classes of interest:
# - libdar.database for the dar_manager featues
# - libdar.libdar_xform for the dar_xform features
# - libdar.libdar_slave for the dar_slave features

# they are all three accessible from python and follow
# very closely the C++ syntax and usage
