TARGET  = MONAMONA
SOURCES = main.cpp \
          BfMain.cpp
INSTDIR = $(BINDIR)/APPS

SHAREDIR   = $(MONADIR)/share
ifneq ($(BUILD_TARGET),ELF)
ADDLINK    = -lbaygui-imp -lmonalibc-imp --enable-auto-import
include $(SHAREDIR)/configs/monapi-ex5.inc
else
include $(SHAREDIR)/configs/monapi-el5.inc
endif

CXXFLAGS += -DDEBUG

-include dependencies
