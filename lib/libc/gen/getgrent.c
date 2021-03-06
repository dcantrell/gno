/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * This file is formatted with tab stops every 8 columns.
 *
 * $Id: getgrent.c,v 1.2 1997/09/21 06:05:00 gdr Exp $
 */
 
#ifdef __ORCAC__
segment "libc_gen__";
#endif

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getgrent.c	8.2 (Berkeley) 3/21/94";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <grp.h>

static FILE *_gr_fp;
static struct group _gr_group;
static int _gr_stayopen;
static int grscan(register int search, register int gid, 
		  register const char *name);
static int start_gr(void);
#ifdef YP
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
static int _gr_stepping_yp;
static int _gr_yp_enabled;
static int _getypgroup(struct group *, const char *, char *);
static int _nextypgroup(struct group *);
#endif

#define	MAXGRP		200
static char *members[MAXGRP];
#define	MAXLINELENGTH	1024
static char line[MAXLINELENGTH];

struct group *
getgrent(void)
{
	if (!_gr_fp && !start_gr()) {
		return NULL;
	}

#ifdef YP
	if (_gr_stepping_yp) {
		if (_nextypgroup(&_gr_group))
			return(&_gr_group);
	}
tryagain:
#endif

	if (!grscan(0, 0, NULL))
		return(NULL);
#ifdef YP
	if(_gr_group.gr_name[0] == '+' && _gr_group.gr_name[1]) {
		_getypgroup(&_gr_group, &_gr_group.gr_name[1],
			    "group.byname");
	} else if(_gr_group.gr_name[0] == '+') {
		if (!_nextypgroup(&_gr_group))
			goto tryagain;
		else
			return(&_gr_group);
	}
#endif
	return(&_gr_group);
}

struct group *
getgrnam(const char *name)
{
	int rval;

	if (!start_gr())
		return(NULL);
#ifdef YP
	tryagain:
#endif
	rval = grscan(1, 0, name);
#ifdef YP
	if(rval == -1 && (_gr_yp_enabled < 0 || (_gr_yp_enabled &&
					_gr_group.gr_name[0] == '+'))) {
		if (!(rval = _getypgroup(&_gr_group, name, "group.byname")))
			goto tryagain;
	}
#endif
	if (!_gr_stayopen)
		endgrent();
	return(rval ? &_gr_group : NULL);
}

struct group *
#ifdef __STDC__
getgrgid(gid_t gid)
#else
getgrgid(gid)
	gid_t gid;
#endif
{
	int rval;

	if (!start_gr())
		return(NULL);
#ifdef YP
	tryagain:
#endif
	rval = grscan(1, gid, NULL);
#ifdef YP
	if(rval == -1 && _gr_yp_enabled) {
		char buf[16];
		snprintf(buf, sizeof buf, "%d", (unsigned)gid);
		if (!(rval = _getypgroup(&_gr_group, buf, "group.bygid")))
			goto tryagain;
	}
#endif
	if (!_gr_stayopen)
		endgrent();
	return(rval ? &_gr_group : NULL);
}

static int
start_gr(void)
{
	if (_gr_fp) {
		rewind(_gr_fp);
		return(1);
	}
	_gr_fp = fopen(_PATH_GROUP, "r");
	if(!_gr_fp) return 0;
#ifdef YP
	/*
	 * This is a disgusting hack, used to determine when YP is enabled.
	 * This would be easier if we had a group database to go along with
	 * the password database.
	 */
	{
		char *line;
		size_t linelen;
		_gr_yp_enabled = 0;
		while((line = fgetln(_gr_fp, &linelen)) != NULL) {
			if(line[0] == '+') {
				if(line[1] && !_gr_yp_enabled) {
					_gr_yp_enabled = 1;
				} else {
					_gr_yp_enabled = -1;
					break;
				}
			}
		}
		rewind(_gr_fp);
	}
#endif
	return 1;
}

int
setgrent(void)
{
	return(setgroupent(0));
}

int
setgroupent(int stayopen)
{
	if (!start_gr())
		return(0);
	_gr_stayopen = stayopen;
#ifdef YP
	_gr_stepping_yp = 0;
#endif
	return(1);
}

void
endgrent(void)
{
#ifdef YP
	_gr_stepping_yp = 0;
#endif
	if (_gr_fp) {
		(void)fclose(_gr_fp);
		_gr_fp = NULL;
	}
}

static int
grscan(register int search, register int gid, register const char *name)
{
	register char *cp, **m;
	char *bp;
#ifdef YP
	int _ypfound = 0;
#endif
	for (;;) {
		if (!fgets(line, sizeof(line), _gr_fp))
			return(0);
		bp = line;
		/* skip lines that are too big */
		if (!index(line, '\n')) {
			int ch;

			while ((ch = getc(_gr_fp)) != '\n' && ch != EOF)
				;
			continue;
		}
		if ((_gr_group.gr_name = strsep(&bp, ":\n")) == NULL)
			break;
#ifdef YP
		/*
		 * XXX   We need to be careful to avoid proceeding
		 * past this point under certain circumstances or
		 * we risk dereferencing null pointers down below.
		 */
		if (_gr_group.gr_name[0] == '+') {
			if (strlen(_gr_group.gr_name) == 1) {
				switch(search) {
				case 0:
					return(1);
				case 1:
					return(-1);
				default:
					return(0);
				}
			} else {
				if (!_getypgroup(&_gr_group, &_gr_group.gr_name[1],
						"group.byname"))
					continue;
			/* We're going to override -- tell the world. */
				members[0] = NULL;
				_ypfound++;
			}
		}
#else
		if (_gr_group.gr_name[0] == '+')
			continue;
#endif /* YP */
		if (search && name) {
			if(strcmp(_gr_group.gr_name, name)) {
				continue;
			}
		}
		if ((_gr_group.gr_passwd = strsep(&bp, ":\n")) == NULL)
			break;;
		if (!(cp = strsep(&bp, ":\n")))
			continue;
#ifdef YP
		if (!_ypfound)
#endif
		_gr_group.gr_gid = atoi(cp);
		if (search && name == NULL && _gr_group.gr_gid != gid)
			continue;
		cp = NULL;
		for (m = _gr_group.gr_mem = members;; bp++) {
			if (m == &members[MAXGRP - 1])
				break;
			if (*bp == ',') {
				if (cp) {
					*bp = '\0';
					*m++ = cp;
					cp = NULL;
				}
			} else if (*bp == '\0' || *bp == '\n' || *bp == ' ') {
				if (cp) {
					*bp = '\0';
					*m++ = cp;
			}
				break;
			} else if (cp == NULL)
				cp = bp;
		}
		*m = NULL;
		return(1);
	}
	/* NOTREACHED */
	return (0);
}

#ifdef YP

static int
_gr_breakout_yp(struct group *gr, char *result)
{
	char *s, *cp;
	char **m;

	/*
	 * XXX If 's' ends up being a NULL pointer, punt on this group.
	 * It means the NIS group entry is badly formatted and should
	 * be skipped.
	 */
	if ((s = strsep(&result, ":")) == NULL) return 0; /* name */
	gr->gr_name = s;

	if ((s = strsep(&result, ":")) == NULL) return 0; /* password */
	gr->gr_passwd = s;

	if ((s = strsep(&result, ":")) == NULL) return 0; /* gid */
	gr->gr_gid = atoi(s);

	if ((s = result) == NULL) return 0;
	cp = 0;

	for (m = _gr_group.gr_mem = members; /**/; s++) {
		if (m == &members[MAXGRP - 1]) {
			break;
		}
		if (*s == ',') {
			if (cp) {
				*s = '\0';
				*m++ = cp;
				cp = NULL;
			}
		} else if (*s == '\0' || *s == '\n' || *s == ' ') {
			if (cp) {
				*s = '\0';
				*m++ = cp;
			}
			break;
		} else if (cp == NULL) {
			cp = s;
		}
	}
	*m = NULL;

	return 1;
}

static char *_gr_yp_domain;

static int
_getypgroup(struct group *gr, const char *name, char *map)
{
	char *result, *s;
	static char resultbuf[1024];
	int resultlen;

	if(!_gr_yp_domain) {
		if(yp_get_default_domain(&_gr_yp_domain))
		  return 0;
	}

	if(yp_match(_gr_yp_domain, map, name, strlen(name),
		    &result, &resultlen))
		return 0;

	s = strchr(result, '\n');
	if(s) *s = '\0';

	if(resultlen >= sizeof resultbuf) return 0;
	strcpy(resultbuf, result);
	result = resultbuf;
	return(_gr_breakout_yp(gr, resultbuf));

}


static int
_nextypgroup(struct group *gr)
{
	static char *key;
	static int keylen;
	char *lastkey, *result;
	static char resultbuf[1024];
	int resultlen;
	int rv;

	if(!_gr_yp_domain) {
		if(yp_get_default_domain(&_gr_yp_domain))
		  return 0;
	}

	if(!_gr_stepping_yp) {
		if(key) free(key);
		rv = yp_first(_gr_yp_domain, "group.byname",
			      &key, &keylen, &result, &resultlen);
		if(rv) {
			return 0;
		}
		_gr_stepping_yp = 1;
		goto unpack;
	} else {
tryagain:
		lastkey = key;
		rv = yp_next(_gr_yp_domain, "group.byname", key, keylen,
			     &key, &keylen, &result, &resultlen);
		free(lastkey);
unpack:
		if(rv) {
			_gr_stepping_yp = 0;
			return 0;
		}

		if(resultlen > sizeof(resultbuf)) {
			free(result);
			goto tryagain;
		}

		strcpy(resultbuf, result);
		free(result);
		if((result = strchr(resultbuf, '\n')) != NULL)
			*result = '\0';
		if (_gr_breakout_yp(gr, resultbuf))
			return(1);
		else
			goto tryagain;
	}
}

#endif /* YP */
