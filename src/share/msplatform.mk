PLAT = none

DEBUG =
MYINC =

CC = cl /nologo # cl will compile *.c as c89
CWARNS = /W4
CMACRO = $(MACRO) /D_CONSOLE
CFLAGS = $(CWARNS) $(CMACRO) $(MYINC) /O2
LDFLAGS = /Fe$@ /link
STATIC = /MT $(LDFLAGS)
SHARED = /MD $(LDFLAGS)

CMPL = $(CC) $(CFLAGS) /c /Fo$@
LINK = $(CC)

RM = -del
MKDIR = -mkdir

O = .obj
E = .exe
D = .dll
L = .lib

!if "$(DEBUG)"=="ON"
BUILD_DIR = debug/
!else
BUILD_DIR =
!endif

AUTOCONF = $(BUILD_DIR)autoconf$(E)
CORETEST = $(BUILD_DIR)thattest$(E)
AUTOOBJ = $(BUILD_DIR)autoconf$(O)
MAINOBJ = $(BUILD_DIR)mainentry$(O)
COREOBJ = $(BUILD_DIR)thattest$(O) $(BUILD_DIR)windows$(O) $(BUILD_DIR)thatcore$(O)
ALLOBJS = $(AUTOOBJ) $(MAINOBJ) $(COREOBJ)

!if "$(PLAT)"!="windows"
default: none
!endif

!if "$(DEBUG)"=="ON"
default: echo $(CORETEST) clean
!else
default: echo $(AUTOCONF) $(COREOBJ) $(CORETEST)
	$(MKDIR) debug
	$(MAKE) PLAT=$(PLAT) DEBUG="ON" MACRO="-DCCDEBUG" /f msplatform.mk
!endif

none: .PHONY
	@echo "nmake <platform> # windows"

echo: .PHONY
	@echo "PLAT= $(PLAT)"
	@echo "DEBUG= $(DEBUG)"
	@echo "CC= $(CC)"
	@echo "CFLAGS= $(CFLAGS)"

clean: .PHONY
	$(RM) $(AUTOCONF) $(CORETEST) $(ALLOBJS) $(BUILD_DIR)

.PHONY:

$(AUTOCONF): $(AUTOOBJ)
	$(RM) autoconf.h
	$(LINK) $(AUTOOBJ) $(STATIC)
	./$@

$(CORETEST): $(MAINOBJ) $(COREOBJ)
	$(LINK) $(MAINOBJ) $(COREOBJ) $(STATIC)
	./$@

{.}.c{$(BUILD_DIR)}$(O):
	$(CMPL) $<

$(AUTOOBJ): autoconf.c
$(MAINOBJ): mainentry.c thatcore.h autoconf.h
$(COREOBJ): thatcore.c windows.c thattest.c thatcore.h autoconf.h
