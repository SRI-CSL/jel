
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

noinst_LIBRARIES = libjpeg.a

jeldir = $(prefix)/lib
jel_LIBRARIES = libjel.a

SYSDEPSOURCES = \
	jpeg-9a/jmemansi.c  #jpeg-9a/jdatadst.c 

bindir =  $(exec_prefix)/bin
bin_PROGRAMS = jpegtran wedge unwedge jblock jhist jquant wcap 

pkgconfigdir = $(prefix)/lib/pkgconfig
pkgconfig_DATA = jel.pc

includedir = $(prefix)
nobase_include_HEADERS = \
	include/jel/jel.h \
	include/jel/jpeg-9a/jconfig.h \
	include/jel/jpeg-9a/jerror.h \
	include/jel/jpeg-9a/jinclude.h \
	include/jel/jpeg-9a/jmorecfg.h \
	include/jel/jpeg-9a/jpegint.h \
	include/jel/jpeg-9a/jpeglib.h


JEL_LIBS = $(jel_LIBRARIES)

wedge_SOURCES = utils/wedge.c $(SYSDEPSOURCES)

wedge_LDADD =  $(JEL_LIBS)

unwedge_SOURCES = utils/unwedge.c $(SYSDEPSOURCES)

unwedge_LDADD = $(JEL_LIBS)

jblock_SOURCES = utils/jblock.c $(SYSDEPSOURCES)

jblock_LDADD = $(JEL_LIBS)

jhist_SOURCES = utils/jhist.c $(SYSDEPSOURCES)

jhist_LDADD = $(JEL_LIBS)

jquant_SOURCES = utils/jquant.c $(SYSDEPSOURCES)

jquant_LDADD = $(JEL_LIBS)

wcap_SOURCES = utils/wcap.c $(SYSDEPSOURCES)

wcap_LDADD = $(JEL_LIBS)

jpegtran_SOURCES = \
	jpeg-9a/jpegtran.c \
	jpeg-9a/transupp.c \
	jpeg-9a/cdjpeg.c \
	jpeg-9a/rdswitch.c \
	libjel/jpeg-stdio-src.c \
	libjel/jpeg-stdio-dst.c \
	$(SYSDEPSOURCES)

jpegtran_LDADD = libjpeg.a

libjpeg_a_SOURCES = \
	jpeg-9a/jaricom.c \
	jpeg-9a/jcapimin.c \
	jpeg-9a/jcapistd.c \
	jpeg-9a/jcarith.c \
	jpeg-9a/jccoefct.c \
	jpeg-9a/jccolor.c \
	jpeg-9a/jcdctmgr.c \
	jpeg-9a/jchuff.c \
	jpeg-9a/jcinit.c \
	jpeg-9a/jcmainct.c \
	jpeg-9a/jcmarker.c \
	jpeg-9a/jcmaster.c \
	jpeg-9a/jcomapi.c \
	jpeg-9a/jcparam.c \
	jpeg-9a/jcprepct.c \
	jpeg-9a/jcsample.c \
	jpeg-9a/jctrans.c \
	jpeg-9a/jdapimin.c \
	jpeg-9a/jdapistd.c \
	jpeg-9a/jdarith.c \
	jpeg-9a/jdcoefct.c \
	jpeg-9a/jdcolor.c \
	jpeg-9a/jddctmgr.c \
	jpeg-9a/jdhuff.c \
	jpeg-9a/jdinput.c \
	jpeg-9a/jdmainct.c \
	jpeg-9a/jdmarker.c \
	jpeg-9a/jdmaster.c \
	jpeg-9a/jdmerge.c \
	jpeg-9a/jdpostct.c \
	jpeg-9a/jdsample.c \
	jpeg-9a/jdtrans.c \
	jpeg-9a/jerror.c \
	jpeg-9a/jfdctflt.c \
	jpeg-9a/jfdctfst.c \
	jpeg-9a/jfdctint.c \
	jpeg-9a/jidctflt.c \
	jpeg-9a/jidctfst.c \
	jpeg-9a/jidctint.c \
	jpeg-9a/jquant1.c \
	jpeg-9a/jquant2.c \
	jpeg-9a/jutils.c \
	jpeg-9a/jmemmgr.c

#	jpeg-9a/jdatadst.c	jpeg-9a/jdatasrc.c


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
	$(SYSDEPSOURCES) \
	$(RSCODE_SOURCES) \
	$(libjpeg_a_SOURCES) 

