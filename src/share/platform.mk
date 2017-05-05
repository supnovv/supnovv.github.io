PLAT = none
PLATS = linux macosx

DEBUG =
MYINC =
MACRO =
LDPATH =
LDLIBS =
SHARED =

CC = gcc -std=c89
CWARNS = -Wall -Wextra -Werror
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

CMPL = $(CC) $(CFLAGS) -c -o$@
LINK = $(CC) -o$@

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

AUTOCONF = $(BUILD_DIR)autoconf$(E)
CORETEST = $(BUILD_DIR)thattest$(E)
AUTOOBJ = $(BUILD_DIR)autoconf$(O)
MAINOBJ = $(BUILD_DIR)mainentry$(O)
COREOBJ = $(BUILD_DIR)thatcore$(O) $(BUILD_DIR)linuxcore$(O) $(BUILD_DIR)thattest$(O)
ALLOBJS = $(AUTOOBJ) $(MAINOBJ) $(COREOBJ)

ifeq ($(PLAT), none)
default: none
endif

ifeq ($(DEBUG), ON)
default: echo $(CORETEST) clean
else
default: echo $(AUTOCONF) $(COREOBJ) $(CORETEST)
	$(MKDIR) debug
	$(MAKE) PLAT=$(PLAT) DEBUG="ON" MACRO="-DCCDEBUG" -f platform.mk
endif

none:
	@echo "make <platfrom> # $(PLATS)"

echo:
	@echo "PLAT= $(PLAT)"
	@echo "DEBUG= $(DEBUG)"
	@echo "CC= $(CC)"
	@echo "CFLAGS= $(CFLAGS)"

clean:
	$(RM) $(AUTOCONF) $(CORETEST) $(ALLOBJS) $(BUILD_DIR)

.PHONY: default none echo clean

$(AUTOCONF): $(AUTOOBJ)
	$(RM) autoconf.h
	$(LINK) $(AUTOOBJ)
	./$@

$(CORETEST): $(MAINOBJ) $(COREOBJ)
	$(LINK) $(MAINOBJ) $(COREOBJ) $(LDLIBS)
	./$@

$(BUILD_DIR)%$(O): %.c
	$(CMPL) $<

$(AUTOOBJ): autoconf.c
$(MAINOBJ): mainentry.c thatcore.h autoconf.h
$(COREOBJ): thatcore.c windows.c thattest.c thatcore.h autoconf.h
