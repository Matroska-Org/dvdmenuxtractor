OUTPUT = ../../../../release/gcc_win32/libdvdread.a
BUILD = ../../../../build/gcc_win32/

CC = gcc
CXX = g++
WINDRES = windres
ASM = yasm
NASM = nasmw
STRIP = strip

CCFLAGS += -DSTDC_HEADERS -I . -I ../win32 -I ..  -I ../../../..
CCFLAGS += -mthreads -D__MINGW32__ -D_WIN32 -D_M_IX86 -O3 -fomit-frame-pointer -march=i686 -msse -mmmx
CFLAGS += -Wdeclaration-after-statement
CXXFLAGS +=  -fno-for-scope
ASMFLAGS += -f win32 -D_WIN32 -DSTDC_HEADERS -I../win32 -I..
NASMFLAGS += -f win32 -D_WIN32 -DSTDC_HEADERS -I../win32 -I.. -I ../../../..
SFLAGS +=  -DSTDC_HEADERS -I../win32 -I.. -I ../../../..
LFLAGS += -static
LFLAGS += $(LIBS)      -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32 -L../../../../release/gcc_win32/
SOURCE += cmd_print.c
SOURCE += dvd_input.c
SOURCE += dvd_reader.c
SOURCE += dvd_udf.c
SOURCE += ifo_print.c
SOURCE += ifo_read.c
SOURCE += md5.c
SOURCE += nav_print.c
SOURCE += nav_read.c


ifeq ($(V),1)
 hide = $(empty)
else
 hide = @
endif

ifeq ($(DEBUG),1)
 CCFLAGS += -g -O0
else
 CCFLAGS += -DNDEBUG
 SFLAGS += -DNDEBUG
endif

OBJS:=$(SOURCE:../../../../%.c=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:%.c=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:../../../../%.cpp=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:%.cpp=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:../../../../%.rc=$(BUILD)%_res.dvdread.o)
OBJS:=$(OBJS:%.rc=$(BUILD)%_res.dvdread.o)
OBJS:=$(OBJS:../../../../%.S=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:%.S=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:../../../../%.m=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:%.m=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:../../../../%.nas=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:%.nas=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:../../../../%.asm=$(BUILD)%.dvdread.o)
OBJS:=$(OBJS:%.asm=$(BUILD)%.dvdread.o)

DEPS:=$(SOURCE:../../../../%.c=$(BUILD)%.dvdread.o.d)
DEPS:=$(DEPS:%.c=$(BUILD)%.dvdread.o.d)
DEPS:=$(DEPS:../../../../%.cpp=$(BUILD)%.dvdread.o.d)
DEPS:=$(DEPS:%.cpp=$(BUILD)%.dvdread.o.d)
DEPS:=$(DEPS:../../../../%.rc=$(BUILD)%_res.dvdread.o.d)
DEPS:=$(DEPS:%.rc=$(BUILD)%_res.dvdread.o.d)
DEPS:=$(DEPS:../../../../%.S=$(BUILD)%.dvdread.o.d)
DEPS:=$(DEPS:%.S=$(BUILD)%.dvdread.o.d)
DEPS:=$(DEPS:../../../../%.m=$(BUILD)%.dvdread.o.d)
DEPS:=$(DEPS:%.m=$(BUILD)%.dvdread.o.d)
DEPS:=$(DEPS:../../../../%.nas=)
DEPS:=$(DEPS:%.nas=)
DEPS:=$(DEPS:../../../../%.asm=)
DEPS:=$(DEPS:%.asm=)

.PHONY: all prebuild postbuild install clean distclean cleandep $(LIBS)

all: prebuild $(LIBS) $(GENERATED) $(OUTPUT) postbuild


$(OUTPUT): $(OBJS) $(USEBUILT) $(ICON) $(INSTALL) $(PLIST)
	$(hide)echo linking $@
	$(hide)mkdir -p ../../../../release/gcc_win32/
	$(hide)$(AR) rcs $@ $(OBJS)

$(BUILD)%_res.dvdread.o: ../../../../%.rc
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(WINDRES) -F pe-i386 -I$(*D) $< $@

$(BUILD)%_res.dvdread.o: %.rc
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(WINDRES) -F pe-i386 -I$(*D) $< $@

$(BUILD)%.dvdread.o: ../../../../%.c
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(CCENV) $(CC) -Wall $(CFLAGS) $(CCFLAGS) -c $< -o $@ -MMD -MP -MF $@.d

$(BUILD)%.dvdread.o: %.c
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(CCENV) $(CC) -Wall $(CFLAGS) $(CCFLAGS) -c $< -o $@ -MMD -MP -MF $@.d

$(BUILD)%.dvdread.o: ../../../../%.m
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(CCENV) $(CC) -Wall $(CFLAGS) $(CCFLAGS) -c $< -o $@ -MMD -MP -MF $@.d

$(BUILD)%.dvdread.o: %.m
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(CCENV) $(CC) -Wall $(CFLAGS) $(CCFLAGS) -c $< -o $@ -MMD -MP -MF $@.d

$(BUILD)%.dvdread.o: ../../../../%.S
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(CCENV) $(CC) -Wall $(SFLAGS) -c $< -o $@ -MMD -MP -MF $@.d

$(BUILD)%.dvdread.o: %.S
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(CCENV) $(CC) -Wall $(SFLAGS) -c $< -o $@ -MMD -MP -MF $@.d

$(BUILD)%.dvdread.o: ../../../../%.cpp
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(CCENV) $(CXX) -Wall $(CXXFLAGS) $(CCFLAGS) -c $< -o $@ -MMD -MP -MF $@.d

$(BUILD)%.dvdread.o: %.cpp
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(CCENV) $(CXX) -Wall $(CXXFLAGS) $(CCFLAGS) -c $< -o $@ -MMD -MP -MF $@.d

$(BUILD)%.dvdread.o: ../../../../%.asm
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD)%.dvdread.o: %.asm
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD)%.dvdread.o: ../../../../%.nas
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(NASM) $(NASMFLAGS) -o $@ $<

$(BUILD)%.dvdread.o: %.nas
	$(hide)echo compiling $<
	$(hide)mkdir -p $(BUILD)$(*D)
	$(hide)$(NASM) $(NASMFLAGS) -o $@ $<

install: $(OUTPUT)

clean:
	$(hide)rm -f $(OBJS)
	$(hide)rm -f $(OUTPUT)

cleandep:
	$(hide)rm -f $(DEPS)

distclean: cleandep
	$(hide)rm -f $(OBJS) $(LIBS)
	$(hide)rm -f $(OUTPUT)

-include $(DEPS)

