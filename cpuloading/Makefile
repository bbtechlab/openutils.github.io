# ----------------------------------------------------------------------------
# Filename     : Makfile to build cpuloading
# Version      : 1.0
# Created      : 06/13/2018 01:30:05 PM
# Revision     : none
# Compiler     : GNU Make
# Author       : Bamboo Do, dovanquyen.vn@gmail.com
# Copyright (c) 2018,  All rights reserved.
# Description  :
# ----------------------------------------------------------------------------

CURRDIR := $(shell pwd)

# ----------------------------------------------------------------------------
# Compiler settings
CROSS ?=
CC ?= $(CROSS)gcc
AR ?= $(CROSS)ar

# ----------------------------------------------------------------------------
# Build objects
obj-y :=
obj-y += cpuloading.o
obj-y += build_date.o

# ----------------------------------------------------------------------------
# Include header
INC :=
CFLAGS := -fdata-sections -ffunction-sections -std=gnu99 -fno-strict-aliasing -fPIC \
            -DLINUX -pipe -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE=1 -D__LINUX__
			
# ----------------------------------------------------------------------------
# Libraries objects

LIBLIST := -lpthread 
LDFLAGS := -Wl,--gc-sections

# ----------------------------------------------------------------------------
# Rules

all: BUILD_DATE cpuloading

BUILD_DATE: FORCE
	@echo "const char *g_strAppBuildDate = \"`date`\";" > build_date.c
	@echo "const char *g_strAppBuildUser = \"`whoami`@`hostname`\";" >> build_date.c

cpuloading: ${obj-y}
	@echo Compiling ... $<
	@$(CC) $(LDFLAGS) -Wl,--start-group $(LIBLIST) ${obj-y} -Wl,--end-group  -o $@
	
clean:
	@rm -f *.o *.a
	@rm -f cpuloading
	
distclean: clean	

FORCE:

%.o: %.c
	$(CC) -c $< $(CFLAGS) $(INC) $(LDFLAGS) -o $@
	
.PHONY: cpuloading	
	
vpath %.c ./