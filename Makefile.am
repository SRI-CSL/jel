
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
	jpeg-6b/jmemansi.c \
	jpeg-6b/jdatadst.c 

bindir =  $(exec_prefix)/bin
bin_PROGRAMS = jpegtran wedge unwedge jblock jhist jquant wcap 

pkgconfigdir = $(prefix)/lib/pkgconfig
pkgconfig_DATA = jel.pc

includedir = $(prefix)
nobase_include_HEADERS = \
	include/jel/jel.h \
	include/jel/jpeg-6b/jconfig.h \
	include/jel/jpeg-6b/jerror.h \
	include/jel/jpeg-6b/jinclude.h \
	include/jel/jpeg-6b/jmorecfg.h \
	include/jel/jpeg-6b/jpegint.h \
	include/jel/jpeg-6b/jpeglib.h


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
	jpeg-6b/jpegtran.c \
	jpeg-6b/transupp.c \
	jpeg-6b/cdjpeg.c \
	jpeg-6b/rdswitch.c \
	$(SYSDEPSOURCES)

jpegtran_LDADD = libjpeg.a

libjpeg_a_SOURCES = \
	jpeg-6b/jcapimin.c \
	jpeg-6b/jcapistd.c \
	jpeg-6b/jccoefct.c \
	jpeg-6b/jccolor.c \
	jpeg-6b/jcdctmgr.c \
	jpeg-6b/jchuff.c \
	jpeg-6b/jcinit.c \
	jpeg-6b/jcmainct.c \
	jpeg-6b/jcmarker.c \
	jpeg-6b/jcmaster.c \
	jpeg-6b/jcomapi.c \
	jpeg-6b/jcparam.c \
	jpeg-6b/jcphuff.c \
	jpeg-6b/jcprepct.c \
	jpeg-6b/jcsample.c \
	jpeg-6b/jctrans.c \
	jpeg-6b/jdapimin.c \
	jpeg-6b/jdapistd.c \
	jpeg-6b/jdatadst.c \
	jpeg-6b/jdatasrc.c \
	jpeg-6b/jdcoefct.c \
	jpeg-6b/jdcolor.c \
	jpeg-6b/jddctmgr.c \
	jpeg-6b/jdhuff.c \
  jpeg-6b/jdinput.c \
	jpeg-6b/jdmainct.c \
	jpeg-6b/jdmarker.c \
	jpeg-6b/jdmaster.c \
	jpeg-6b/jdmerge.c \
	jpeg-6b/jdphuff.c \
	jpeg-6b/jdpostct.c \
	jpeg-6b/jdsample.c \
	jpeg-6b/jdtrans.c \
	jpeg-6b/jerror.c \
	jpeg-6b/jfdctflt.c \
	jpeg-6b/jfdctfst.c \
	jpeg-6b/jfdctint.c \
	jpeg-6b/jidctflt.c \
	jpeg-6b/jidctfst.c \
	jpeg-6b/jidctint.c \
	jpeg-6b/jidctred.c \
	jpeg-6b/jquant1.c \
	jpeg-6b/jquant2.c \
	jpeg-6b/jutils.c \
	jpeg-6b/jmemmgr.c

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
	libjel/jel.c \
	$(SYSDEPSOURCES) \
	$(RSCODE_SOURCES) \
	$(libjpeg_a_SOURCES) 

