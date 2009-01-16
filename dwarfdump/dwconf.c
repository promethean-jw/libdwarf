/*
  Copyright (C) 2006 Silicon Graphics, Inc.  All Rights Reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it would be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Further, this software is distributed without any warranty that it is
  free of the rightful claim of any third person regarding infringement
  or the like.  Any license provided herein, whether implied or
  otherwise, applies only to this software file.  Patent licenses, if
  any, provided herein do not apply to combinations of this program with
  other software, or any other product whatsoever.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston MA 02111-1307, USA.

  Contact information:  Silicon Graphics, Inc., 1500 Crittenden Lane,
  Mountain View, CA 94043, or:

  http://www.sgi.com

  For further information regarding this notice, see:

  http://oss.sgi.com/projects/GenInfo/NoticeExplan


$Header: /plroot/cmplrs.src/v7.4.5m/.RCS/PL/dwarfdump/RCS/dwconf.c,v 1.4 2006/04/18 18:05:57 davea Exp $ */


#include "globals.h"
#include <ctype.h>
#include "dwconf.h"
#include "makename.h"


struct token_s {
    unsigned tk_len;
    char *tk_data;
};
enum linetype_e {
    LT_ERROR,
    LT_COMMENT,
    LT_BLANK,
    LT_BEGINABI,
    LT_REG,
    LT_FRAME_INTERFACE,
    LT_CFA_REG,
    LT_INITIAL_REG_VALUE,
    LT_REG_TABLE_SIZE,
    LT_ENDABI
};

struct comtable_s {
    enum linetype_e type;
    char *name;
    size_t namelen;
};

static int errcount = 0;	/* Count errors found in this scan of
				   the configuration file. */

static char name_begin_abi[] = "beginabi:";
static char name_reg[] = "reg:";
static char name_frame_interface[] = "frame_interface:";
static char name_cfa_reg[] = "cfa_reg:";
static char name_initial_reg_value[] = "initial_reg_value:";
static char name_reg_table_size[] = "reg_table_size:";
static char name_endabi[] = "endabi:";

static struct comtable_s comtable[] = {
    {LT_BEGINABI, name_begin_abi},
    {LT_REG, name_reg},
    {LT_FRAME_INTERFACE, name_frame_interface},
    {LT_CFA_REG, name_cfa_reg},
    {LT_INITIAL_REG_VALUE, name_initial_reg_value},
    {LT_REG_TABLE_SIZE, name_reg_table_size},
    {LT_ENDABI, name_endabi},
};
static int size_of_comtable = sizeof(comtable) / sizeof(comtable[0]);


static FILE *find_a_file(char *named_file, char **defaults,
			 string * name_used);
static int find_abi_start(FILE * stream, char *abi_name, long *offset,
			  unsigned long *lineno_out);
static int parse_abi(FILE * stream, char *fname, char *abiname,
		     struct dwconf_s *out, unsigned long lineno);
static char *get_token(char *cp, struct token_s *outtok);





/*  This finds a dwarfdump.conf file and
    then parses it.  It updates
    conf_out as appropriate.

    This finds the first file (looking in a set of places)
    with that name.  It then looks for the right  ABI entry.
    If the first file it finds does not have that ABI entry it
    gives up.
  
    It would also be reasonable to search every 'dwarfdump.conf'
    it finds for the abi. But we stop at the first dwarfdump.conf
    we find.
*/
int
find_conf_file_and_read_config(char *named_file,
			       char *named_abi, char **defaults,
			       struct dwconf_s *conf_out)
{

    FILE *conf_stream = 0;
    char *name_used = 0;
    long offset = 0;
    int res = FALSE;
    unsigned long lineno = 0;

    errcount = 0;

    conf_stream = find_a_file(named_file, defaults, &name_used);
    if (!conf_stream) {
	++errcount;
	printf("dwarfdump found no file %s!\n",
	       named_file ? named_file : "readable for configuration. "
	       "(add options -v -v to see what file names tried)\n");
	return errcount;
    }
    if (verbose > 1) {
	printf("dwarfdump using configuration file %s\n", name_used);
    }

    res = find_abi_start(conf_stream, named_abi, &offset, &lineno);
    if (errcount > 0) {
	++errcount;
	printf("dwarfdump found no ABI %s in file %s.\n",
	       named_abi, name_used);
	return errcount;
    }
    res = fseek(conf_stream, offset, SEEK_SET);
    if (res != 0) {
	++errcount;
	printf("dwarfdump seek to %ld offset in %s failed!\n",
	       offset, name_used);
	return errcount;
    }
    parse_abi(conf_stream, name_used, named_abi, conf_out, lineno);
    fclose(conf_stream);
    return errcount;
}

/* Given path strings, attempt to make a canonical file name:
   that is, avoid superfluous '/' so that no
    '//' (or worse) is created in the output. The path components
    are to be separated so at least one '/'
    is to appear between the two 'input strings' when
    creating the output.
*/
static char *
canonical_append(char *target, unsigned int target_size,
		 char *first_string, char *second_string)
{
    size_t firstlen = strlen(first_string);

    /* +1 +1: Leave room for added "/" and final NUL, though that is
       overkill, as we drop a NUL byte too. */
    if ((firstlen + strlen(second_string) + 1 + 1) >= target_size) {
	/* Not enough space. */
	return NULL;
    }
    for (; *second_string == '/'; ++second_string) {
    }
    for (; firstlen > 0 && first_string[firstlen - 1] == '/';
	 --firstlen) {
    }
    target[0] = 0;
    if (firstlen > 0) {
	strncpy(target, first_string, firstlen);
	target[firstlen + 1] = 0;
    }
    target[firstlen] = '/';
    firstlen++;
    target[firstlen] = 0;
    strcat(target, second_string);
    return target;
}

#ifdef BUILD_FOR_TEST
#define CANBUF 25
struct canap_s {
    char *res_exp;
    char *first;
    char *second;
} canap[] = {
    {
    "ab/c", "ab", "c"}, {
    "ab/c", "ab/", "c"}, {
    "ab/c", "ab", "/c"}, {
    "ab/c", "ab////", "/////c"}, {
    "ab/", "ab", ""}, {
    "ab/", "ab////", ""}, {
    "ab/", "ab////", ""}, {
    "/a", "", "a"}, {
    0, "/abcdefgbijkl", "pqrstuvwxyzabcd"}, {
    0, 0, 0}
};
static void
test_canonical_append(void)
{
    /* Make buf big, this is test code, so be safe. */
    char lbuf[1000];
    unsigned i;
    unsigned failcount = 0;

    printf("Entry test_canonical_append\n");
    for (i = 0;; ++i) {
	char *res = 0;

	if (canap[i].first == 0 && canap[i].second == 0)
	    break;

	res = canonical_append(lbuf, CANBUF, canap[i].first,
			       canap[i].second);
	if (res == 0) {
	    if (canap[i].res_exp == 0) {
		/* GOOD */
		printf("PASS %u\n", i);
	    } else {
		++failcount;
		printf("FAIL: entry %u wrong, expected %s, got NULL\n",
		       i, canap[i].res_exp);
	    }
	} else {
	    if (canap[i].res_exp == 0) {
		++failcount;
		printf("FAIL: entry %u wrong, got %s expected NULL\n",
		       i, res);
	    } else {
		if (strcmp(res, canap[i].res_exp) == 0) {
		    printf("PASS %u\n", i);
		    /* GOOD */
		} else {
		    ++failcount;
		    printf("FAIL: entry %u wrong, expected %s got %s\n",
			   i, canap[i].res_exp, res);
		}
	    }
	}
    }
    printf("FAIL count %u\n", failcount);

}
#endif /* BUILD_FOR_TEST */
/* Try to find a file as named and open for read.
   We treat each name as a full name, we are not
   combining separate name and path components.
   This is  an arbitrary choice...

    The defaults are listed in dwarfdump.c in the array 
    config_file_defaults[].
*/
static FILE *
find_a_file(char *named_file, char **defaults, string * name_used)
{
    FILE *fin = 0;
    char *lname = named_file;
    const char *type = "rw";
    int i = 0;

#ifdef BUILD_FOR_TEST
    test_canonical_append();
#endif /* BUILD_FOR_TEST */

    if (lname) {
	/* Name given, just assume it is fully correct, try no other. */
	if (verbose > 1) {
	    printf("dwarfdump looking for configuration as %s\n",
		   lname);
	}
	fin = fopen(lname, type);
	if (fin) {
	    *name_used = lname;
	    return fin;
	}
	return 0;
    }
    /* No name given, find a default, if we can. */
    for (i = 0; defaults[i]; ++i) {
	lname = defaults[i];
	if (strncmp(lname, "HOME/", 5) == 0) {
	    /* arbitrary size */
	    char buf[2000];
	    char *homedir = getenv("HOME");

	    if (homedir) {
		char *cp = canonical_append(buf, sizeof(buf),
					    homedir, lname + 5);

		if (!cp) {
		    /* OOps, ignore this one. */
		    continue;
		}
		lname = makename(buf);
	    }
	}
	if (verbose > 1) {
	    printf("dwarfdump looking for configuration as %s\n",
		   lname);
	}
	fin = fopen(lname, type);
	if (fin) {
	    *name_used = lname;
	    return fin;
	}
    }
    return 0;
}

/* Start at a token begin, see how long it is,
   return length. */
unsigned
find_token_len(char *cp)
{
    unsigned len = 0;

    for (; *cp; ++cp) {
	if (isspace(*cp)) {
	    return len;
	}
	if (*cp == '#') {
	    return len;		/* begins comment */
	}
	++len;
    }
    return len;
}

/*
   Skip past all whitespace: the only code that even knows
   what whitespace is.
*/
static char *
skipwhite(char *cp)
{
    for (; *cp; ++cp) {
	if (!isspace(*cp)) {
	    return cp;
	}
    }
    return cp;
}

/* Return TRUE if ok. FALSE if find more tokens.
   Emit error message if error.
*/
static int
ensure_has_no_more_tokens(char *cp, char *fname, unsigned long lineno)
{
    struct token_s tok;

    cp = get_token(cp, &tok);
    if (tok.tk_len > 0) {
	printf("dwarfdump.conf error: "
	       "extra characters after command operands, found "
	       "\"%s\" in %s line %lu\n", tok.tk_data, fname, lineno);
	++errcount;
	return FALSE;
    }
    return TRUE;
}


/*
	There may be many  beginabi: lines in a dwarfdump.conf file,
	find the one we want and return it's file offset.
*/
static int
find_abi_start(FILE * stream,
	       char *abi_name, long *offset, unsigned long *lineno_out)
{
    char buf[100];
    unsigned long lineno = 0;

    for (; !feof(stream);) {

	struct token_s tok;
	char *line = 0;
	long loffset = ftell(stream);

	line = fgets(buf, sizeof(buf), stream);
	++lineno;
	if (!line) {
	    ++errcount;
	    return FALSE;
	}

	line = get_token(buf, &tok);

	if (strcmp(tok.tk_data, name_begin_abi) != 0) {
	    continue;
	}
	get_token(line, &tok);
	if (strcmp(tok.tk_data, abi_name) != 0) {
	    continue;
	}

	*offset = loffset;
	*lineno_out = lineno;
	return TRUE;
    }

    ++errcount;
    return FALSE;
}

static char *tempstr = 0;
static unsigned tempstr_len = 0;

/*
	Use a global buffer (tempstr) to turn a non-delimited
	input char array into a NUL-terminated C string
        (with the help of makename() to get a permanent
        address for the result ing string).
*/
static char *
build_string(unsigned tlen, char *cp)
{
    if (tlen >= tempstr_len) {
	free(tempstr);
	tempstr = malloc(tlen + 100);
    }
    strncpy(tempstr, cp, tlen);
    tempstr[tlen] = 0;
    return makename(tempstr);
}

/*
	The tokenizer for our simple parser.
*/
static char *
get_token(char *cp, struct token_s *outtok)
{
    char *lcp = skipwhite(cp);
    unsigned tlen = find_token_len(lcp);

    outtok->tk_len = tlen;
    if (tlen > 0) {
	outtok->tk_data = build_string(tlen, lcp);
    } else {
	outtok->tk_data = "";
    }
    return lcp + tlen;

}

/*
	We can't get all the field set up statically very easily,
        so we get the command string length set here.
*/
static void
finish_comtable_setup(void)
{
    unsigned i;

    for (i = 0; i < size_of_comtable; ++i) {
	comtable[i].namelen = strlen(comtable[i].name);
    }
}

/*
    Given  a line of the table, determine if it is a command
    or not, and if a command, which one is it.
    Return LT_ERROR if it's not recognized.
*/
static enum linetype_e
which_command(char *cp, struct comtable_s **tableentry)
{
    int i;
    struct token_s tok;

    if (*cp == '#')
	return LT_COMMENT;
    if (!*cp)
	return LT_BLANK;

    get_token(cp, &tok);

    for (i = 0; i < size_of_comtable; ++i) {
	if (tok.tk_len == comtable[i].namelen &&
	    strcmp(comtable[i].name, tok.tk_data) == 0) {

	    *tableentry = &comtable[i];
	    return comtable[i].type;
	}
    }

    return LT_ERROR;
}

/* We are promised it's an abiname: command
   find the name on the line.
*/
static int
parsebeginabi(char *cp, char *fname, char *abiname,
	      unsigned long lineno, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    size_t abinamelen = strlen(abiname);
    struct token_s tok;


    cp = cp + clen + 1;
    cp = skipwhite(cp);
    get_token(cp, &tok);
    if (tok.tk_len != abinamelen ||
	strncmp(cp, abiname, abinamelen) != 0) {
	printf("dwarfdump internal error: "
	       "mismatch %s with %s   %s line %lu\n",
	       cp, tok.tk_data, fname, lineno);
	++errcount;
	return FALSE;
    }
    {
	int res =
	    ensure_has_no_more_tokens(cp + tok.tk_len, fname, lineno);
	return res;
    }
}

/* This expands register names as required, but does not
   ensure no names duplicated.
*/
#define CONF_TABLE_OVERSIZE  100
static void
add_to_reg_table(struct dwconf_s *conf,
		 char *rname, unsigned long rval, char *fname,
		 unsigned long lineno)
{
    if (conf->cf_regs_malloced == 0) {
	conf->cf_regs = 0;
	conf->cf_named_regs_table_size = 0;
    }
    if (rval >= conf->cf_named_regs_table_size) {
	char **newregs = 0;
	unsigned long newtablen = rval + CONF_TABLE_OVERSIZE;
	unsigned long newtabsize = newtablen * sizeof(char *);
	unsigned long oldtabsize =
	    conf->cf_named_regs_table_size * sizeof(char *);
	newregs = realloc(conf->cf_regs, newtabsize);
	if (!newregs) {
	    printf("dwarfdump: unable to malloc table %lu bytes. "
		   " %s line %lu\n", newtabsize, fname, lineno);
	    exit(1);
	}
	/* Zero out the new entries. */
	memset((char *) newregs + (oldtabsize), 0,
	       (newtabsize - oldtabsize));
	conf->cf_named_regs_table_size = (unsigned long) newtablen;
	conf->cf_regs = newregs;
	conf->cf_regs_malloced = 1;
    }
    conf->cf_regs[rval] = rname;
    return;
}

/* Our input is supposed to be a number.
   Determine the value (and return it) or generate an error message.
*/
static int
make_a_number(char *cmd, char *filename, unsigned long
	      lineno, struct token_s *tok, unsigned long *val_out)
{
    char *endnum = 0;
    unsigned long val = 0;

    val = strtoul(tok->tk_data, &endnum, 0);
    if (val == 0 && endnum == (tok->tk_data)) {
	printf("dwarfdump.conf error: "
	       "%s missing register number (\"%s\" not valid)  %s line %lu",
	       cmd, tok->tk_data, filename, lineno);
	++errcount;
	return FALSE;
    }
    if (endnum != (tok->tk_data + tok->tk_len)) {
	printf("dwarfdump.conf error: "
	       "%s Missing register number (\"%s\" not valid)  %s line %lu",
	       cmd, tok->tk_data, filename, lineno);
	++errcount;
	return FALSE;
    }
    *val_out = val;
    return TRUE;



}

/* We are guaranteed it's a reg: command, so parse that command
    and record the interesting data.
*/
static int
parsereg(char *cp, char *fname, unsigned long lineno,
	 struct dwconf_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s regnum;
    struct token_s tokreg;
    unsigned long val = 0;
    int ok = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tokreg);
    cp = get_token(cp, &regnum);
    if (tokreg.tk_len == 0) {
	printf("dwarfdump.conf error: "
	       "reg: missing register name  %s line %lu",
	       fname, lineno);
	++errcount;
	return FALSE;

    }
    if (regnum.tk_len == 0) {
	printf("dwarfdump.conf error: "
	       "reg: missing register number  %s line %lu",
	       fname, lineno);
	++errcount;
	return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &regnum, &val);

    if (!ok) {
	++errcount;
	return FALSE;
    }

    add_to_reg_table(conf, tokreg.tk_data, val, fname, lineno);

    {
	int res = ensure_has_no_more_tokens(cp, fname, lineno);

	return res;
    }
}

/*
   We are guaranteed it's an frame_interface: command.
   Parse it and record the value data.
*/
static int
parseframe_interface(char *cp, char *fname, unsigned long lineno,
		     struct dwconf_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
	printf("dwarfdump.conf error: "
	       "%s missing interface number %s line %lu",
	       comtab->name, fname, lineno);
	++errcount;
	return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {
	++errcount;
	return FALSE;
    }
    if (val != 2 && val != 3) {
	printf("dwarfdump.conf error: "
	       "%s only interface numbers 2 or 3 are allowed, "
	       " not %lu. %s line %lu",
	       comtab->name, val, fname, lineno);
	++errcount;
	return FALSE;
    }

    conf->cf_interface_number = (int) val;
    {
	int res = ensure_has_no_more_tokens(cp, fname, lineno);

	return res;
    }
}

/*
   We are guaranteed it's a cfa_reg: command. Parse it
   and record the important data.
*/
static int
parsecfa_reg(char *cp, char *fname, unsigned long lineno,
	     struct dwconf_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
	printf("dwarfdump.conf error: "
	       "%s missing cfa_reg number %s line %lu",
	       comtab->name, fname, lineno);
	++errcount;
	return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {
	++errcount;
	return FALSE;
    }
    conf->cf_cfa_reg = (int) val;
    {
	int res = ensure_has_no_more_tokens(cp, fname, lineno);

	return res;
    }
}


/* We are guaranteed it's an initial_reg_value: command,
   parse it and put the reg value where it will be remembered. 
*/
static int
parseinitial_reg_value(char *cp, char *fname,
		       unsigned long lineno,
		       struct dwconf_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
	printf("dwarfdump.conf error: "
	       "%s missing initial reg value %s line %lu",
	       comtab->name, fname, lineno);
	++errcount;
	return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {

	++errcount;
	return FALSE;
    }
    conf->cf_initial_rule_value = (int) val;
    {
	int res = ensure_has_no_more_tokens(cp, fname, lineno);

	return res;
    }
}


/* We are guaranteed it's a table size command, parse it
    and record the table size.
*/
static int
parsereg_table_size(char *cp, char *fname, unsigned long lineno,
		    struct dwconf_s *conf, struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;
    unsigned long val = 0;
    int ok = FALSE;

    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (tok.tk_len == 0) {
	printf("dwarfdump.conf error: "
	       "%s missing reg table size value %s line %lu",
	       comtab->name, fname, lineno);
	++errcount;
	return FALSE;
    }

    ok = make_a_number(comtab->name, fname, lineno, &tok, &val);

    if (!ok) {
	++errcount;
	return FALSE;
    }
    conf->cf_table_entry_count = (unsigned long) val;
    {
	int res = ensure_has_no_more_tokens(cp, fname, lineno);

	return res;
    }

}


/*  We are guaranteed it's an endabi: command, parse it and
    check we have the right abi.
*/
static int
parseendabi(char *cp, char *fname, char *abiname, unsigned long lineno,
	    struct comtable_s *comtab)
{
    size_t clen = comtab->namelen;
    struct token_s tok;


    cp = cp + clen + 1;
    cp = get_token(cp, &tok);
    if (strcmp(abiname, tok.tk_data) != 0) {
	printf("%s error: "
	       "mismatch abi name %s (here) vs. %s (beginabi:)  %s line %lu\n",
	       comtab->name, tok.tk_data, abiname, fname, lineno);
	++errcount;
	return FALSE;
    }
    {
	int res = ensure_has_no_more_tokens(cp, fname, lineno);

	return res;
    }

}



/* Return TRUE if we succeeded and filed in *out.
   Return FALSE if we failed (and fill in nothing).
  beginabi:  <abiname>
  reg: <regname> <dwarf regnumber>
  frame_interface: <integer value 2 or 3>
  cfa_reg:  <number>
  initial_reg_value:  <number: normally 1034 or 1035 >
  reg_table_size: <size of table>
  endabi:  <abiname>
 
  We are positioned at the start of a beginabi: line when
  called.

*/
static int
parse_abi(FILE * stream, char *fname, char *abiname,
	  struct dwconf_s *out, unsigned long lineno)
{
    struct dwconf_s localconf;
    char buf[1000];
    int comtype = 0;
    long regcount = 0;

    unsigned long beginabi_lineno = 0;
    unsigned long frame_interface_lineno = 0;
    unsigned long initial_reg_value_lineno = 0;
    unsigned long reg_table_size_lineno = 0;
    unsigned long cfa_reg_lineno = 0;
    static int first_time_done = 0;
    struct comtable_s *comtabp = 0;


    if (first_time_done == 0) {
	finish_comtable_setup();
	first_time_done = 1;
    }




    init_conf_file_data(&localconf);

    for (; !feof(stream);) {

	char *line = 0;

	/* long loffset = ftell(stream); */
	line = fgets(buf, sizeof(buf), stream);
	if (!line) {
	    ++errcount;
	    printf
		("dwarfdump: end of file or error before endabi: in %s, line %lu\n",
		 fname, lineno);
	    return FALSE;
	}
	++lineno;
	line = skipwhite(line);
	comtype = which_command(line, &comtabp);
	switch (comtype) {
	case LT_ERROR:
	    ++errcount;
	    printf
		("dwarfdump: Unknown text in %s is \"%s\" at line %lu\n",
		 fname, line, lineno);
	    break;
	case LT_COMMENT:
	    break;
	case LT_BLANK:
	    break;
	case LT_BEGINABI:
	    if (beginabi_lineno > 0) {
		++errcount;
		printf
		    ("dwarfdump: Encountered beginabi: when not expected. "
		     "%s line %lu previous beginabi line %lu\n", fname,
		     lineno, beginabi_lineno);
	    }
	    beginabi_lineno = lineno;
	    parsebeginabi(line, fname, abiname, lineno, comtabp);
	    break;

	case LT_REG:
	    parsereg(line, fname, lineno, &localconf, comtabp);
	    ++regcount;
	    break;
	case LT_FRAME_INTERFACE:
	    if (frame_interface_lineno > 0) {
		++errcount;
		printf
		    ("dwarfdump: Encountered duplicate frame_interface: "
		     "%s line %lu previous frame_interface: line %lu\n",
		     fname, lineno, frame_interface_lineno);
	    }
	    frame_interface_lineno = lineno;
	    parseframe_interface(line, fname,
				 lineno, &localconf, comtabp);
	    break;
	case LT_CFA_REG:
	    if (cfa_reg_lineno > 0) {
		printf("dwarfdump: Encountered duplicate cfa_reg: "
		       "%s line %lu previous cfa_reg line %lu\n",
		       fname, lineno, cfa_reg_lineno);
		++errcount;
	    }
	    cfa_reg_lineno = lineno;
	    parsecfa_reg(line, fname, lineno, &localconf, comtabp);
	    break;
	case LT_INITIAL_REG_VALUE:
	    if (initial_reg_value_lineno > 0) {
		printf
		    ("dwarfdump: Encountered duplicate initial_reg_value_lineno: "
		     "%s line %lu previous initial_reg_value: line %lu\n",
		     fname, lineno, initial_reg_value_lineno);
		++errcount;
	    }
	    initial_reg_value_lineno = lineno;

	    parseinitial_reg_value(line, fname,
				   lineno, &localconf, comtabp);
	    break;
	case LT_REG_TABLE_SIZE:
	    if (reg_table_size_lineno > 0) {
		printf("dwarfdump: duplicate reg_table_size: "
		       "%s line %lu previous reg_table_size: line %lu\n",
		       fname, lineno, reg_table_size_lineno);
		++errcount;
	    }
	    reg_table_size_lineno = lineno;
	    parsereg_table_size(line, fname,
				lineno, &localconf, comtabp);
	    break;
	case LT_ENDABI:
	    parseendabi(line, fname, abiname, lineno, comtabp);

	    if (regcount > localconf.cf_table_entry_count) {
		printf("dwarfdump: more registers named than "
		       " in  %s  ( %lu named vs  %s %lu)  %s line %lu\n",
		       abiname, (unsigned long) regcount,
		       name_reg_table_size,
		       (unsigned long) localconf.cf_table_entry_count,
		       fname, (unsigned long) lineno);
		++errcount;
	    }

	    *out = localconf;
	    return TRUE;
	default:
	    printf
		("dwarfdump internal error, impossible line type %d  %s %lu \n",
		 (int) comtype, fname, lineno);
	    exit(1);

	}
    }
    ++errcount;
    printf("End of file, no endabi: found. %s, line %lu\n",
	   fname, lineno);
    return FALSE;
}

/* MIPS/IRIX frame register names.
   For alternate name sets, use dwarfdump.conf or
   revise dwarf.h and libdwarf.h and this table.
*/
static char *regnames[] = {
    "cfa",
    "r1/at", "r2/v0", "r3/v1",
    "r4/a0", "r5/a1", "r6/a2", "r7/a3",
    "r8/t0", "r9/t1", "r10/t2", "r11/t3",
    "r12/t4", "r13/t5", "r14/t6", "r15/t7",
    "r16/s0", "r17/s1", "r18/s2", "r19/s3",
    "r20/s4", "r21/s5", "r22/s6", "r23/s7",
    "r24/t8", "r25/t9", "r26/k0", "r27/k1",
    "r28/gp", "r29/sp", "r30/s8", "r31",

    "$f0", "$f1",
    "$f2", "$f3",
    "$f4", "$f5",
    "$f6", "$f7",
    "$f8", "$f9",
    "$f10", "$f11",
    "$f12", "$f13",
    "$f14", "$f15",
    "$f16", "$f17",
    "$f18", "$f19",
    "$f20", "$f21",
    "$f22", "$f23",
    "$f24", "$f25",
    "$f26", "$f27",
    "$f28", "$f29",
    "$f30", "$f31",
    "ra", "slk",
};


/* These defaults match MIPS/IRIX ABI defaults.
   For other ABIs, see -x abi=<whatever>
   to configure dwarfdump (and libdwarf) frame 
   data reporting at runtime.
*/
void
init_conf_file_data(struct dwconf_s *config_file_data)
{
    unsigned long base_table_count =
	sizeof(regnames) / sizeof(regnames[0]);

    memset(config_file_data, 0, sizeof(*config_file_data));
    config_file_data->cf_interface_number = 2;
    config_file_data->cf_table_entry_count = DW_REG_TABLE_SIZE;
    config_file_data->cf_initial_rule_value =
	DW_FRAME_REG_INITIAL_VALUE;
    config_file_data->cf_cfa_reg = DW_FRAME_CFA_COL;
    config_file_data->cf_regs = regnames;
    config_file_data->cf_named_regs_table_size = base_table_count;
    config_file_data->cf_regs_malloced = 0;
    if (config_file_data->cf_table_entry_count != base_table_count) {
	printf("dwarfdump: improper base table initization, "
	       "header files wrong: "
	       "DW_REG_TABLE_SIZE %u != string table size %lu\n",
	       (unsigned) DW_REG_TABLE_SIZE,
	       (unsigned long) base_table_count);
	exit(1);
    }

    return;
}

/*  Print the 'right' string for the register we are given.
    Deal sensibly with the special regs as well as numbers
    we know and those we have not been told about.

*/
void
print_reg_from_config_data(char *intfmt, Dwarf_Signed reg,
			   struct dwconf_s *config_data)
{
    char *name = 0;

    if (reg == config_data->cf_cfa_reg) {
	printf("%s", "cfa");
	return;
    }
    if (reg == DW_FRAME_CFA_COL3) {
	/* This should not be necessary, but sometimes one forgets to
	   do the cfa_reg: command in dwarfdump.conf */
	printf("%s", "cfa");
	return;
    }
    if (reg == DW_FRAME_UNDEFINED_VAL) {
	printf("u");
	return;
    }
    if (reg == DW_FRAME_SAME_VAL) {
	printf("s");
	return;
    }
    if (config_data->cf_regs == 0 ||
	reg < 0 || reg > config_data->cf_named_regs_table_size) {
	printf(intfmt, (signed long long) reg);
	return;
    }
    name = config_data->cf_regs[reg];
    if (!name) {
	printf(intfmt, (signed long long) reg);
	return;
    }
    printf("%s", name);
    return;
}
