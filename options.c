/*
 * This file has all the code for the option command.
 * I would rather this command were not necessary, but
 * it is the only way to keep the wolves off of my back.
 *
 * @(#)options.c	3.3 (Berkeley) 5/25/81
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include "curses.h"
#include <ctype.h>
#include <string.h>
#include "machdep.h"
#include "rogue.h"

#define	NUM_OPTS	(sizeof optlist / sizeof (OPTION))

/*
 * description of an option and what to do with it
 */
struct optstruct {
    char	*o_name;	/* option name */
    char	*o_prompt;	/* prompt for interactive entry */
    void	*o_opt;		/* pointer to thing to set */
    void	(*o_putfunc)();	/* function to print value */
    int		(*o_getfunc)();	/* function to get value interactively */
};

typedef struct optstruct	OPTION;

OPTION	optlist[] = {
    {"terse",	 "Terse output: ",
		 (int *) &terse,	put_bool,	get_bool	},
    {"flush",	 "Flush typeahead during battle: ",
		 (int *) &fight_flush,	put_bool,	get_bool	},
    {"jump",	 "Show position only at end of run: ",
		 (int *) &jump,		put_bool,	get_bool	},
    {"step",	"Do inventories one line at a time: ",
		(int *) &slow_invent,	put_bool,	get_bool	},
    {"askme",	"Ask me about unidentified things: ",
		(int *) &askme,		put_bool,	get_bool	},
    {"name",	 "Name: ",
		 (int *) whoami,	put_str,	get_str		},
    {"fruit",	 "Fruit: ",
		 (int *) fruit,		put_str,	get_str		},
    {"file",	 "Save file: ",
		 (int *) file_name,	put_str,	get_str		}
};

/*
 * print and then set options from the terminal
 */
void
option()
{
    OPTION	*op;
    int	retval;

    wclear(hw);
    touchwin(hw);
    /*
     * Display current values of options
     */
    for (op = optlist; op <= &optlist[NUM_OPTS-1]; op++)
    {
	waddstr(hw, op->o_prompt);
	(*op->o_putfunc)(op->o_opt);
	waddch(hw, '\n');
    }
    /*
     * Set values
     */
    wmove(hw, 0, 0);
    for (op = optlist; op <= &optlist[NUM_OPTS-1]; op++)
    {
	waddstr(hw, op->o_prompt);
	if ((retval = (*op->o_getfunc)(op->o_opt, hw)))
	    if (retval == QUIT)
		break;
	    else if (op > optlist) {	/* MINUS */
		wmove(hw, (int)(op - optlist) - 1, 0);
		op -= 2;
	    }
	    else	/* trying to back up beyond the top */
	    {
		beep();
		wmove(hw, 0, 0);
		op--;
	    }
    }
    /*
     * Switch back to original screen
     */
    mvwaddstr(hw, LINES-1, 0, "--Press space to continue--");
    draw(hw);
    wait_for(hw,' ');
    clearok(cw, TRUE);
    touchwin(cw);
    after = FALSE;
}

/*
 * put out a boolean
 */
void
put_bool(void *b)
{
    waddstr(hw, *(int *)b ? "True" : "False");
}

/*
 * put out a string
 */
void
put_str(void *str)
{
    waddstr(hw, (char *) str);
}

/*
 * allow changing a boolean option and print it out
 */

int
get_bool(void *vp, WINDOW *win)
{
    int *bp = (int *) vp;
    int oy, ox;
    int op_bad;

    op_bad = TRUE;
    getyx(win, oy, ox);
    waddstr(win, *bp ? "True" : "False");
    while(op_bad)	
    {
	wmove(win, oy, ox);
	draw(win);
	switch (readchar(win))
	{
	    case 't':
	    case 'T':
		*bp = TRUE;
		op_bad = FALSE;
		break;
	    case 'f':
	    case 'F':
		*bp = FALSE;
		op_bad = FALSE;
		break;
	    case '\n':
	    case '\r':
		op_bad = FALSE;
		break;
	    case '\033':
	    case '\007':
		return QUIT;
	    case '-':
		return MINUS;
	    default:
		mvwaddstr(win, oy, ox + 10, "(T or F)");
	}
    }
    wmove(win, oy, ox);
    waddstr(win, *bp ? "True" : "False");
    waddch(win, '\n');
    return NORM;
}

/*
 * set a string option
 */
int
get_str(void *vopt, WINDOW *win)
{
    char *opt = (char *) vopt;
    char *sp;
    int c, oy, ox;
    char buf[80];

    draw(win);
    getyx(win, oy, ox);
    /*
     * loop reading in the string, and put it in a temporary buffer
     */
    for (sp = buf;
	(c = readchar(win)) != '\n' && c != '\r' && c != '\033' && c != '\007';
	wclrtoeol(win), draw(win))
    {
	if (c == -1)
	    continue;
	else if (c == md_erasechar())	/* process erase character */
	{
	    if (sp > buf)
	    {
		int i;
		int myx, myy;

		sp--;

		for (i = (int) strlen(unctrl(*sp)); i; i--)
		{
		    getyx(win,myy,myx);
		    if ((myx == 0)&& (myy > 0))
		    {
			wmove(win,myy-1,getmaxx(win)-1);
			waddch(win,' ');
			wmove(win,myy-1,getmaxx(win)-1);
		    }
		    else
			waddch(win, '\b');
		}
	    }
	    continue;
	}
	else if (c == md_killchar())	/* process kill character */
	{
	    sp = buf;
	    wmove(win, oy, ox);
	    continue;
	}
	else if (sp == buf)
	    if (c == '-')
		break;
	    else if (c == '~')
	    {
		strcpy(buf, home);
		waddstr(win, home);
		sp += strlen(home);
		continue;
	    }

	if ((sp - buf) < 78) /* Avoid overflow */
	{
	    *sp++ = c;
	    waddstr(win, unctrl(c));
	}
    }
    *sp = '\0';
    if (sp > buf)	/* only change option if something has been typed */
	strucpy(opt, buf, strlen(buf));
    wmove(win, oy, ox);
    waddstr(win, opt);
    waddch(win, '\n');
    draw(win);
    if (win == cw)
	mpos += (int)(sp - buf);
    if (c == '-')
	return MINUS;
    else if (c == '\033' || c == '\007')
	return QUIT;
    else
	return NORM;
}

/*
 * parse options from string, usually taken from the environment.
 * the string is a series of comma seperated values, with booleans
 * being stated as "name" (true) or "noname" (false), and strings
 * being "name=....", with the string being defined up to a comma
 * or the end of the entire option string.
 */

void
parse_opts(char *str)
{
    char *sp;
    OPTION *op;
    int len;

    while (*str)
    {
	/*
	 * Get option name
	 */
	for (sp = str; isalpha(*sp); sp++)
	    continue;
	len = (int)(sp - str);
	/*
	 * Look it up and deal with it
	 */
	for (op = optlist; op <= &optlist[NUM_OPTS-1]; op++)
	    if (EQSTR(str, op->o_name, len))
	    {
		if (op->o_putfunc == put_bool)	/* if option is a boolean */
		    *(int *)op->o_opt = TRUE;
		else				/* string option */
		{
		    char *start;
		    /*
		     * Skip to start of string value
		     */
		    for (str = sp + 1; *str == '='; str++)
			continue;
		    if (*str == '~')
		    {
			strcpy((char *) op->o_opt, home);
			start = (char *) op->o_opt + strlen(home);
			while (*++str == '/')
			    continue;
		    }
		    else
			start = (char *) op->o_opt;
		    /*
		     * Skip to end of string value
		     */
		    for (sp = str + 1; *sp && *sp != ','; sp++)
			continue;
		    strucpy(start, str, sp - str);
		}
		break;
	    }
	    /*
	     * check for "noname" for booleans
	     */
	    else if (op->o_putfunc == put_bool
	      && EQSTR(str, "no", 2) && EQSTR(str + 2, op->o_name, len - 2))
	    {
		*(int *)op->o_opt = FALSE;
		break;
	    }

	/*
	 * skip to start of next option name
	 */
	while (*sp && !isalpha(*sp))
	    sp++;
	str = sp;
    }
}

/*
 * copy string using unctrl for things
 */
void
strucpy(char *s1, char *s2, size_t len)
{
    const char *sp;

    while (len--)
    {
    	sp = unctrl(*s2);
	strcpy(s1, sp);
	s1 += strlen(sp);
	s2++;
    }
    *s1 = '\0';
}
