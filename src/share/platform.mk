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
CWARNS = -Wall -Wextra -Werror -Wno-error=unused-function -Wno-unused-function
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
MESSAGE = $(BUILD_DIR)l_message$(O)
STRINGO = $(BUILD_DIR)l_string$(O)
LUACOBJ = $(BUILD_DIR)l_state$(O)
IONFOBJ = $(BUILD_DIR)l_ionfmgr$(O)
SOCKOBJ = $(BUILD_DIR)linuxsock$(O)
SRVCOBJ = $(BUILD_DIR)l_service$(O)
MASTERO = $(BUILD_DIR)l_master$(O)
#HTTPOBJ = $(BUILD_DIR)httpservice$(O)
TESTOBJ = $(COREOBJ) $(STRINGO) $(MESSAGE) $(LUACOBJ) $(IONFOBJ) $(SOCKOBJ) $(SRVCOBJ) $(MASTERO) #$(HTTPOBJ)
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

# luaconf.h:581:2: #error "Compiler does not support 'long long'. Use option '-DLUA_32BITS' or '-DLUA_C89_NUMBERS'
# it is caused by LLONG_MAX is not defined in C89 (C99 stuff)
l_state.o: l_state.c thatcore.h
	$(CC) -std=c99 $(CMPL_OPTIONS) $<

$(BUILD_DIR)%$(O): %.c
	$(CMPL) $<

$(AUTOOBJ): autoconf.c
$(COREOBJ): thatcore.c linuxcore.c thattest.c thatcore.h autoconf.h l_prefix.h
$(IONFOBJ): l_ionfmgr.c l_ionfmgr.h plationf.h
$(SOCKOBJ): linuxsock.c l_socket.h platsock.h
$(SRVCOBJ): l_service.c l_service.h
$(STRINGO): l_string.c thatcore.h
$(MASTERO): l_master.c l_master.h
$(MESSAGE): l_message.c l_message.h
#$(HTTPOBJ): httpservice.c httpservice.h

