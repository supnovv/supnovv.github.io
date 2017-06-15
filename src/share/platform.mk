PLAT = none
PLATS = linux macosx

DEBUG =
MYINC =
MACRO =
LDPATH =
LDLIBS = -llua -lm
SHARED =

CC = gcc
CC89 = gcc -std=c89
CWARNS = -Wall -Wextra -Werror -Wno-error=unused-function
CMACRO = $(MACRO)
POSINDEPCODE = -fPIC # position independent code
CFLAGS = $(CWARNS) $(CMACRO) $(MYINC) -g -O2
LDSTATIC = -Wl,-Bstatic
LDSHARED = -Wl,-Bdynamic

ifeq ($(PLAT), linux)
SHARED = $(POSINDEPCODE) -shared -Wl,-E -ldl
LDLIBS += -lpthread
endif

ifeq ($(PLAT), macosx)
SHARED = $(POSINDEPCODE) -dynamiclib -Wl,-undefined,dynamic_lookup -ldl
endif

CMPL_OPTIONS = $(CFLAGS) -c -o$@
CMPL = $(CC89) $(CMPL_OPTIONS)
LINK = $(CC89) -o$@

RM = rm -rf
MKDIR = mkdir -p

O = .o
E =
D = .so
L = .a

ifeq ($(DEBUG), ON)
BUILD_DIR = debug/
else
BUILD_DIR =
endif

AUTOOBJ = $(BUILD_DIR)autoconf$(O)
COREOBJ = $(BUILD_DIR)thatcore$(O) $(BUILD_DIR)linuxcore$(O) $(BUILD_DIR)thattest$(O)
STRINGO = $(BUILD_DIR)string$(O)
LUACOBJ = $(BUILD_DIR)luacapi$(O)
IONFOBJ = $(BUILD_DIR)ionfmgr$(O)
SOCKOBJ = $(BUILD_DIR)linuxsock$(O)
SRVCOBJ = $(BUILD_DIR)service$(O)
HTTPOBJ = $(BUILD_DIR)http_service$(O)
TESTOBJ = $(COREOBJ) $(STRINGO) $(LUACOBJ) $(IONFOBJ) $(SOCKOBJ) $(SRVCOBJ) $(HTTPOBJ)
ALLOBJS = $(AUTOOBJ) $(TESTOBJ)

AUTOCONF = $(BUILD_DIR)autoconf$(E)
CORETEST = $(BUILD_DIR)thattest$(E)

ifeq ($(PLAT), none)
default: none
endif

ifeq ($(DEBUG), ON)
default: echo $(CORETEST) clean
else
default: echo $(AUTOCONF) $(CORETEST)
#	$(MKDIR) debug
#	$(MAKE) PLAT=$(PLAT) DEBUG="ON" MACRO="-DCCDEBUG" -f platform.mk
endif

none:
	@echo "make <platfrom> # $(PLATS)"

echo:
	@echo "PLAT= $(PLAT)"
	@echo "DEBUG= $(DEBUG)"
	@echo "CC= $(CC89)"
	@echo "CFLAGS= $(CFLAGS)"

clean:
	$(RM) $(AUTOCONF) $(CORETEST) $(ALLOBJS) $(BUILD_DIR)

.PHONY: default none echo clean

$(AUTOCONF): $(AUTOOBJ)
	$(RM) autoconf.h
	$(LINK) $(AUTOOBJ)
	./$@

$(CORETEST): $(TESTOBJ)
	$(LINK) $(TESTOBJ) $(LDLIBS)
	./$@

luacapi.o: luacapi.c luacapi.h
	$(CC) $(CMPL_OPTIONS) $<

$(BUILD_DIR)%$(O): %.c
	$(CMPL) $<

$(AUTOOBJ): autoconf.c
$(COREOBJ): thatcore.c linuxcore.c thattest.c thatcore.h autoconf.h l_prefix.h
$(IONFOBJ): ionfmgr.c ionfmgr.h plationf.h
$(SOCKOBJ): linuxsock.c socket.h platsock.h
$(SRVCOBJ): service.c service.h
$(STRINGO): string.c string.h
$(HTTPOBJ): http_service.c http_service.h

