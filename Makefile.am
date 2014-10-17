
PROJECT_GIT_ORIG:=$(shell git config --get remote.origin.url 2>/dev/null || echo "unknownGITorigin")
PROJECT_GIT_BRCH:=$(shell git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknownGITbranch")
PROJECT_GIT_HASH:=$(shell git log --pretty=format:'%H' -n 1 2>/dev/null || echo "unknownGIThash")
PROJECT_GIT_TIME:=$(shell git log --pretty=format:'%ci' -n 1 2>/dev/null || echo "unknownGITtime")

PROJECT_GIT_STAMPS = \
	-DPROJECT_GIT_ORIG='"$(PROJECT_GIT_ORIG)"' \
	-DPROJECT_GIT_BRCH='"$(PROJECT_GIT_BRCH)"' \
	-DPROJECT_GIT_HASH='"$(PROJECT_GIT_HASH)"' \
	-DPROJECT_GIT_TIME='"$(PROJECT_GIT_TIME)"'

ACLOCAL_AMFLAGS = -I config-aux --install

AM_CPPFLAGS = -Werror -Wall -DECC -DJEL_VERSION='"$(VERSION)"' -I. -I$(srcdir)/include -I$(srcdir)/rscode  $(PROJECT_GIT_STAMPS)

LIBS = -ljpeg

jeldir = $(prefix)/lib
jel_LIBRARIES = libjel.a

bindir =  $(exec_prefix)/bin
bin_PROGRAMS = wedge unwedge jblock jhist jquant wcap energy

pkgconfigdir = $(prefix)/lib/pkgconfig
pkgconfig_DATA = jel.pc

includedir = $(prefix)
nobase_include_HEADERS = include/jel/jel.h 


JEL_LIBS = $(jel_LIBRARIES)

wedge_SOURCES = utils/wedge.c

wedge_LDADD =  $(JEL_LIBS)

unwedge_SOURCES = utils/unwedge.c

unwedge_LDADD = $(JEL_LIBS)

jblock_SOURCES = utils/jblock.c 

jblock_LDADD = $(JEL_LIBS)

jhist_SOURCES = utils/jhist.c

jhist_LDADD = $(JEL_LIBS)

jquant_SOURCES = utils/jquant.c

jquant_LDADD = $(JEL_LIBS)

wcap_SOURCES = utils/wcap.c

wcap_LDADD = $(JEL_LIBS)

energy_SOURCES = utils/energy.c

energy_LDADD = $(JEL_LIBS)

RSCODE_SOURCES = \
	rscode/rs.c  \
	rscode/galois.c  \
	rscode/berlekamp.c  \
	rscode/crcgen.c

libjel_a_SOURCES = \
	libjel/ijel-ecc.c \
	libjel/ijel.c \
	libjel/jpeg-fd-dst.c \
	libjel/jpeg-fd-src.c \
	libjel/jpeg-mem-dst.c \
	libjel/jpeg-mem-src.c \
	libjel/jpeg-stdio-dst.c \
	libjel/jpeg-stdio-src.c \
	libjel/jel.c \
	libjel/misc.c \
	$(RSCODE_SOURCES)

