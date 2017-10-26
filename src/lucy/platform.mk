PLAT = none
PLATS = linux macosx

DEBUG =
MYINC = -I./
MACRO =
LDPATH =
LDLIBS = -llua -lm -ldl
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

AUTOOBJ = autoconf$(O)

COREOBJ = core/base$(O) \
          core/fileop$(O) \
          core/queue$(O) \
          core/table$(O) \
          core/string$(O) \
          core/match$(O) \
          core/master$(O) \
          core/state$(O) \
          osi/linuxcore$(O) \
          osi/linuxsock$(O)

HTTPOBJ = net/http$(O)
TESTOBJ = $(COREOBJ) core/test$(O) # $(COREOBJ) #  $(HTTPOBJ)
ALLOBJS = $(AUTOOBJ) $(TESTOBJ)

AUTOCONF = autoconf$(E)
CORETEST = core/test$(E)

ifeq ($(PLAT), none)
default: none
else
default: echo $(AUTOCONF) $(CORETEST)
endif

none:
	@echo "make <platfrom> # $(PLATS)"

echo:
	@echo "PLAT= $(PLAT)"
	@echo "DEBUG= $(DEBUG)"
	@echo "CC= $(CC89)"
	@echo "CFLAGS= $(CFLAGS)"

clean:
	$(RM) $(AUTOCONF) $(CORETEST) $(ALLOBJS) $(BUILD_DIR) autoconf.h

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
core/state.o: core/state.c lucycore.h
	$(CC) -std=c99 $(CMPL_OPTIONS) $<

%.$(O): %.c
	$(CMPL) $<

$(AUTOOBJ): autoconf.c core/prefix.h osi/plationf.h osi/platsock.h
$(COREIND): autoconf.h lucycore.h core/prefix.h osi/plationf.h osi/platsock.h osi/linuxpref.h
$(PLATSRC): osi/linuxcore.c osi/eventpoll.c osi/bsdkqueue.c osi/plainpoll.c osi/linuxsock.c
$(COREOBJ): core/base.c core/string.c core/state.c core/master.c $(PLATSRC) $(COREIND)
$(HTTPOBJ): net/http.c net/http.h $(COREIND)

