SRCTOP=../..
include $(SRCTOP)/Makefile.inc

TARGET = kernel.bin
ALIAS  = KERNEL.BIN

KERNEL_BASE_ADDR = 0x1000
REL_KERNEL_ADDR  = 0x0200
KERNEL_START_ADDR= 0x1200 # REL_KERNEL_ADDR + KERNEL_BASE_ADDR
MONA_CFG_ADDR    = 0x90000
LFLAGS       = -n -Ttext $(KERNEL_START_ADDR) -static
CXXFLAGS    += -DOSTYPE="\"$(OSTYPE)\"" -DREL_KERNEL_ADDR=$(REL_KERNEL_ADDR) -DKERNEL_BASE_ADDR=$(KERNEL_BASE_ADDR) -momit-leaf-frame-pointer -fomit-frame-pointer -march=i686
CXXFLAGS    += -DMONA_CFG_SIZE=$(MONA_CFG_SIZE) -DMONA_CFG_ADDR=$(MONA_CFG_ADDR)
INCLUDE      = -I. -I$(INCDIR)
SOURCES      = cstart.cpp            \
               kernel.cpp            \
               ihandlers.cpp         \
               operator.cpp          \
               purevirtual.cpp       \
               checker.cpp           \
               string.cpp            \
               Semaphore.cpp         \
               GraphicalConsole.cpp  \
               pic.cpp               \
               io.cpp                \
               syscalls.cpp          \
               Process.cpp           \
               GDTUtil.cpp           \
               IDTUtil.cpp           \
               PageManager.cpp       \
               MemoryManager.cpp     \
               Segments.cpp          \
               Mutex.cpp             \
               UserSemaphore.cpp     \
               VesaConsole.cpp       \
               LogConsole.cpp        \
               IDManager.cpp         \
               KObject.cpp           \
               Thread.cpp            \
               Loader.cpp            \
               Scheduler.cpp         \
               RTC.cpp               \
               Messenger.cpp         \
               MemoryAllocator.cpp   \
               apm.cpp               \
               shutdown.cpp          \
               vsprintf.cpp          \
               gdb-adapter.cpp       \
               Uart.cpp              \
               gdb-i386-stub.cpp     \
               KObjectService.cpp    \
               addressmap.cpp

ASSRCS = memset.S memcpy.S memcmp.S memmove.S strlen.S
OBJECTS = $(SOURCES:.cpp=.o) $(ASSRCS:.S=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS) mode12h.o ihandler.o core.o apm_bios.o
	$(LD) -e $(SYMPREFIX)cstart --Map kernel.map $(LFLAGS) -o $@ $(OBJECTS) mode12h.o ihandler.o core.o apm_bios.o
#	$(NDISASM) -u $@ > kernel.lst
	$(STRIP) -O binary $@

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

.SUFFIXES: .S .o
.S.o:
	$(CC) $(CXXFLAGS) $(INCLUDE) -c $<

mode12h.o: mode12h.asm
	$(NASM) $(NFLAGS) mode12h.asm  -o $@ -l mode12h.lst

ihandler.o: ihandler.asm
	$(NASM) $(NFLAGS) ihandler.asm  -o $@ -l ihandler.lst

core.o: core.asm
	$(NASM) $(NFLAGS) core.asm  -o $@ -l core.lst

apm_bios.o: apm_bios.asm
	$(NASM) $(NFLAGS) apm_bios.asm  -o $@ -l apm_bios.lst

doxygen:
	$(DOXYGEN)

install: $(TARGET)
	mkdir -p $(BINDIR)
	$(INSTALL) $(TARGET) $(BINDIR)/$(ALIAS)

uninstall:
	rm -f $(BINDIR)/$(ALIAS)

depend dependencies:
	$(CXX) -MM $(INCLUDE) $(CXXFLAGS) $(SOURCES) > dependencies

clean:
	rm -f $(TARGET) *.o *.lst *.map dependencies

# for flymake
check-syntax:
	LANG=C $(CXX) -o nul -fsyntax-only $(CXXFLAGS) $(INCLUDE) -S ${CHK_SOURCES}

-include dependencies
