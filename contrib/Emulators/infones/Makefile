TARGET  = INFONES
SOURCES = InfoNES.cpp InfoNES_System_Mona.cpp InfoNES_Mapper.cpp InfoNES_pAPU.cpp K6502.cpp
INSTDIR = $(BINDIR)/APPS/BAYGUI

SHAREDIR   = $(MONADIR)/share
ifneq ($(BUILD_TARGET),ELF)
ADDLINK    = -lbaygui-imp -lmonalibc-imp --enable-auto-import
ADDLINKDEP = $(MONADIR)/lib/libbaygui-imp.a $(MONADIR)/lib/libmonalibc-imp.a
include $(SHAREDIR)/configs/monapi-ex5.inc
else
ADDLINK    = -lbaygui-imp -lmonalibc-imp -lmonapi-imp
ADDLINKDEP = $(MONADIR)/lib/libbaygui.a $(MONADIR)/lib/libmonalibc.a
include $(SHAREDIR)/configs/monapi-el5.inc
endif

CXXFLAGS += -DDEBUG

-include dependencies
