ifndef $(MONADIR)
export MONADIR=$(PWD)/../../../mona
endif


TARGET  = GNOTE
SOURCES = \
trash/tmenuitem.cpp \
trash/tmenu.cpp \
trash/tmenubar.cpp \
trash/tpopupmenu.cpp \
controller.cpp \
canvas.cpp \
document.cpp \
rootwindow.cpp \
gnote.cpp

INSTDIR = $(BINDIR)/APPS/BAYGUI
INSTFILES = GNOTE.map
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
INCLUDE += -I .

-include dependencies
