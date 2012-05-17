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
// to contact dar's author : http://dar.linux.free.fr/email.html
// to contact Chris Martin, author of dar-help, contact dar's author
//        which will forward, or give you Chris's email upon his agreement
//
/*********************************************************************/
// $Id: dar-help.c,v 1.11 2009/12/18 11:48:47 edrusb Rel $
//
/*********************************************************************/

void no_compiler_warning(char *x)
{
    char rcsid[] =
"$Id: dar-help.c,v 1.11 2009/12/18 11:48:47 edrusb Rel $";
    no_compiler_warning(rcsid);
}
/*
 * Using libxml2 to format the DAR help text.
 *
 * Influenced by gjobsread.c, the libxml2 example.
 *
 */

#include "../my_config.h"

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if STDC_HEADERS
#include <ctype.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#if HAVE_LIBXML_TREE_H
#include <libxml/tree.h>
#endif

#if HAVE_LIBXML_PARSER_H
#include <libxml/parser.h>
#endif

int verbose = 0;

typedef enum { FALSE, TRUE } boolean;

#define NO_INDENT 0
/* The default values which can be overridden by the DTD
 *
 * These values are the column in which the flag, desc or continuation
 * line is to start.
 *
 * The continuation line for a desc starts in column
 *      (DESC_INDENT+CONT_INDENT-1).
 *
 *  The continuation line for a hdr (the only other one) starts in
 *  column CONT_INDENT.
 */
#define FLAG_INDENT    9
#define DESC_INDENT   17
#define CONT_INDENT    3
#define WORD_WRAP_COL 79

static unsigned flag_indent, desc_indent, cont_indent, word_wrap_col;

#define INIT_LGTH 1024
char *line = NULL;
    /* the allocated length of line at present */
unsigned line_alloc = 0;
    /* the next unused position in line (line[line_col]) */
unsigned line_col;

static xmlChar *
xrealloc(xmlChar * buf, int *alloc, int request)
{
    buf = realloc(buf, request);
    if (buf == NULL) {
	fprintf(stderr, "xrealloc: can't extend from %d to %d bytes.\n",
		*alloc, request);
	exit(1);
    }
    *alloc = request;
    return buf;
}

static xmlChar *
xmalloc(int request)
{
    xmlChar *buf = malloc(request);
    if (buf == NULL) {
	fprintf(stderr, "xmalloc: can't allocate %d bytes.\n", request);
	exit(1);
    }
    return buf;
}

static xmlChar *
xstrdup(xmlChar * s)
{
    xmlChar *str = strdup(s);
    if (str == NULL) {
	fprintf(stderr, "can't strdup str (length %d).\n", strlen(s));
	exit(1);
    }
    return str;
}

static void
xfree(xmlChar * s)
{
    if (s)
	free(s);
}

static void
init_line(void)
{
    if (line)
	*line = '\0';

    line_col = 0;
}

/*
 * From the glibc man page, in the standard C locale
 */
#define WHITE_SPACE " \f\n\r\t\v"

static void
append_to_line_with_indent(const xmlChar * s, const char c, unsigned indent)
{
    static char *piece = NULL;
    static unsigned piece_alloc = 0;
    int i, indent_needed;
    unsigned len;

    if (s == NULL)
	return;

    len = strlen(s);

	/* Allow for a possible character suffix ('<' or '[') */
    if (len + 2 > piece_alloc)
	piece = xrealloc(piece, &piece_alloc, len + 16);
    strcpy(piece, s);

	/* Remove leading spaces */
    i = (int)strspn(piece, WHITE_SPACE);
    memmove(piece, &piece[i], strlen(piece) - i + 1);

    if (c) {
	if (piece_alloc < strlen(piece) + 1 + 1)
	    piece = xrealloc(piece, &piece_alloc, strlen(piece) + 16);
	memmove(&piece[1], piece, strlen(piece) + 1);
	piece[0] = c;
    }

	/* Trim trailing space. */
    while (isspace(piece[strlen(piece) - 1]))
	piece[strlen(piece) - 1] = '\0';

    if (verbose)
	printf("append: trailing spaces removed:\n  '%s'\n", piece);

	/*
	 * indent is in columns, line_col is a char array index:
	 *  indent=8 <=> line_col=7
	 */
    if (line_col < indent - 1)    /* indentation needed */
	indent_needed = (indent - 1) - line_col;
    else
	    /* no more indentation, just a space separator */
	if (line_col > 0 &&
	    !(isspace(line[line_col - 1]) || (line[line_col - 1] == '<')))
	    indent_needed = 1;
	else
	    indent_needed = 0;

    if (line_alloc < line_col + indent_needed + strlen(piece) + 1) {
	line =
	    xrealloc(line, &line_alloc,
		     line_col + indent_needed + strlen(piece) + 16);
    }

    for (i = 0; i < indent_needed; i++)
	line[line_col++] = ' ';

    strcpy(&line[line_col], piece);
    line_col += strlen(piece);

}

static void
append_to_line_asis(const xmlChar * s)
{
    if (line_alloc < line_col + strlen(s) + 1) {
	line = xrealloc(line, &line_alloc, line_col + strlen(s) + 16);
    }
    strcpy(&line[line_col], s);
    line_col += strlen(s);
}

/*
 * Remove leading, trailing and embedded white space.
 *
 * Leading and trailing white space (defined by isspace(c)) is removed
 * completely, the first and last character of the resulting string
 * will not be white space -- the string may, however, only contain
 * the zero byte terminator.
 *
 * The string WHITE_SPACE contains the list of characters isspace()
 * considers to be white space.
 *
 * Embedded white space other than the space character is, notionally,
 * replaced by the space character and then runs of spaces are reduced
 * to a single space.
 *
 * last_was_space  isspace(*p)  action
 *   true           true      character ignored
 *   true           false     last_was_space set false, character copied
 *   false          true      last_was_space set true, space character copied
 *   false          false     character copied
 *
 */
static void
remove_white_space(xmlChar * content)
{
    size_t initial_sp;
    xmlChar *p, *q, *dup;
    boolean last_was_space;

	/* Trim trailing space. */
    while (isspace(content[strlen(content) - 1]))
	content[strlen(content) - 1] = '\0';

    if (strlen(content) == 0)
	return;

	/* Trim leading space */
    if ((initial_sp = strspn(content, WHITE_SPACE)))
	memmove(content, content + initial_sp, strlen(content) - initial_sp + 1);

	/*
	 * Now go through the content removing embedded white space, replacing
	 * two or more spaces in sequence by a single space.
	 */
    dup = p = xstrdup(content);
    q = content;
    last_was_space = FALSE;
    for (; *p; p++) {
	if (last_was_space) {
	    if (isspace(*p))
		continue;
	    else {
		last_was_space = FALSE;
		*q++ = *p;
	    }
	} else {
	    if (isspace(*p)) {
		last_was_space = TRUE;
		*q++ = ' ';
	    } else
		*q++ = *p;
	}
    }
    *q = '\0';
    xfree(dup);
}

static void
show_line(char *line)
{
    char *p = line;

    fputs("  '", stdout);
    while (*p) {
	if (isprint(*p))
	    putc(*p, stdout);
	else if (*p == '\n')
	    fputs("\\n", stdout);
	else if (*p == '\t')
	    fputs("\\t", stdout);
	else if (*p == ' ')
	    putc(' ', stdout);
	else
	    fprintf(stdout, "\\%03o", *p);
	p++;
    }
    fputs("'\n", stdout);
}

static void
lineout(char *line)
{
    char *p = line;

    fputs("\tdialog.printf(gettext(\"", stdout);
    while (*p) {
	if (isprint(*p))
	    putc(*p, stdout);
	else if (*p == '\n')
	    fputs("\\n", stdout);
	else if (*p == '\t')
	    fputs("\\t", stdout);
	else if (*p == ' ')
	    putc(' ', stdout);
	else
	    fprintf(stdout, "\\%03o", *p);
	p++;
    }
    if (*(p - 1) != '\n')
	fputs("\\n", stdout);
    fputs("\"));\n", stdout);
}

/*
 * Assume tab stops are set at 8 column intervals, that is, a tab
 * character appearing in the 8 columns preceding the tab stop will
 * move the cursor to the tab stop column.
 *
 * a picture...
 *
 * column          1         2         3
 *        123456789012345678901234567890...
 *                T       T      T...
 *        YYYYYYYNYYYYYYYNYYYYYYYN...
 * i      01234567890
 *
 *
 * If there are no non-space characters (that is, c == ' ') between
 * the Y furthest away from T and T, replace all these space
 * characters by a tab character ('\t').
 *
 * Continue until the end of the string.
 *
 */
static char *
spaces_to_tabs(char *line)
{
    char *p = line;
    char *sqzd = xstrdup(line);
    char *q;
    unsigned i, j;
    unsigned pending = 0;         /* just like unexpand... */

    if (verbose > 1) {
	printf("                1         2         3\n");
	printf("      0123456789012345678901234567890\n");
	printf("line: '%s'\n", line);
    }
    q = sqzd;
    for (i = 1; *p; i++, p++) {
	if (*p == '\n') {           /* a newline is a new line */
	    pending = 0;
	    i = 0;
	    *q++ = *p;
	    continue;
	}
	if ((i % 8) == 1) {
	    if (pending > 1)
		*q++ = '\t';
	    else if (pending)
		*q++ = ' ';
	    pending = 0;
	    *q = '\0';                /* for debugging printf */
	}
	if (*p == ' ')
	    pending++;
	else {
	    if (pending)
		for (j = 0; j < pending; j++)
		    *q++ = ' ';
	    *q++ = *p;
	    pending = 0;
	    *q = '\0';                /* for debugging printf */
	}
    }
    *q = '\0';
    if (verbose > 1) {
	printf("                1         2         3\n");
	printf("      0123456789012345678901234567890\n");
	printf("sqzd: '%s'\n", line);
    }
    return sqzd;

}

static void
output_string(char *s)
{
    char line[128], *sqzd;
    char *str;
    char *tok;
    size_t start, follow_on, indent;
    boolean eol, end_line;

    str = xstrdup(s);
    if (verbose) {
	printf("output_string: str \n");
	show_line(str);
    }

	/*
	 * The only lines which may be broken with the second piece
	 * "following on" are <hdr> lines -- indented from the left margin
	 * by cont_indent -- and <desc> lines -- indented from the
	 * left margin by desc_indent + cont_indent.
	 *
	 * Since both desc_indent and cont_indent represent columns,
	 * the continuation indent for a <desc> is
	 * desc_indent+cont_indent-1.
	 */
    follow_on = cont_indent;
	/* it is assumed that hdr lines don't begin with space */
    if (*str == ' ')
	follow_on += desc_indent - 1;

    if (verbose)
	printf("output_string: follow_on %u, s:\n  '%s'\n", follow_on, s);

	/*
	 * Find the first space after column desc_indent and start word
	 * wrapping from there.
	 */
    start = strcspn(str + desc_indent, " ");
    strncpy(line, str, desc_indent + start);
    line[desc_indent + start] = '\0';
    tok = strtok(str + desc_indent + start, " ");
    while (tok) {
	int lgth_reqd = strlen(tok);
	if (line[strlen(line) - 1] != ' ')
	    lgth_reqd++;
	eol = (strlen(tok) == 1) && (*tok == '\n');
	if (eol)
	    indent = desc_indent;
	else
	    indent = follow_on;
	end_line = eol || (strlen(line) + lgth_reqd > word_wrap_col);
	if (end_line) {
	    if (verbose)
		printf("output_string (end_line): line:\n  '%s'\n", line);
	    sqzd = spaces_to_tabs(line);
	    lineout(sqzd);
	    xfree(sqzd);
		/* so it starts in in column "indent" */
	    memset(line, ' ', indent - 1);
	    line[indent - 1] = '\0';
	}
	if (!eol) {
		/* There may be a space there now, it doesn't matter */
	    if (line[strlen(line) - 1] != ' ')
		strcat(line, " ");
	    strcat(line, tok);
	}
	tok = strtok(NULL, " ");
    }

    if (verbose)
	printf("output_string (exit): line:\n  '%s'\n", line);
    sqzd = spaces_to_tabs(line);
	/*
	 * Is there anything in line to output?
	 *
	 * ...could a line of white space appear here?
	 */
    if (strlen(sqzd)) {
	lineout(sqzd);
	xfree(sqzd);
    }

    xfree(str);
}

/*
 * Extract the content from the current element if it is a text
 * element otherwise from the child text element of the current element.
 */
static xmlChar *
get_text(xmlNodePtr cur)
{
    xmlChar *content, *trimmed;

    if (!xmlStrcmp(cur->name, (const xmlChar *)"text"))
	content = cur->content;
    else if (cur->children &&
	     !xmlStrcmp(cur->children->name, (const xmlChar *)"text"))
	content = cur->children->content;
    else {
	xmlNodePtr n;
	if (cur->children == NULL)
	    printf("get_text: no children\n");
	else {
	    n = cur->children;
	    while (n) {
		printf("get_text: %s\n", n->name);
		n = n->next;
	    }
	}
	return NULL;
    }
    trimmed = xstrdup(content);
    remove_white_space(trimmed);
    return trimmed;
}

/*
typedef xmlBuffer *xmlBufferPtr; struct xmlNode
struct xmlNode {
    void           *_private;/ * application data * /
    xmlElementType   type;/ * type number, must be second ! * /
    const xmlChar   *name;      / * the name of the node, or the entity * /
    struct _xmlNode *children;/ * parent->childs link * /
    struct _xmlNode *last;/ * last child link * /
    struct _xmlNode *parent;/ * child->parent link * /
    struct _xmlNode *next;/ * next sibling link  * /
    struct _xmlNode *prev;/ * previous sibling link  * /
    struct _xmlDoc  *doc;/ * the containing document * /
     / * End of common part * /
    xmlNs           *ns;        / * pointer to the associated namespace * /
    xmlChar         *content;   / * the content * /
    struct _xmlAttr *properties;/ * properties list * /
    xmlNs           *nsDef;     / * namespace definitions on this node * /
};

*/

static void
vspace(char *name, char *s)
{
    char *tail;
    int number = strtol(s, &tail, 0);
    int i;
    char *newlines;

    if (*tail != '\0') {
	fprintf(stderr, "vspace: error converting '%s', tail is '%s'.\n", s,
		tail);
	return;
    }

    if (number <= 0) {
	fprintf(stderr, "vspace: won't insert %d blank lines...\n", number);
	return;
    }

    if (verbose)
	fprintf(stderr, "vspace: inserting %d blank %s before %s.\n",
		number, number == 1 ? "line" : "lines", name);

    newlines = xmalloc(number + 1);
    for (i = 0; i < number; i++)
	newlines[i] = '\n';
    newlines[number] = '\0';
    lineout(newlines);
    xfree(newlines);
}

static void
set_var(char *name, unsigned *var, char *value)
{
    char *tail;
    int number = strtol(value, &tail, 0);

    if (*tail != '\0') {
	fprintf(stderr, "set_var (%s): error converting '%s', tail is '%s'.\n",
		name, value, tail);
	return;
    }

    if (number <= 0) {
	fprintf(stderr, "set_var: won't set %s to %d...\n", name, number);
	return;
    }

    if (verbose == 0)
	fprintf(stderr, "set_var: setting %s to %d.\n", name, number);

    *var = number;
}

static void
arg_handler(const xmlChar * name, xmlChar * content, int indent)
{
    xmlChar *str;

    if (!xmlStrcmp(name, (const xmlChar *)"repl")) {
	str = xmalloc(strlen(content) + 3);
	strcpy(str, "<");
	strcat(str, content);
	strcat(str, ">");
	append_to_line_with_indent(str, '\0', indent);
	xfree(str);
    } else if (!xmlStrcmp(name, (const xmlChar *)"p")) {
	    /*
	     * content is NULL, add space-newline-space to line so strtok in
	     * output_string will pick up the newline by itself.
	     */
	append_to_line_asis(" \n ");
    } else
	append_to_line_with_indent(content, '\0', indent);
}

static void
get_hdr_attr(xmlNodePtr cur)
{
    xmlChar *trimmed;
    xmlAttr *attr;

    attr = cur->properties;
    while (attr) {
	if (!xmlStrcmp(attr->name, (const xmlChar *)"vspace")) {
	    if (attr->children->content) {
		trimmed = xstrdup(attr->children->content);
		remove_white_space(trimmed);
		vspace("hdr", trimmed);
		xfree(trimmed);
	    }
	}
	attr = attr->next;
    }
}

static void
get_arg_attr(xmlNodePtr cur)
{
    xmlChar *content, *trimmed, *opt, *repl;
    xmlAttr *attr;

    attr = cur->properties;
    opt = repl = NULL;
    while (attr) {
	if (!xmlStrcmp(attr->name, (const xmlChar *)"vspace")) {
	    if (attr->children->content) {
		trimmed = xstrdup(attr->children->content);
		remove_white_space(trimmed);
		vspace("arg", trimmed);
		xfree(trimmed);
	    }
	} else if (!xmlStrcmp(attr->name, (const xmlChar *)"flag")) {
	    if (attr->children->content) {
		trimmed = xstrdup(attr->children->content);
		remove_white_space(trimmed);
		arg_handler(attr->name, trimmed, flag_indent);
		xfree(trimmed);
	    }
	} else if (!xmlStrcmp(attr->name, (const xmlChar *)"opt")) {
	    if (attr->children->content) {
		trimmed = xstrdup(attr->children->content);
		remove_white_space(trimmed);
		if (*trimmed) {
		    opt = xmalloc(strlen(trimmed) + 1 + 2);
		    strcpy(opt, "[");
		    strcat(opt, trimmed);
		    strcat(opt, "]");
		}
		xfree(trimmed);
	    }
	} else if (!xmlStrcmp(attr->name, (const xmlChar *)"repl")) {
	    if (attr->children->content) {
		trimmed = xstrdup(attr->children->content);
		remove_white_space(trimmed);
		if (*trimmed) {
		    repl = xmalloc(strlen(trimmed) + 1 + 2);
		    strcpy(repl, "<");
		    strcat(repl, trimmed);
		    strcat(repl, ">");
		}
		xfree(trimmed);
	    }
	} else if (!xmlStrcmp(attr->name, (const xmlChar *)"text")) {
	    if (attr->children->content)
		printf("get_attr: text node with content '%s'.\n",
		       attr->children->content);
	}
	attr = attr->next;
    }

    if (opt || repl) {
	unsigned lgth = 1;
	if (opt)
	    lgth += strlen(opt);
	if (repl)
	    lgth += strlen(repl);
	content = xmalloc(lgth);
	*content = '\0';
	if (opt && *opt)
	    strcat(content, opt);
	if (repl && *repl)
	    strcat(content, repl);
	arg_handler(cur->name, content, flag_indent);
	xfree(opt);
	xfree(repl);
	xfree(content);
    }
}

static void
display_line(const char *name)
{
    if (*line) {
	if (verbose) {
	    printf("\nLine to output:\n");
	    show_line(line);
	}
	output_string(line);
	init_line();
    } else
	fprintf(stdout, "At end tag %s: line is empty.\n", name);

}

static void
desc_handler(xmlNodePtr cur)
{
    xmlChar *content;

	/* get desc:text content */
    content = get_text(cur);
    arg_handler(cur->name, content, desc_indent);
    xfree(content);

    cur = cur->children;
    cur = cur->next;

    while (cur != NULL) {
	if (!xmlStrcmp(cur->name, (const xmlChar *)"p")) {
		/* no content expected */
	    if (cur->content) {
		if (verbose)
		    printf("desc_handler: <p> content is not NULL '%s'.\n",
			   cur->content);
		continue;
	    }
	    arg_handler(cur->name, NULL, desc_indent);
	} else if (!xmlStrcmp(cur->name, (const xmlChar *)"repl")) {
	    content = get_text(cur);
	    arg_handler(cur->name, content, desc_indent);
	    xfree(content);
	} else if (!xmlStrcmp(cur->name, (const xmlChar *)"opt")) {
	    content = get_text(cur);
	    arg_handler(cur->name, content, desc_indent);
	    xfree(content);
	} else if (!xmlStrcmp(cur->name, (const xmlChar *)"text")) {
	    content = get_text(cur);
	    arg_handler(cur->name, content, desc_indent);
	    xfree(content);
	} else {
	    content = get_text(cur);
	    arg_handler(cur->name, content, desc_indent);
	    xfree(content);
	}
	cur = cur->next;
    }

}

static void
indents_handler(xmlNodePtr cur)
{
	/*
	 * Set flag_indent, desc_indent, cont_indent and word_wrap if the
	 * corresponding attrtibutes are present.
   */
    xmlChar *trimmed;
    xmlAttr *attr;

    attr = cur->properties;
    while (attr) {
	if (!xmlStrcmp(attr->name, (const xmlChar *)"flag")) {
	    if (attr->children->content) {
		trimmed = xstrdup(attr->children->content);
		remove_white_space(trimmed);
		set_var("flag_indent", &flag_indent, trimmed);
		xfree(trimmed);
	    }
	} else if (!xmlStrcmp(attr->name, (const xmlChar *)"desc")) {
	    if (attr->children->content) {
		trimmed = xstrdup(attr->children->content);
		remove_white_space(trimmed);
		set_var("desc_indent", &desc_indent, trimmed);
		xfree(trimmed);
	    }
	} else if (!xmlStrcmp(attr->name, (const xmlChar *)"cont")) {
	    if (attr->children->content) {
		trimmed = xstrdup(attr->children->content);
		remove_white_space(trimmed);
		set_var("cont_indent", &cont_indent, trimmed);
		xfree(trimmed);
	    }
	} else if (!xmlStrcmp(attr->name, (const xmlChar *)"wrap")) {
	    if (attr->children->content) {
		trimmed = xstrdup(attr->children->content);
		remove_white_space(trimmed);
		set_var("word_wrap_col", &word_wrap_col, trimmed);
		xfree(trimmed);
	    }
	}
	attr = attr->next;
    }

}

static void
hdr_handler(xmlNodePtr cur)
{
    xmlChar *content;

	/* Only vspace, processed entirely in get_hdr_attr() */
    get_hdr_attr(cur);

    content = get_text(cur);
    append_to_line_with_indent(content, '\0', NO_INDENT);
    display_line("hdr");
    xfree(content);
}

static void
select_handler(xmlNodePtr cur)
{
    const xmlChar *top = cur->name;
    xmlChar *content;

    if (!xmlStrcmp(cur->name, (const xmlChar *)"indents"))
	indents_handler(cur);
    else if (!xmlStrcmp(cur->name, (const xmlChar *)"hdr"))
	hdr_handler(cur);
    else if (!xmlStrcmp(cur->name, (const xmlChar *)"arg")) {
	    /* get arg:attribute content -- this calls arg_handler directly */
	get_arg_attr(cur);
	    /* get arg:text content */
	content = get_text(cur);
	arg_handler(cur->name, content, flag_indent);
	xfree(content);

	cur = cur->children;
	cur = cur->next;

	while (cur != NULL) {
	    if (!xmlStrcmp(cur->name, (const xmlChar *)"desc"))
		desc_handler(cur);
	    else {
		if (xmlIsBlankNode(cur)) {
		    if (verbose)
			printf("select_handler (%s:%s): no content (xmlIsBlankNode(cur) "
			       "is true)\n", top, cur->name);
		} else {
		    if (verbose)
			printf("select_handler (%s:%s): content '%s'\n",
			       top, cur->name, cur->content);
		}
	    }
	    cur = cur->next;
	}
	display_line("arg");
    } else if (xmlIsBlankNode(cur)) {
	if (verbose)
	    printf
		("select_handler (%s): no content (xmlIsBlankNode(cur) is true)\n",
		 top);
    } else {
	if (verbose)
	    printf("select_handler (%s): content '%s'\n", top, cur->content);
    }
}

static void
parse_xml(char *filename)
{
    xmlDocPtr doc;
    xmlNodePtr cur;

	/*
	 * build an XML tree from a the file;
	 */
    doc = xmlParseFile(filename);
    if (doc == NULL)
	return;

	/*
	 * Check the document is of the right kind
	 */

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
	fprintf(stderr, "empty document\n");
	xmlFreeDoc(doc);
	return;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *)"help")) {
	fprintf(stderr, "document of the wrong type, root node != help");
	xmlFreeDoc(doc);
	return;
    }

	/*
	 * Now, walk the tree.
	 */
    cur = cur->children;

    if (cur == 0)
	return;

    while (cur != NULL) {
	if (xmlIsBlankNode(cur)) {
	    if (verbose)
		printf("parse_xml (%s): node is blank "
		       "(xmlIsBlankNode(cur) true).\n", cur->name);
	} else
	    select_handler(cur);

	cur = cur->next;
    }

}

int
main(int argc, char **argv)
{
    char *fn;

#ifdef MEMWATCH
    mwInit();
#endif

    if (argc < 2)
	fn = "dar.xml";
    else
	fn = argv[1];
    fprintf(stderr, "%s: parsing XML in file %s.\n", argv[0], fn);

	/*
	 * Initial settings, may be overridden by the DTD.
	 */
    flag_indent = FLAG_INDENT;
    desc_indent = DESC_INDENT;
    cont_indent = CONT_INDENT;
    word_wrap_col = WORD_WRAP_COL;

    line = xrealloc(line, &line_alloc, INIT_LGTH);
    line_col = 0;

    parse_xml(fn);

#ifdef MEMWATCH
    mwTerm();
#endif

    return (0);
}

/*
  Local Variables:
  compile-command: "gcc -O2 -g -Wall -W -fexceptions -I/opt/include/libxml2 -L/opt/lib -lxml2 dar-help.c -o dar-help"
  End:
*/
