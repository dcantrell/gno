#
# Makefile for nroff(1).
#
# $Id: makefile.mk,v 1.1 1997/03/14 06:22:28 gdr Exp $
#

IIGS		= FALSE	# TRUE or FALSE
USE_INSIGHT	= FALSE # TRUE or FALSE

BINDIR		= /usr/bin
TMACDIR		= /usr/lib/tmac
MANDIR		= /usr/man

#
###### end of configuration
#

PROFILE		= # -pg

.IF $(IIGS) == TRUE
DEFINES		=
OPTIMIZE	=
LDLIBS		=
.ELIF $(USE_INSIGHT) == TRUE
CC		= insight 
DEFINES		=
OPTIMIZE	= -g
LDLIBS		= int.tqs -ltermcap
.ELSE
CC		= gcc
DEFINES		= # -DDEBUG
OPTIMIZE	= $(PROFILE) -g
LDLIBS		= $(PROFILE) -ltermcap
.END

.IF $(IIGS) == TRUE
UX_SRC		=
UX_OBJ		=
.ELSE
UX_SRC		= err.c
UX_OBJ		= err.o
.END

GCC_PARANOIA	= \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Waggregate-return \
	-Wnested-externs \
	-fno-asm -fno-builtin -fno-inline

.IF $(CC) == gcc
GCC_FLAGS	= -Wall -funsigned-char $(GCC_PARANOIA)
.ELSE
GCC_FLAGS	=
.END


CFLAGS	= $(OPTIMIZE) $(GCC_FLAGS) $(DEFINES)

SUNOS_H	= sunos.h
OBJS	= main.o command.o escape.o io.o low.o macros.o strings.o \
	  text.o $(UX_OBJ)
SRCS	= main.c command.c escape.c io.c low.c macros.c strings.c \
	  text.c $(UX_SRC)

default: nroff

nroff: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LDLIBS)

clobber:
	$(RM) -f nroff $(OBJS) 
#	$(RM) -f nroff $(OBJS) *~ core tca.* *.tqs insight.log

dep:
	gcc -MM $(CFLAGS) *.c > depend

callchart:
	cflow $(SRCS) > $@

install:
	$(INSTALL) -d $(BINDIR) $(TMACDIR) $(MANDIR)/man1 $(MANDIR)/man7
	$(INSTALL) -m755 nroff $(BINDIR)
	$(INSTALL) -m644 tmac.an $(TMACDIR)
	$(INSTALL) -m644 tmac.s $(TMACDIR)
	$(INSTALL) -m644 nroff.1 $(TMACDIR)/man1
	$(INSTALL) -m644 man.7 $(TMACDIR)/man7
	$(INSTALL) -m644 ms.7 $(TMACDIR)/man7

#
# additional dependancies
#
command.o::	$(SUNOS_H) nroff.h config.h
escape.o::	$(SUNOS_H) nroff.h config.h
io.o::	 	$(SUNOS_H) nroff.h config.h macros.h io.h
low.o::	 	$(SUNOS_H) nroff.h config.h
macros.o::	$(SUNOS_H) nroff.h config.h macros.h
main.o::	$(SUNOS_H) nroff.h config.h macros.h
strings.o::	$(SUNOS_H) nroff.h config.h
text.o::	$(SUNOS_H) nroff.h config.h io.h
