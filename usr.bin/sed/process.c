/*-
 * Copyright (c) 1992 Diomidis Spinellis.
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Diomidis Spinellis of Imperial College, University of London.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/*
 * Modified for GNO (Apple IIGS) by Dave Tribby, October 1997
 *
 * Constructs unacceptable to compiler are replaced using #ifndef __ORCAC__
 *
 * Changes not related to compiler are replaced using #ifndef __GNO__
 *
 * Added prototyped headers, surrounded by #ifndef __STDC__
 */

#ifdef __ORCAC__
segment "process";
#endif

#ifndef __GNO__
#ifndef lint
#if 0
static char sccsid[] = "@(#)process.c	8.6 (Berkeley) 4/20/94";
#endif
static const char rcsid[] =
	"$Id: process.c,v 1.1 1997/11/17 04:26:54 gdr Exp $";
#endif /* not lint */
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/uio.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defs.h"
#include "extern.h"

static SPACE HS, PS, SS;
#define	pd		PS.deleted
#define	ps		PS.space
#define	psl		PS.len
#define	hs		HS.space
#define	hsl		HS.len

#ifndef __ORCAC__
static inline int	 applies __P((struct s_command *));
#else
static int		 applies __P((struct s_command *));
#endif
static void		 flush_appends __P((void));
static void		 lputs __P((char *));
#ifndef __ORCAC__
static inline int	 regexec_e __P((regex_t *, const char *, int, int, size_t));
#else
static int		 regexec_e __P((regex_t *, const char *, int, int, size_t));
#endif
static void		 regsub __P((SPACE *, char *, char *));
static int		 substitute __P((struct s_command *));

struct s_appends *appends;	/* Array of pointers to strings to append. */
static int appendx;		/* Index into appends array. */
#ifndef __GNO__
int appendnum;			/* Size of appends array. */
#else
/* >>> How can this work without initialization? (DMT) */
int appendnum=1;		/* Size of appends array. */
#endif

static int lastaddr;		/* Set by applies if last address of a range. */
static int sdone;		/* If any substitutes since last line input. */
				/* Iov structure for 'w' commands. */
static regex_t *defpreg;
size_t maxnsub;
regmatch_t *match;

#define OUT(s) { fwrite(s, sizeof(u_char), psl, stdout); }

void
#ifndef __STDC__
process()
#else
process(void)
#endif
{
	struct s_command *cp;
	SPACE tspace;
	size_t len, oldpsl;
	char *p;

	for (linenum = 0; mf_fgets(&PS, REPLACE);) {
		pd = 0;
top:
		cp = prog;
redirect:
		while (cp != NULL) {
			if (!applies(cp)) {
				cp = cp->next;
				continue;
			}
			switch (cp->code) {
			case '{':
				cp = cp->u.c;
				goto redirect;
			case 'a':
				if (appendx >= appendnum)
					appends = xrealloc(appends,
					    sizeof(struct s_appends) *
					    (appendnum *= 2));
				appends[appendx].type = AP_STRING;
				appends[appendx].s = cp->t;
				appends[appendx].len = strlen(cp->t);
				appendx++;
				break;
			case 'b':
				cp = cp->u.c;
				goto redirect;
			case 'c':
				pd = 1;
				psl = 0;
				if (cp->a2 == NULL || lastaddr)
					(void)printf("%s", cp->t);
				break;
			case 'd':
				pd = 1;
				goto new;
			case 'D':
				if (pd)
					goto new;
				if ((p = memchr(ps, '\n', psl - 1)) == NULL) {
					pd = 1;
					goto new;
				} else {
					psl -= (p + 1) - ps;
					memmove(ps, p + 1, psl);
					goto top;
				}
			case 'g':
				cspace(&PS, hs, hsl, REPLACE);
				break;
			case 'G':
				cspace(&PS, hs, hsl, 0);
				break;
			case 'h':
				cspace(&HS, ps, psl, REPLACE);
				break;
			case 'H':
				cspace(&HS, ps, psl, 0);
				break;
			case 'i':
				(void)printf("%s", cp->t);
				break;
			case 'l':
				lputs(ps);
				break;
			case 'n':
				if (!nflag && !pd)
					OUT(ps)
				flush_appends();
				if (!mf_fgets(&PS, REPLACE))
					exit(0);
				pd = 0;
				break;
			case 'N':
				flush_appends();
				if (!mf_fgets(&PS, 0)) {
					if (!nflag && !pd)
						OUT(ps)
					exit(0);
				}
				break;
			case 'p':
				if (pd)
					break;
				OUT(ps)
				break;
			case 'P':
				if (pd)
					break;
				if ((p = memchr(ps, '\n', psl - 1)) != NULL) {
					oldpsl = psl;
					psl = (p + 1) - ps;
				}
				OUT(ps)
				if (p != NULL)
					psl = oldpsl;
				break;
			case 'q':
				if (!nflag && !pd)
					OUT(ps)
				flush_appends();
				exit(0);
			case 'r':
				if (appendx >= appendnum)
					appends = xrealloc(appends,
					    sizeof(struct s_appends) *
					    (appendnum *= 2));
				appends[appendx].type = AP_FILE;
				appends[appendx].s = cp->t;
				appends[appendx].len = strlen(cp->t);
				appendx++;
				break;
			case 's':
				sdone |= substitute(cp);
				break;
			case 't':
				if (sdone) {
					sdone = 0;
					cp = cp->u.c;
					goto redirect;
				}
				break;
			case 'w':
				if (pd)
					break;
				if (cp->u.fd == -1 && (cp->u.fd = open(cp->t,
#ifndef __GNO__
				    O_WRONLY|O_APPEND|O_CREAT|O_TRUNC,
#else			/* GNO's open needs CR/LF translation (O_TRANS) mode */
				    O_WRONLY|O_APPEND|O_CREAT|O_TRUNC|O_TRANS,
#endif
				    DEFFILEMODE)) == -1)
					err(1, "%s", cp->t);
				if (write(cp->u.fd, ps, psl) != psl)
					err(1, "%s", cp->t);
				break;
			case 'x':
				if (hs == NULL)
					cspace(&HS, "\n", 1, REPLACE);
				tspace = PS;
				PS = HS;
				HS = tspace;
				break;
			case 'y':
				if (pd)
					break;
				for (p = ps, len = psl; --len; ++p)
					*p = cp->u.y[*p];
				break;
			case ':':
			case '}':
				break;
			case '=':
				(void)printf("%lu\n", linenum);
			}
			cp = cp->next;
		} /* for all cp */

new:		if (!nflag && !pd)
			OUT(ps)
		flush_appends();
	} /* for all lines */
}

/*
 * TRUE if the address passed matches the current program state
 * (lastline, linenumber, ps).
 */
#define	MATCH(a)						\
	(a)->type == AT_RE ? regexec_e((a)->u.r, ps, 0, 1, psl) :	\
	    (a)->type == AT_LINE ? linenum == (a)->u.l : lastline

/*
 * Return TRUE if the command applies to the current line.  Sets the inrange
 * flag to process ranges.  Interprets the non-select (``!'') flag.
 */
#ifndef __ORCAC__
static inline int
#else
static int
#endif
#ifndef __STDC__
applies(cp)
	struct s_command *cp;
#else
applies(struct s_command *cp)
#endif
{
	int r;

	lastaddr = 0;
	if (cp->a1 == NULL && cp->a2 == NULL)
		r = 1;
	else if (cp->a2)
		if (cp->inrange) {
			if (MATCH(cp->a2)) {
				cp->inrange = 0;
				lastaddr = 1;
			}
			r = 1;
		} else if (MATCH(cp->a1)) {
			/*
			 * If the second address is a number less than or
			 * equal to the line number first selected, only
			 * one line shall be selected.
			 *	-- POSIX 1003.2
			 */
			if (cp->a2->type == AT_LINE &&
			    linenum >= cp->a2->u.l)
				lastaddr = 1;
			else
				cp->inrange = 1;
			r = 1;
		} else
			r = 0;
	else
		r = MATCH(cp->a1);
	return (cp->nonsel ? ! r : r);
}

/*
 * substitute --
 *	Do substitutions in the pattern space.  Currently, we build a
 *	copy of the new pattern space in the substitute space structure
 *	and then swap them.
 */
static int
#ifndef __STDC__
substitute(cp)
	struct s_command *cp;
#else
substitute(struct s_command *cp)
#endif
{
	SPACE tspace;
	regex_t *re;
	size_t re_off, slen;
	int lastempty, n;
	char *s;

	s = ps;
	re = cp->u.s->re;
	if (re == NULL) {
		if (defpreg != NULL && cp->u.s->maxbref > defpreg->re_nsub) {
			linenum = cp->u.s->linenum;
			errx(1, "%lu: %s: \\%d not defined in the RE",
					linenum, fname, cp->u.s->maxbref);
		}
	}
	if (!regexec_e(re, s, 0, 0, psl))
		return (0);

  	SS.len = 0;				/* Clean substitute space. */
  	slen = psl;
  	n = cp->u.s->n;
	lastempty = 1;

  	switch (n) {
  	case 0:					/* Global */
  		do {
			if (lastempty || match[0].rm_so != match[0].rm_eo) {
				/* Locate start of replaced string. */
				re_off = match[0].rm_so;
				/* Copy leading retained string. */
				cspace(&SS, s, re_off, APPEND);
				/* Add in regular expression. */
				regsub(&SS, s, cp->u.s->new);
			}

			/* Move past this match. */
			if (match[0].rm_so != match[0].rm_eo) {
				s += match[0].rm_eo;
				slen -= match[0].rm_eo;
				lastempty = 0;
			} else {
				if (match[0].rm_so == 0)
					cspace(&SS, s, match[0].rm_so + 1,
					    APPEND);
				else
					cspace(&SS, s + match[0].rm_so, 1,
					    APPEND);
				s += match[0].rm_so + 1;
				slen -= match[0].rm_so + 1;
				lastempty = 1;
			}
		} while (slen > 0 && regexec_e(re, s, REG_NOTBOL, 0, slen));
		/* Copy trailing retained string. */
		if (slen > 0)
			cspace(&SS, s, slen, APPEND);
  		break;
	default:				/* Nth occurrence */
		while (--n) {
			s += match[0].rm_eo;
			slen -= match[0].rm_eo;
			if (!regexec_e(re, s, REG_NOTBOL, 0, slen))
				return (0);
		}
		/* FALLTHROUGH */
	case 1:					/* 1st occurrence */
		/* Locate start of replaced string. */
		re_off = match[0].rm_so + (s - ps);
		/* Copy leading retained string. */
		cspace(&SS, ps, re_off, APPEND);
		/* Add in regular expression. */
		regsub(&SS, s, cp->u.s->new);
		/* Copy trailing retained string. */
		s += match[0].rm_eo;
		slen -= match[0].rm_eo;
		cspace(&SS, s, slen, APPEND);
		break;
	}

	/*
	 * Swap the substitute space and the pattern space, and make sure
	 * that any leftover pointers into stdio memory get lost.
	 */
	tspace = PS;
	PS = SS;
	SS = tspace;
	SS.space = SS.back;

	/* Handle the 'p' flag. */
	if (cp->u.s->p)
		OUT(ps)

	/* Handle the 'w' flag. */
	if (cp->u.s->wfile && !pd) {
		if (cp->u.s->wfd == -1 && (cp->u.s->wfd = open(cp->u.s->wfile,
#ifndef __GNO__
		    O_WRONLY|O_APPEND|O_CREAT|O_TRUNC, DEFFILEMODE)) == -1)
#else		/* GNO's open needs CR/LF translation (O_TRANS) mode */
		    O_WRONLY|O_APPEND|O_CREAT|O_TRUNC|O_TRANS, DEFFILEMODE)) == -1)
#endif
			err(1, "%s", cp->u.s->wfile);
		if (write(cp->u.s->wfd, ps, psl) != psl)
			err(1, "%s", cp->u.s->wfile);
	}
	return (1);
}

/*
 * Flush append requests.  Always called before reading a line,
 * therefore it also resets the substitution done (sdone) flag.
 */
static void
#ifndef __STDC__
flush_appends()
#else
flush_appends(void)
#endif
{
	FILE *f;
	int count, i;
#ifndef __GNO__
	char buf[8 * 1024];
#else
	static char buf[8 * 1024];
#endif

	for (i = 0; i < appendx; i++)
		switch (appends[i].type) {
		case AP_STRING:
			fwrite(appends[i].s, sizeof(char), appends[i].len,
			    stdout);
			break;
		case AP_FILE:
			/*
			 * Read files probably shouldn't be cached.  Since
			 * it's not an error to read a non-existent file,
			 * it's possible that another program is interacting
			 * with the sed script through the file system.  It
			 * would be truly bizarre, but possible.  It's probably
			 * not that big a performance win, anyhow.
			 */
			if ((f = fopen(appends[i].s, "r")) == NULL)
				break;
			while ((count = fread(buf, sizeof(char), sizeof(buf), f)))
				(void)fwrite(buf, sizeof(char), count, stdout);
			(void)fclose(f);
			break;
		}
	if (ferror(stdout))
		errx(1, "stdout: %s", strerror(errno ? errno : EIO));
	appendx = sdone = 0;
}

static void
#ifndef __STDC__
lputs(s)
	register char *s;
#else
lputs(register char *s)
#endif
{
	register int count;
	register char *escapes, *p;
	struct winsize win;
	static int termwidth = -1;

	if (termwidth == -1)
		if ((p = getenv("COLUMNS")))
			termwidth = atoi(p);
		else if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0 &&
		    win.ws_col > 0)
			termwidth = win.ws_col;
		else
			termwidth = 60;

	for (count = 0; *s; ++s) {
		if (count >= termwidth) {
			(void)printf("\\\n");
			count = 0;
		}
		if (isprint((unsigned char)*s) && *s != '\\') {
			(void)putchar(*s);
			count++;
		} else {
			escapes = "\\\a\b\f\n\r\t\v";
			(void)putchar('\\');
			if ((p = strchr(escapes, *s))) {
				(void)putchar("\\abfnrtv"[p - escapes]);
				count += 2;
			} else {
				(void)printf("%03o", *(u_char *)s);
				count += 4;
			}
		}
	}
	(void)putchar('$');
	(void)putchar('\n');
	if (ferror(stdout))
		errx(1, "stdout: %s", strerror(errno ? errno : EIO));
}

#ifndef __ORCAC__
static inline int
#else
static int
#endif
#ifndef __STDC__
regexec_e(preg, string, eflags, nomatch, slen)
	regex_t *preg;
	const char *string;
	int eflags, nomatch;
	size_t slen;
#else
regexec_e(regex_t *preg,
	const char *string,
	int eflags,
	int nomatch,
	size_t slen)
#endif
{
	int eval;

	if (preg == NULL) {
		if (defpreg == NULL)
			errx(1, "first RE may not be empty");
	} else
		defpreg = preg;

	/* Set anchors, discounting trailing newline (if any). */
	if (slen > 0 && string[slen - 1] == '\n')
		slen--;
	match[0].rm_so = 0;
	match[0].rm_eo = slen;

	eval = regexec(defpreg, string,
	    nomatch ? 0 : maxnsub + 1, match, eflags | REG_STARTEND);
	switch(eval) {
	case 0:
		return (1);
	case REG_NOMATCH:
		return (0);
	}
	errx(1, "RE error: %s", strregerror(eval, defpreg));
	/* NOTREACHED */
}

/*
 * regsub - perform substitutions after a regexp match
 * Based on a routine by Henry Spencer
 */
static void
#ifndef __STDC__
regsub(sp, string, src)
	SPACE *sp;
	char *string, *src;
#else
regsub(	SPACE *sp,
	char *string,
	char *src)
#endif
{
	register int len, no;
	register char c, *dst;

#define	NEEDSP(reqlen)							\
	if (sp->len >= sp->blen - (reqlen) - 1) {			\
		sp->blen += (reqlen) + 1024;				\
		sp->space = sp->back = xrealloc(sp->back, sp->blen);	\
		dst = sp->space + sp->len;				\
	}

	dst = sp->space + sp->len;
	while ((c = *src++) != '\0') {
		if (c == '&')
			no = 0;
		else if (c == '\\' && isdigit((unsigned char)*src))
			no = *src++ - '0';
		else
			no = -1;
		if (no < 0) {		/* Ordinary character. */
 			if (c == '\\' && (*src == '\\' || *src == '&'))
 				c = *src++;
			NEEDSP(1);
 			*dst++ = c;
			++sp->len;
 		} else if (match[no].rm_so != -1 && match[no].rm_eo != -1) {
			len = match[no].rm_eo - match[no].rm_so;
			NEEDSP(len);
			memmove(dst, string + match[no].rm_so, len);
			dst += len;
			sp->len += len;
		}
	}
	NEEDSP(1);
	*dst = '\0';
}

/*
 * aspace --
 *	Append the source space to the destination space, allocating new
 *	space as necessary.
 */
void
#ifndef __STDC__
cspace(sp, p, len, spflag)
	SPACE *sp;
	char *p;
	size_t len;
	enum e_spflag spflag;
#else
cspace(	SPACE *sp,
	char *p,
	size_t len,
	enum e_spflag spflag)
#endif
{
	size_t tlen;

	/* Make sure SPACE has enough memory and ramp up quickly. */
	tlen = sp->len + len + 1;
	if (tlen > sp->blen) {
		sp->blen = tlen + 1024;
		sp->space = sp->back = xrealloc(sp->back, sp->blen);
	}

	if (spflag == REPLACE)
		sp->len = 0;

	memmove(sp->space + sp->len, p, len);

	sp->space[sp->len += len] = '\0';
}

/*
 * Close all cached opened files and report any errors
 */
void
#ifndef __STDC__
cfclose(cp, end)
	register struct s_command *cp, *end;
#else
cfclose(register struct s_command *cp,
	register struct s_command *end)
#endif
{

	for (; cp != end; cp = cp->next)
		switch(cp->code) {
		case 's':
			if (cp->u.s->wfd != -1 && close(cp->u.s->wfd))
				err(1, "%s", cp->u.s->wfile);
			cp->u.s->wfd = -1;
			break;
		case 'w':
			if (cp->u.fd != -1 && close(cp->u.fd))
				err(1, "%s", cp->t);
			cp->u.fd = -1;
			break;
		case '{':
			cfclose(cp->u.c, cp->next);
			break;
		}
}
