/* $OpenBSD$ */

/*
 * Copyright (c) 2010 Nicholas Marriott <nicholas.marriott@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include "tmux.h"

#ifndef NO_USE_PANE_BORDER_ACS_ASCII
#include <string.h>

static char tty_acs_table[UCHAR_MAX][4] = {
	['+'] = "\342\206\222",	/* arrow pointing right */
	[','] = "\342\206\220",	/* arrow pointing left */
	['-'] = "\342\206\221",	/* arrow pointing up */
	['.'] = "\342\206\223",	/* arrow pointing down */
	['0'] = "\342\226\256",	/* solid square block */
	['`'] = "\342\227\206",	/* diamond */
	['a'] = "\342\226\222",	/* checker board (stipple) */
	['b'] = "\342\220\211",
	['c'] = "\342\220\214",
	['d'] = "\342\220\215",
	['e'] = "\342\220\212",
	['f'] = "\302\260",	/* degree symbol */
	['g'] = "\302\261",	/* plus/minus */
	['h'] = "\342\220\244",	/* board of squares	ACS_BOARD	*/
	['i'] = "\342\220\213",
	['j'] = "\342\224\230",	/* lower right corner */
	['k'] = "\342\224\220",	/* upper right corner */
	['l'] = "\342\224\214",	/* upper left corner */
	['m'] = "\342\224\224",	/* lower left corner */
	['n'] = "\342\224\274",	/* large plus or crossover */
	['o'] = "\342\216\272",	/* scan line 1 */
	['p'] = "\342\216\273",	/* scan line 3 */
	['q'] = "\342\224\200",	/* horizontal line */
	['r'] = "\342\216\274",	/* scan line 7 */
	['s'] = "\342\216\275",	/* scan line 9 */
	['t'] = "\342\224\234",	/* tee pointing right */
	['u'] = "\342\224\244",	/* tee pointing left */
	['v'] = "\342\224\264",	/* tee pointing up */
	['w'] = "\342\224\254",	/* tee pointing down */
	['x'] = "\342\224\202",	/* vertical line */
	['y'] = "\342\211\244",	/* less-than-or-equal-to */
	['z'] = "\342\211\245",	/* greater-than-or-equal-to */
	['{'] = "\317\200",   	/* greek pi */
	['|'] = "\342\211\240",	/* not-equal */
	['}'] = "\302\243",	/* UK pound sign */
	['~'] = "\302\267"	/* bullet */
};

static char tty_acs_ascii_table[UCHAR_MAX][2] = {
	['}'] = "f",	/* UK pound sign		ACS_STERLING	*/
	['.'] = "v",	/* arrow pointing down		ACS_DARROW	*/
	[','] = "<",	/* arrow pointing left		ACS_LARROW	*/
	['+'] = ">",	/* arrow pointing right		ACS_RARROW	*/
	['-'] = "^",	/* arrow pointing up		ACS_UARROW	*/
	['h'] = "#",	/* board of squares		ACS_BOARD	*/
	['~'] = "o",	/* bullet			ACS_BULLET	*/
	['a'] = ":",	/* checker board (stipple)	ACS_CKBOARD	*/
	['f'] = "\\",	/* degree symbol		ACS_DEGREE	*/
	['`'] = "+",	/* diamond			ACS_DIAMOND	*/
	['z'] = ">",	/* greater-than-or-equal-to	ACS_GEQUAL	*/
	['{'] = "*",	/* greek pi			ACS_PI		*/
	['q'] = "-",	/* horizontal line		ACS_HLINE	*/
	['i'] = "#",	/* lantern symbol		ACS_LANTERN	*/
	['n'] = "+",	/* large plus or crossover	ACS_PLUS	*/
	['y'] = "<",	/* less-than-or-equal-to	ACS_LEQUAL	*/
	['m'] = "+",	/* lower left corner		ACS_LLCORNER	*/
	['j'] = "+",	/* lower right corner		ACS_LRCORNER	*/
	['|'] = "!",	/* not-equal			ACS_NEQUAL	*/
	['g'] = "#",	/* plus/minus			ACS_PLMINUS	*/
	['o'] = "~",	/* scan line 1			ACS_S1		*/
	['p'] = "-",	/* scan line 3			ACS_S3		*/
	['r'] = "-",	/* scan line 7			ACS_S7		*/
	['s'] = "_",	/* scan line 9			ACS_S9		*/
	['0'] = "#",	/* solid square block		ACS_BLOCK	*/
	['w'] = "+",	/* tee pointing down		ACS_TTEE	*/
	['u'] = "+",	/* tee pointing left		ACS_RTEE	*/
	['t'] = "+",	/* tee pointing right		ACS_LTEE	*/
	['v'] = "+",	/* tee pointing up		ACS_BTEE	*/
	['l'] = "+",	/* upper left corner		ACS_ULCORNER	*/
	['k'] = "+",	/* upper right corner		ACS_URCORNER	*/
	['x'] = "|",	/* vertical line		ACS_VLINE	*/
};
#else
static int	tty_acs_cmp(const void *, const void *);

/* Table mapping ACS entries to UTF-8. */
struct tty_acs_entry {
	u_char	 	 key;
	const char	*string;
};
static const struct tty_acs_entry tty_acs_table[] = {
	{ '+', "\342\206\222" },	/* arrow pointing right */
	{ ',', "\342\206\220" },	/* arrow pointing left */
	{ '-', "\342\206\221" },	/* arrow pointing up */
	{ '.', "\342\206\223" },	/* arrow pointing down */
	{ '0', "\342\226\256" },	/* solid square block */
	{ '`', "\342\227\206" },	/* diamond */
	{ 'a', "\342\226\222" },	/* checker board (stipple) */
	{ 'b', "\342\220\211" },
	{ 'c', "\342\220\214" },
	{ 'd', "\342\220\215" },
	{ 'e', "\342\220\212" },
	{ 'f', "\302\260" },		/* degree symbol */
	{ 'g', "\302\261" },		/* plus/minus */
	{ 'h', "\342\220\244" },
	{ 'i', "\342\220\213" },
	{ 'j', "\342\224\230" },	/* lower right corner */
	{ 'k', "\342\224\220" },	/* upper right corner */
	{ 'l', "\342\224\214" },	/* upper left corner */
	{ 'm', "\342\224\224" },	/* lower left corner */
	{ 'n', "\342\224\274" },	/* large plus or crossover */
	{ 'o', "\342\216\272" },	/* scan line 1 */
	{ 'p', "\342\216\273" },	/* scan line 3 */
	{ 'q', "\342\224\200" },	/* horizontal line */
	{ 'r', "\342\216\274" },	/* scan line 7 */
	{ 's', "\342\216\275" },	/* scan line 9 */
	{ 't', "\342\224\234" },	/* tee pointing right */
	{ 'u', "\342\224\244" },	/* tee pointing left */
	{ 'v', "\342\224\264" },	/* tee pointing up */
	{ 'w', "\342\224\254" },	/* tee pointing down */
	{ 'x', "\342\224\202" },	/* vertical line */
	{ 'y', "\342\211\244" },	/* less-than-or-equal-to */
	{ 'z', "\342\211\245" },	/* greater-than-or-equal-to */
	{ '{', "\317\200" },   		/* greek pi */
	{ '|', "\342\211\240" },	/* not-equal */
	{ '}', "\302\243" },		/* UK pound sign */
	{ '~', "\302\267" }		/* bullet */
};
#endif  /* NO_USE_PANE_BORDER_ACS_ASCII */

/* Table mapping UTF-8 to ACS entries. */
struct tty_acs_reverse_entry {
	const char	*string;
	u_char		 key;
};
static const struct tty_acs_reverse_entry tty_acs_reverse2[] = {
	{ "\302\267", '~' }
};
static const struct tty_acs_reverse_entry tty_acs_reverse3[] = {
	{ "\342\224\200", 'q' },
	{ "\342\224\201", 'q' },
	{ "\342\224\202", 'x' },
	{ "\342\224\203", 'x' },
	{ "\342\224\214", 'l' },
	{ "\342\224\217", 'k' },
	{ "\342\224\220", 'k' },
	{ "\342\224\223", 'l' },
	{ "\342\224\224", 'm' },
	{ "\342\224\227", 'm' },
	{ "\342\224\230", 'j' },
	{ "\342\224\233", 'j' },
	{ "\342\224\234", 't' },
	{ "\342\224\243", 't' },
	{ "\342\224\244", 'u' },
	{ "\342\224\253", 'u' },
	{ "\342\224\263", 'w' },
	{ "\342\224\264", 'v' },
	{ "\342\224\273", 'v' },
	{ "\342\224\274", 'n' },
	{ "\342\225\213", 'n' },
	{ "\342\225\220", 'q' },
	{ "\342\225\221", 'x' },
	{ "\342\225\224", 'l' },
	{ "\342\225\227", 'k' },
	{ "\342\225\232", 'm' },
	{ "\342\225\235", 'j' },
	{ "\342\225\240", 't' },
	{ "\342\225\243", 'u' },
	{ "\342\225\246", 'w' },
	{ "\342\225\251", 'v' },
	{ "\342\225\254", 'n' },
};

#ifdef NO_USE_PANE_BORDER_ACS_ASCII
static int
tty_acs_cmp(const void *key, const void *value)
{
	const struct tty_acs_entry	*entry = value;
	int				 test = *(u_char *)key;

	return (test - entry->key);
}
#endif  /* NO_USE_PANE_BORDER_ACS_ASCII */

static int
tty_acs_reverse_cmp(const void *key, const void *value)
{
	const struct tty_acs_reverse_entry	*entry = value;
	const char				*test = key;

	return (strcmp(test, entry->string));
}

#ifndef NO_USE_PANE_BORDER_ACS_ASCII
static int
get_utf8_width(const char *s)
{
	const char		*p = s;
	struct utf8_data	 ud;
	enum utf8_state		 more;

	for (more = utf8_open(&ud, *p++); more == UTF8_MORE; more = utf8_append(&ud, *p++))
		;
	if (more != UTF8_DONE)
		fatalx("INTERNAL ERROR: In get_utf8_width, utf8_open or utf8_append return error %d", more);
	log_debug("%s width is %d", s, ud.width);
	return ud.width;
}

enum acs_type {
	ACST_UTF8,
	ACST_ACS,
	ACST_ASCII,
};

static enum acs_type
tty_acs_type(struct tty *tty)
{
	if (tty == NULL)
		return (ACST_ASCII);

	/*
	 * If the U8 flag is present, it marks whether a terminal supports
	 * UTF-8 and ACS together.
	 *
	 * If it is present and zero, we force ACS - this gives users a way to
	 * turn off UTF-8 line drawing.
	 *
	 * If it is nonzero, we can fall through to the default and use UTF-8
	 * line drawing on UTF-8 terminals.
	 */

	struct environ_entry	*envent;
	envent = environ_find(tty->client->environ, "TMUX_ACS");
	if (envent != NULL) {
		if (strcasestr(envent->value, "utf-8") != NULL ||
		    strcasestr(envent->value, "utf8") != NULL)
			return (ACST_UTF8);
		else if (strcasestr(envent->value, "acs") != NULL)
			return (ACST_ACS);
		else
			return (ACST_ASCII);
	}

	if (options_get_number(global_s_options, "pane-border-acs"))
		return (ACST_ACS);
	if (options_get_number(global_s_options, "pane-border-ascii"))
		return (ACST_ASCII);

	if ((tty->client->flags & CLIENT_UTF8) &&
	    (!tty_term_has(tty->term, TTYC_U8) ||
	     tty_term_number(tty->term, TTYC_U8) != 0)) {
		static int hline_width = 0;
		const char *hline = "\342\224\200";
		if (hline_width == 0) {
			hline_width = get_utf8_width(hline);
			log_debug("hline_width=%d", hline_width);
		}
		if (hline_width == 1)
			return (ACST_UTF8);
	}

	if (tty_term_has(tty->term, TTYC_ACSC))
		return (ACST_ACS);

	return (ACST_ASCII);
}
#endif /* NO_USE_PANE_BORDER_ACS_ASCII */

/* Should this terminal use ACS instead of UTF-8 line drawing? */
int
tty_acs_needed(struct tty *tty)
{
#ifndef NO_USE_PANE_BORDER_ACS_ASCII
	return (tty_acs_type(tty) == ACST_ACS);
#else
	if (tty == NULL)
		return (0);

	/*
	 * If the U8 flag is present, it marks whether a terminal supports
	 * UTF-8 and ACS together.
	 *
	 * If it is present and zero, we force ACS - this gives users a way to
	 * turn off UTF-8 line drawing.
	 *
	 * If it is nonzero, we can fall through to the default and use UTF-8
	 * line drawing on UTF-8 terminals.
	 */
	if (tty_term_has(tty->term, TTYC_U8) &&
	    tty_term_number(tty->term, TTYC_U8) == 0)
		return (1);

	if (tty->client->flags & CLIENT_UTF8)
		return (0);
	return (1);
#endif /* NO_USE_PANE_BORDER_ACS_ASCII */
}

/* Retrieve ACS to output as UTF-8. */
const char *
tty_acs_get(struct tty *tty, u_char ch)
{
#ifndef NO_USE_PANE_BORDER_ACS_ASCII
	switch (tty_acs_type(tty)) {
	case ACST_UTF8:
		if (tty_acs_table[ch][0] != '\0')
			return (&tty_acs_table[ch][0]);
		break;
	case ACST_ACS:
		if (tty->term->acs[ch][0] != '\0')
			return (&tty->term->acs[ch][0]);
		break;
	case ACST_ASCII:
		break;
	}

	if (tty_acs_ascii_table[ch][0] != '\0')
		return (&tty_acs_ascii_table[ch][0]);
	return (NULL);
#else
    const struct tty_acs_entry  *entry;

	/* Use the ACS set instead of UTF-8 if needed. */
	if (tty_acs_needed(tty)) {
		if (tty->term->acs[ch][0] == '\0')
			return (NULL);
		return (&tty->term->acs[ch][0]);
	}

	/* Otherwise look up the UTF-8 translation. */
	entry = bsearch(&ch, tty_acs_table, nitems(tty_acs_table),
	    sizeof tty_acs_table[0], tty_acs_cmp);
	if (entry == NULL)
		return (NULL);
	return (entry->string);
#endif /* NO_USE_PANE_BORDER_ACS_ASCII */
}

/* Reverse UTF-8 into ACS. */
int
tty_acs_reverse_get(__unused struct tty *tty, const char *s, size_t slen)
{
	const struct tty_acs_reverse_entry	*table, *entry;
	u_int					 items;

	if (slen == 2) {
		table = tty_acs_reverse2;
		items = nitems(tty_acs_reverse2);
	} else if (slen == 3) {
		table = tty_acs_reverse3;
		items = nitems(tty_acs_reverse3);
	} else
		return (-1);
	entry = bsearch(s, table, items, sizeof table[0], tty_acs_reverse_cmp);
	if (entry == NULL)
		return (-1);
	return (entry->key);
}
