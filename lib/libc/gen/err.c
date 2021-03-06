/*
 * This file is formatted with tab stops every 8 columns.
 *
 * $Id: err.c,v 1.2 1997/09/21 06:04:59 gdr Exp $
 */
/*-
 * Copyright (c) 1993
 *      The Regents of the University of California.  All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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

#ifdef __ORCAC__
segment "libc_gen__";
#endif

/* need bit 3 and 6 (minimum) for variadic function definitions */
#pragma optimize 78

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <gno/gno.h>

/*
 * 4.4BSD uses __progname of type char *.  That requires having access
 * to the compiler startup code.  We use __prognameGS() to get us that
 * information from GS/OS.
 */

static FILE *err_file        = NULL; /* file to use for error output */
static void (*err_exit)(int) = NULL;

void err_set_file(void *fp);
void err_set_exit(void (*ef)(int));

void
err_set_file(void *fp)
{
        if (fp) {
                err_file = fp;
        } else {
                err_file = stderr;
        }
}

void
err_set_exit(void (*ef)(int))
{
        err_exit = ef;
}

volatile void
err(int eval, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        verr(eval, fmt, ap);
        va_end(ap);
}

volatile void
verr(int eval, const char *fmt, va_list ap)
{
        int sverrno;

        sverrno = errno;
        if (! err_file) {
                err_set_file((FILE *)0);
        }
        fprintf(err_file, "%s: ", __prognameGS());
        if (fmt != NULL) {
                vfprintf(err_file, fmt, ap);
                fprintf(err_file, ": ");
        }
        fprintf(err_file, "%s\n", strerror(sverrno));
        if(err_exit) {
                err_exit(eval);
        }
        exit(eval);
}

volatile void
errx(int eval, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        verrx(eval, fmt, ap);
        va_end(ap);
}

volatile void
verrx(int eval, const char *fmt, va_list ap)
{
        if (! err_file) {
                err_set_file((FILE *)0);
        }
        fprintf(err_file, "%s: ", __prognameGS());
        if (fmt != NULL) {
                vfprintf(err_file, fmt, ap);
        }
        fprintf(err_file, "\n");
        if (err_exit) {
                err_exit(eval);
        }
        exit(eval);
}

void
warn(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        vwarn(fmt, ap);
        va_end(ap);
}

void
vwarn(const char *fmt, va_list ap)
{
        int sverrno;

        sverrno = errno;
        if (! err_file) {
                err_set_file((FILE *)0);
        }
        fprintf(err_file, "%s: ", __prognameGS());
        if (fmt != NULL) {
                vfprintf(err_file, fmt, ap);
                fprintf(err_file, ": ");
        }
        fprintf(err_file, "%s\n", strerror(sverrno));
}

void
warnx(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        vwarnx(fmt, ap);
        va_end(ap);
}

void
vwarnx(const char *fmt, va_list ap)
{
        if (! err_file) {
                err_set_file((FILE *)0);
        }
        fprintf(err_file, "%s: ", __prognameGS());
        if (fmt != NULL) {
                vfprintf(err_file, fmt, ap);
        }
        fprintf(err_file, "\n");
}
