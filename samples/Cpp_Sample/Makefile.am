# set the include path found by configure
INCLUDES= $(all_includes) -I../../include

AM_CFLAGS   = -Wall
AM_CXXFLAGS = $(AM_CFLAGS)

# use shared library
AM_LDFLAGS = -L../../src ../../src/libssm.la -lm
# use static library
#AM_LDFLAGS = -L../src -lssm -lm -all-static

noinst_HEADERS = intSsm.h

noinst_PROGRAMS = ssmReadSample ssmWriteSample

ssmWriteSample_SOURCES = ssmWriteSample.cpp
ssmReadSample_SOURCES =  ssmReadSample.cpp

