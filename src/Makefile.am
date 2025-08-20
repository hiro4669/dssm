noinst_HEADERS =  ssm-coordinator.h

# set the include path found by configure
#INCLUDES= $(all_includes) -I../include

AM_CFLAGS   = -Wall
#AM_CXXFLAGS = $(AM_CFLAGS)
AM_CXXFLAGS = $(AM_CFLAGS) $(all_includes) -I../include

## libssm
lib_LTLIBRARIES = libssm.la

libssm_la_SOURCES = libssm-time.c libssm-shm.c libssm.c dssm-proxy-client.cpp dssm-utility.cpp
#########################################
## For dssm 
#libssm_la_SOURCES = $(libssm_la_SOURCES) dssm-proxy-client.cpp dssm-utility.cpp
#libssm_la_SOURCES = $(libssm_la_SOURCES) ../dssm/PConnector/src/dssm-proxy-client.cpp 
#########################################

libssm_la_CFLAGS = $(AM_CFLAGS) -Werror
libssm_la_CXXFLAGS = $(libssm_la_CFLAGS)
libssm_la_LIBADD = @LD_LIBRT@ -lpthread

#---------------------------------------------------
# ***_la_LDFLAGS = -version-info  CURRENT:REVISION:AGE
# 1. Bugfix                          C   : R++ : A
# 2. Add Interface                   C++ : R=0 : A++
# 3. Delete or Change Interface      C++ : R=0 : A=0

#libssm_la_LDFLAGS = -version-info 5:1:5
libssm_la_LDFLAGS = -version-info 6:0:0

## ssm-coordinator
bin_PROGRAMS = ssm-coordinator

ssm_coordinator_SOURCES = ssm-coordinator.cpp

ssm_coordinator_CFLAGS = $(AM_CFLAGS)
ssm_coordinator_CXXFLAGS = $(ssm_coordinator_CFLAGS)

ssm_coordinator_LDADD = libssm.la
