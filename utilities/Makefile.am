# set the include path found by configure
#INCLUDES= $(all_includes) -I../include

AM_CFLAGS   = -Wall
#AM_CXXFLAGS = $(AM_CFLAGS)
AM_CXXFLAGS = $(AM_CFLAGS) $(all_includes) -I../include

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = lsssm ssm-logger ssm-player ssm-date ssm-monitor ssm-graph dssm-proxy
noinst_HEADERS = printlog.hpp

# shell scripts
#EXTRA_DIST = ssm-advance-player killssm psssm topssm
dist_bin_SCRIPTS = killssm psssm topssm

## lsssm
lsssm_SOURCES = lsssm.c
lsssm_LDADD = ../src/libssm.la

## ssm-logger
ssm_logger_SOURCES = ssm-logger.cpp
ssm_logger_CFLAGS = $(AM_CFLAGS) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
ssm_logger_CXXFLAGS = $(ssm_logger_CFLAGS)
ssm_logger_LDADD = ../src/libssm.la -lm

## ssm-player
ssm_player_SOURCES = ssm-player.cpp
ssm_player_CFLAGS = $(AM_CFLAGS) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
ssm_player_LDADD = ../src/libssm.la -lm

## ssm-transporter
#ssm_transporter_SOURCES = ssm-transporter.c
#ssm_transporter_CFLAGS = $(AM_CFLAGS) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
#ssm_transporter_CXXFLAGS = $(ssm_transporter_CFLAGS)
#ssm_transporter_LDADD = ../src/libssm.la -lm -lpthread

## ssm-date
ssm_date_SOURCES = ssm-date.c
ssm_date_CFLAGS = $(AM_CFLAGS)
ssm_date_CXXFLAGS = $(ssm_date_CFLAGS)
ssm_date_LDADD = ../src/libssm.la

## ssm-monitor
ssm_monitor_SOURCES = ssm-monitor.c
ssm_monitor_CFLAGS = $(AM_CFLAGS)
ssm_monitor_CXXFLAGS = $(ssm_monitor_CFLAGS)
ssm_monitor_LDADD = ../src/libssm.la

## ssm-graph
ssm_graph_SOURCES = ssm-graph.cpp
ssm_graph_CFLAGS = $(AM_CFLAGS)
ssm_graph_CXXFLAGS = $(ssm_graph_CFLAGS)
ssm_graph_LDADD = ../src/libssm.la

##############################
## dssm-proxy
##############################
#bin_PROGRAMS = $(bin_PROGRAMS) dssm-proxy
dssm_proxy_SOURCES = dssm-proxy.cpp ../src/dssm-utility.cpp
dssm_proxy_CFLAGS = $(AM_CFLAGS) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 # -fsanitize=address
dssm_proxy_CXXFLAGS = $(dssm_proxy_CFLAGS) -std=c++14
dssm_proxy_LDADD = ../src/libssm.la  -lpthread
