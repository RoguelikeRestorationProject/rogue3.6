/*
 * File for the fun ends
 * Death or a total win
 *
 * @(#)rip.c	3.13 (Berkeley) 6/16/81
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "curses.h"
#include "machdep.h"
#include "rogue.h"

static char *rip[] = {
"                       __________",
"                      /          \\",
"                     /    REST    \\",
"                    /      IN      \\",
"                   /     PEACE      \\",
"                  /                  \\",
"                  |                  |",
"                  |                  |",
"                  |   killed by a    |",
"                  |                  |",
"                  |       1980       |",
"                 *|     *  *  *      | *",
"         ________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______",
    0
};

char	*killname();

/*
 * death:
 *	Do something really fun when he dies
 */

void
death(int monst)
{
    char **dp = rip, *killer;
    struct tm *lt;
    time_t date;
    char buf[80];

    time(&date);
    lt = localtime(&date);
    clear();
    move(8, 0);
    while (*dp)
	printw("%s\n", *dp++);
    mvaddstr(14, 28-(((int)strlen(whoami)+1)/2), whoami);
    purse -= purse/10;
    sprintf(buf, "%d Au", purse);
    mvaddstr(15, 28-(((int)strlen(buf)+1)/2), buf);
    killer = killname(monst);
    mvaddstr(17, 28-(((int)strlen(killer)+1)/2), killer);
    mvaddstr(16, 33, vowelstr(killer));
    sprintf(prbuf, "%4d", 1900+lt->tm_year);
    mvaddstr(18, 26, prbuf);
    move(LINES-1, 0);
    draw(stdscr);
    score(purse, 0, monst);
    exit(0);
}

/*
 * score -- figure score and post it.
 */

void
open_score(void)
{
#ifdef SCOREFILE
    char *scorefile = SCOREFILE;

    if (scoreboard != NULL) {
        rewind(scoreboard);
        return;
    }

    scoreboard = fopen(scorefile, "r+");

    if ((scoreboard == NULL) && (errno == ENOENT))
    {
        scoreboard = fopen(scorefile, "w+");
        md_chmod(scorefile,0664);
    }

    if (scoreboard == NULL) {
         fprintf(stderr, "Could not open %s for writing: %s\n", scorefile, strerror(errno));
         fflush(stderr);
    }
#else
    scoreboard = NULL;
#endif
}

/* VARARGS2 */
void
score(int amount, int flags, int monst)
{
    static struct sc_ent {
	int sc_score;
	char sc_name[80];
	int sc_flags;
	int sc_level;
	char sc_login[8];
	int sc_monster;
    } top_ten[10];
    struct sc_ent *scp;
    int i;
    struct sc_ent *sc2;
    FILE *outf;
    char *killer;
    int prflags = 0;
    static char *reason[] = {
	"killed",
	"quit",
	"A total winner",
    };
    char scoreline[100];
    int rogue_ver = 0, scorefile_ver = 0;

    /*
     * Open file and read list
     */

    if (scoreboard == NULL)
        return;
    
    outf = scoreboard;

    for (scp = top_ten; scp <= &top_ten[9]; scp++)
    {
	scp->sc_score = 0;
	for (i = 0; i < 80; i++)
	    scp->sc_name[i] = rnd(255);
	scp->sc_flags = RN;
	scp->sc_level = RN;
	scp->sc_monster = RN;
	scp->sc_login[0] = '\0';
    }

    signal(SIGINT, SIG_DFL);
    if ((flags != -1) && (flags != 1))
    {
	mvaddstr(LINES-1, 0, "[Press return to continue]");
	draw(stdscr);
	prbuf[0] = 0;
	get_str(prbuf, stdscr);
	endwin();
    }
    if (wizard)
	if (strcmp(prbuf, "names") == 0)
	    prflags = 1;
	else if (strcmp(prbuf, "edit") == 0)
	    prflags = 2;

    md_lockfile(outf);

    encread(scoreline, 100, outf);
    (void) sscanf(scoreline, "R%d %d\n", &rogue_ver, &scorefile_ver);

    if ((rogue_ver == 36) && (scorefile_ver == 2))
        for(i = 0; i < 10; i++)
	{
	    encread(&top_ten[i].sc_name, 80, outf);
	    encread(&top_ten[i].sc_login, 8, outf);
	    encread(scoreline, 100, outf);
	    (void) sscanf(scoreline, " %d %d %d %d \n",
		&top_ten[i].sc_score,  &top_ten[i].sc_flags,
		&top_ten[i].sc_level,  &top_ten[i].sc_monster);
	}

    /*
     * Insert her in list if need be
     */
    if (!waswizard)
    {
	for (scp = top_ten; scp <= &top_ten[9]; scp++)
	    if (amount > scp->sc_score)
		break;
	if (scp <= &top_ten[9])
	{
	    for (sc2 = &top_ten[9]; sc2 > scp; sc2--)
		*sc2 = *(sc2-1);
	    scp->sc_score = amount;
	    strcpy(scp->sc_name, whoami);
	    scp->sc_flags = flags;
	    if (flags == 2)
		scp->sc_level = max_level;
	    else
		scp->sc_level = level;
	    scp->sc_monster = monst;
	    strncpy(scp->sc_login, md_getusername(), 8);
	}
    }
    /*
     * Print the list
     */
    if (flags != -1)
	printf("\n\n\n");
    printf("Top Ten Adventurers:\nRank\tScore\tName\n");
    for (scp = top_ten; scp <= &top_ten[9]; scp++) {
	if (scp->sc_score) {
	    printf("%d\t%d\t%s: %s on level %d", scp - top_ten + 1,
		scp->sc_score, scp->sc_name, reason[scp->sc_flags],
		scp->sc_level);
	    if (scp->sc_flags == 0) {
		printf(" by a");
		killer = killname(scp->sc_monster);
		if (*killer == 'a' || *killer == 'e' || *killer == 'i' ||
		    *killer == 'o' || *killer == 'u')
			putchar('n');
		printf(" %s", killer);
	    }
	    if (prflags == 1)
	    {
		printf(" (%s)", scp->sc_login);
		putchar('\n');
	    }
	    else if (prflags == 2)
	    {
		fflush(stdout);
		fgets(prbuf,80,stdin);
		if (prbuf[0] == 'd')
		{
		    for (sc2 = scp; sc2 < &top_ten[9]; sc2++)
			*sc2 = *(sc2 + 1);
		    top_ten[9].sc_score = 0;
		    for (i = 0; i < 80; i++)
			top_ten[9].sc_name[i] = rnd(255);
		    top_ten[9].sc_flags = RN;
		    top_ten[9].sc_level = RN;
		    top_ten[9].sc_monster = RN;
		    scp--;
		}
	    }
	    else
		printf(".\n");
	}
    }

    /*
     * Update the list file
     */

    rewind(outf);

    strcpy(scoreline, "R36 2\n");
    encwrite(scoreline, 100, outf);

    for(i = 0; i < 10; i++)
    {
        encwrite(&top_ten[i].sc_name, 80, outf);
        encwrite(&top_ten[i].sc_login, 8, outf);
        sprintf(scoreline, " %d %d %d %d \n",
            top_ten[i].sc_score,  top_ten[i].sc_flags,
            top_ten[i].sc_level,  top_ten[i].sc_monster);
        encwrite(scoreline, 100, outf);
    }

    md_unlockfile(outf);

    fclose(outf);
}

void 
total_winner()
{
    struct linked_list *item;
    struct object *obj;
    int worth = 0;
    int c;
    int oldpurse;

    clear();
    standout();
    addstr("                                                               \n");
    addstr("  @   @               @   @           @          @@@  @     @  \n");
    addstr("  @   @               @@ @@           @           @   @     @  \n");
    addstr("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
    addstr("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
    addstr("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
    addstr("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
    addstr("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
    addstr("                                                               \n");
    addstr("     Congratulations, you have made it to the light of day!    \n");
    standend();
    addstr("\nYou have joined the elite ranks of those who have escaped the\n");
    addstr("Dungeons of Doom alive.  You journey home and sell all your loot at\n");
    addstr("a great profit and are admitted to the fighters guild.\n");
    mvaddstr(LINES - 1, 0, "--Press space to continue--");
    refresh();
    wait_for(stdscr, ' ');
    clear();
    mvaddstr(0, 0, "   Worth  Item");
    oldpurse = purse;
    for (c = 'a', item = pack; item != NULL; c++, item = next(item))
    {
	obj = (struct object *) ldata(item);
	switch (obj->o_type)
	{
	    case FOOD:
		worth = 2 * obj->o_count;
	    when WEAPON:
		switch (obj->o_which)
		{
		    case MACE: worth = 8;
		    when SWORD: worth = 15;
		    when BOW: worth = 75;
		    when ARROW: worth = 1;
		    when DAGGER: worth = 2;
		    when ROCK: worth = 1;
		    when TWOSWORD: worth = 30;
		    when SLING: worth = 1;
		    when DART: worth = 1;
		    when CROSSBOW: worth = 15;
		    when BOLT: worth = 1;
		    when SPEAR: worth = 2;
		    otherwise: worth = 0;
		}
		worth *= (1 + (10 * obj->o_hplus + 10 * obj->o_dplus));
		worth *= obj->o_count;
		obj->o_flags |= ISKNOW;
	    when ARMOR:
		switch (obj->o_which)
		{
		    case LEATHER: worth = 5;
		    when RING_MAIL: worth = 30;
		    when STUDDED_LEATHER: worth = 15;
		    when SCALE_MAIL: worth = 3;
		    when CHAIN_MAIL: worth = 75;
		    when SPLINT_MAIL: worth = 80;
		    when BANDED_MAIL: worth = 90;
		    when PLATE_MAIL: worth = 400;
		    otherwise: worth = 0;
		}
		if (obj->o_which >= MAXARMORS)
                    break;
		worth *= (1 + (10 * (a_class[obj->o_which] - obj->o_ac)));
		obj->o_flags |= ISKNOW;
	    when SCROLL:
		s_know[obj->o_which] = TRUE;
		worth = s_magic[obj->o_which].mi_worth;
		worth *= obj->o_count;
	    when POTION:
		p_know[obj->o_which] = TRUE;
		worth = p_magic[obj->o_which].mi_worth;
		worth *= obj->o_count;
	    when RING:
		obj->o_flags |= ISKNOW;
		r_know[obj->o_which] = TRUE;
		worth = r_magic[obj->o_which].mi_worth;
		if (obj->o_which == R_ADDSTR || obj->o_which == R_ADDDAM ||
		    obj->o_which == R_PROTECT || obj->o_which == R_ADDHIT)
			if (obj->o_ac > 0)
			    worth += obj->o_ac * 20;
			else
			    worth = 50;
	    when STICK:
		obj->o_flags |= ISKNOW;
		ws_know[obj->o_which] = TRUE;
		worth = ws_magic[obj->o_which].mi_worth;
		worth += 20 * obj->o_charges;
	    when AMULET:
		worth = 1000;
	}
	mvprintw(c - 'a' + 1, 0, "%c) %5d  %s", c, worth, inv_name(obj, FALSE));
	purse += worth;
    }
    mvprintw(c - 'a' + 1, 0,"   %5d  Gold Peices          ", oldpurse);
    refresh();
    score(purse, 2, 0);
    exit(0);
}

char *
killname(int monst)
{
    if (isupper(monst))
	return monsters[monst-'A'].m_name;
    else
	switch (monst)
	{
	    case 'a':
		return "arrow";
	    case 'd':
		return "dart";
	    case 'b':
		return "bolt";
	}
    return("");
}
