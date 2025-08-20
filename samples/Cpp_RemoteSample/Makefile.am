# set the include path found by configure
INCLUDES= $(all_includes) -I../../include

AM_CFLAGS   = -Wall
AM_CXXFLAGS = $(AM_CFLAGS)

# use shared library
AM_LDFLAGS = -L../../src ../../src/libssm.la -lm
# use static library
#AM_LDFLAGS = -L../src -lssm -lm -all-static

noinst_HEADERS = intSsm.h

noinst_PROGRAMS = read_buffer_test read_proxy_test write_proxy_test

ssmWriteSample_SOURCES = ssmWriteSample.cpp
ssmReadSample_SOURCES =  ssmReadSample.cpp

read_buffer_test_SOURCES = read_buffer_test.cpp
read_proxy_test_SOURCES = read_proxy_test.cpp
write_proxy_test_SOURCES = write_proxy_test.cpp

