AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS = $(EVDEV_CFLAGS)
AM_CFLAGS += $(CONFIG_CFLAGS)

bin_PROGRAMS=spacefn
spacefn_SOURCES = src/spacefn.c
spacefn_LDADD = -levdev -lconfig
