# vanilla-Makefile
#
# This makefile is configured for use with gcc under UNIX for
# a development compile. Uncomment the relavant portion and
# comment the development section to compile on your platform. 
#
# For Windows (95/98/NT) compile, see makefile.win first. If
# it does not work then create a new console application
# in MS Visual C++ with the file names that are part of "SRC" below.
# Define the following macros in the project options: 
#       _WINDOWS, _NEED_BASENAME, _NEED_DIRNAME
# and build the project.
#
# Note that the only functions used from libgen are "basename"
# and "dirname". If _NEED_BASENAME and _NEED_DIRNAME are defined
# then one shouldn't link against libgen.
#
# The file ff_irtm.h is generated using a small program named
# "gen_irtm.c". This program can be compiled and run by making
# the target "gen_irtm". Ordinarily, no one should have to run
# it. It is included for completeness sake.
#
# You may have to tweak the options below in order to compile
# on your platform.
#

# Basic compile using gcc, force chars to be signed.
# If your compiler does not recognize -fsigned-char, remove this
# option and add the equivalent instead.
CC=gcc -g
CFLAGS= -fsigned-char
LDFLAGS= -static
LDLIBS=-lgen -lm

# For AIX native compiler
# CC=xlc
# CFLAGS=-O -qchar=signed
# LDFLAGS=-O
# LDLIBS=-lm

# gcc compile on a SunOS machine.
# If your compiler does not recognize -fsigned-char, remove this
# option and add the equivalent instead.
# CC=gcc
# CFLAGS=-O -fsigned-char -DNEED_STRTOUL
# LDFLAGS=-O
# LDLIBS=-lm

# For HPUX native compiler
# CC=cc
# CFLAGS=-O -Ae +DAportable 
# LDFLAGS=-O -Wl,-a,archive_shared
# LDLIBS=-lm

# For x86 Linux and other intel UNIXs
# CC=gcc
# CFLAGS=-O -D_LITTLE_ENDIAN
# LDFLAGS=-O
# LDLIBS=-lm

# For SGI native compiler
# CC=cc
# CFLAGS=-O -signed
# LDFLAGS=-O
# LDLIBS=-lgen -lm

####################################################################
############ D E V E L O P M E N T     C O M P I L E ###############
####################################################################
## Default development compile
#CC=gcc
#
## Compile with purify
## CC=purify gcc
#
#CFLAGS=-O -fsigned-char -Wimplicit -Wformat -Winline 
#CFLAGS+=-g
#CFLAGS+=-DLOGGING 
## CFLAGS+=-DDEBUG
#
## Compile with gprof profiling
## CFLAGS+=-pg
#
## Compile with optimization
#CFLAGS+=-O
#
## Baseline file mmap time
## CFLAGS+=-DDEBUG_BASELINE
#
## Link with debugging
#LDFLAGS+=-g
#
## Link with optimize
#LDFLAGS+=-O
#
#LDLIBS=-lm
####################################################################

SRC=vanilla.c dataset.c header.c dir.c convert.c fields.c select.c \
		buffs.c output.c search.c tools.c io_lablib3.c \
		fake.c ff_t20.c ff_irtm.c \
		mem.c system.c \
		vidx.c vidxuse.c vidxbits.c \
		rough_ct.c \
		logging.c

INC=header.h gendef.h proto.h io_lablib3.h toolbox.h tools.h \
		version.h fake.h output.h ff_irtm.h mem.h dos.h system.h \
		vidx.h vidxbits.h \
		rough_ct.h 

MISC=gen_irtm.c Makefile makefile.win

OBJ=$(SRC:.c=.o)

vanilla:	$(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LDLIBS) -o vanilla

check: $(SRC)
	lint $(CFLAGS) $(SRC)

.c.o:
	$(CC) -c $(CFLAGS) $<

gen_irtm: gen_irtm.c
	$(CC) -o$@ gen_irtm.c
	./gen_irtm > ff_irtm.h

clean:
	rm -f $(OBJ) vanilla

depend:
	gcc -MM $(SRC) >> Makefile

dist:
	VER=`grep 'version[[][]][ ]*=.*;' version.h | sed 's/.*=.*[ ]\([.0-9][.0-9]*\).*\;/\1/'`; \
	$(MAKE) "VER=$$VER" Dist

Dist:
	mkdir vanilla-$(VER)
	-ln $(SRC) $(INC) $(MISC) vanilla-$(VER)
	tar -chf - vanilla-$(VER) | \
		compress - > vanilla-$(VER).tar.Z && \
		echo "Distribution saved in vanilla-$(VER).tar.Z" || \
		echo "Creation of distribution failed."
	-rm -rf vanilla-$(VER)

TAGS: $(SRC) $(INC)
	etags --ignore-indentation $(SRC) $(INC)

XREF: $(SRC) $(INC)
	ctags -x $(SRC) $(INC)

send:	vanilla
	send.sh `config.guess`

vanilla.o: vanilla.c header.h tools.h gendef.h proto.h version.h \
 output.h
dataset.o: dataset.c header.h tools.h gendef.h proto.h system.h
header.o: header.c header.h tools.h gendef.h proto.h io_lablib3.h \
 toolbox.h
dir.o: dir.c tools.h gendef.h system.h
convert.o: convert.c header.h tools.h gendef.h proto.h mem.h
fields.o: fields.c header.h tools.h gendef.h proto.h
select.o: select.c header.h tools.h gendef.h proto.h
buffs.o: buffs.c header.h tools.h gendef.h proto.h mem.h
output.o: output.c header.h tools.h gendef.h proto.h output.h
search.o: search.c header.h tools.h gendef.h proto.h
tools.o: tools.c tools.h gendef.h
io_lablib3.o: io_lablib3.c header.h tools.h gendef.h proto.h \
 io_lablib3.h toolbox.h
fake.o: fake.c header.h tools.h gendef.h proto.h output.h fake.h
ff_t20.o: ff_t20.c fake.h header.h tools.h gendef.h proto.h output.h
ff_irtm.o: ff_irtm.c fake.h header.h tools.h gendef.h proto.h output.h \
 ff_irtm.h
mem.o: mem.c
vidx.o: vidx.h header.h
system.o: system.c
vidxuse.o: vidxuse.c vidx.h header.h tools.h gendef.h proto.h \
 mem.h
logging.o: logging.c
vidxbits.o: vidxbits.c vidxbits.h
rough_ct.o: rough_ct.c header.h tools.h gendef.h proto.h
