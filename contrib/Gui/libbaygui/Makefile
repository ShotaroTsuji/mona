ifndef $(MONADIR)
export MONADIR=$(shell cd $(PWD)/../../../mona; pwd)
endif

LIB_STATIC   = libbaygui.a
TARGET       = $(LIB_STATIC)

include $(MONADIR)/share/configs/Makefile.inc
include $(MONADIR)/share/configs/dirnames.inc
include $(MONADIR)/share/configs/monapi.inc

ifneq ($(BUILD_TARGET),ELF)
LIB_IMPORT   = libbaygui-imp.a
LIB_DYNAMIC  = BAYGUI.DLL
LIB_DYNAMIC5 = BAYGUI.DL5
TARGET      += $(LIB_IMPORT) $(LIB_DYNAMIC) $(LIB_DYNAMIC5)
endif

SOURCES = \
	main.cpp \
	awt/Button.cpp \
	awt/Checkbox.cpp \
	awt/CheckboxGroup.cpp \
	awt/Component.cpp \
	awt/Container.cpp \
	awt/FontMetrics.cpp \
	awt/Frame.cpp \
	awt/Graphics.cpp \
	awt/Image.cpp \
	awt/Label.cpp \
	awt/ListBox.cpp \
	awt/Scrollbar.cpp \
	awt/TextField.cpp \
	awt/Window.cpp \
	awt/ImeManager.cpp \
	lang/Object.cpp \
	lang/String.cpp \
	lang/Thread.cpp \
	util/Hashtable.cpp \
	util/Vector.cpp

OBJECTS = $(SOURCES:.cpp=.o)
INCLUDE =  -I. -I.. -I$(INCDIR) -I ../ime_server -I$(INCDIR)/stlport

all : $(TARGET)

$(LIB_STATIC): $(OBJECTS)
	rm -f $@
	$(AR) $@ $(OBJECTS)
	$(RANLIB) $@

ifneq ($(BUILD_TARGET),ELF)
$(LIB_IMPORT): $(LIB_DYNAMIC)
$(LIB_DYNAMIC): $(OBJECTS)
	$(LD) -e _dllmain --export-all-symbols --out-implib $(LIB_IMPORT) -o $@ $(OBJECTS) -L$(MONADIR)/lib -lmonapi-imp -lmonalibc-imp
	$(STRIP) $@

$(LIB_DYNAMIC5): $(LIB_DYNAMIC)
#	bzip2 -c $(LIB_DYNAMIC) > $@
	bim2bin in:$(LIB_DYNAMIC) out:$@ -osacmp -tek5 BS:0 eprm:z0
endif

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) -c -o $(<:.cpp=.o) $(CXXFLAGS) $(INCLUDE) $<

depend:
	$(CXX) -MM $(INCLUDE) $(CXXFLAGS) $(SOURCES) > dependencies

clean:
	rm -f $(TARGET) $(OBJECTS)

distclean: clean
	rm dependencies
	touch dependencies

install: $(TARGET)
	mkdir -p $(LIBDIR)
	$(INSTALL) -p -m 0644 $(LIB_STATIC) $(LIBDIR)/
ifneq ($(BUILD_TARGET),ELF)
	$(INSTALL) -p -m 0644 $(LIB_IMPORT) $(LIBDIR)/
	mkdir -p $(BINDIR)/LIBS
	$(INSTALL) -p $(LIB_DYNAMIC5) $(BINDIR)/LIBS/
endif
	mkdir -p $(INCDIR)/baygui
	mkdir -p $(INCDIR)/baygui/awt
	mkdir -p $(INCDIR)/baygui/awt/event
	mkdir -p $(INCDIR)/baygui/lang
	mkdir -p $(INCDIR)/baygui/util
	mkdir -p $(BINDIR)/APPS/
	mkdir -p $(BINDIR)/APPS/BAYGUI
	$(INSTALL) -p mplus_j12r_u.bdf $(BINDIR)/APPS/BAYGUI
	$(INSTALL) -p mplus_f12r.bdf $(BINDIR)/APPS/BAYGUI
	$(INSTALL) -p baygui.h $(INCDIR)
	$(INSTALL) -p -m 0644 ./baygui/awt/*.h $(INCDIR)/baygui/awt
	$(INSTALL) -p -m 0644 ./baygui/awt/event/*.h $(INCDIR)/baygui/awt/event
	$(INSTALL) -p -m 0644 ./baygui/lang/*.h $(INCDIR)/baygui/lang
	$(INSTALL) -p -m 0644 ./baygui/util/*.h $(INCDIR)/baygui/util
	$(INSTALL) -p BAYGUI.MSH $(BINDIR)/APPS
	$(INSTALL) -p BAYGUI.INI $(BINDIR)/APPS
	$(INSTALL) -p MONAWALL.BM5 $(BINDIR)/APPS

uninstall:
	rm -f $(LIBDIR)/$(LIB_STATIC)
ifneq ($(BUILD_TARGET),ELF)
	rm -f $(LIBDIR)/$(LIB_IMPORT)
	rm -f $(BINDIR)/LIBS/$(LIB_DYNAMIC5)
endif
	rm -f $(INCDIR)/baygui.h
	rm -f $(INCDIR)/baygui/awt/*.h
	rm -f $(INCDIR)/baygui/awt/event/*.h
	rm -f $(INCDIR)/baygui/lang/*.h
	rm -f $(INCDIR)/baygui/util/*.h
	rm -r $(INCDIR)/baygui/util
	rm -r $(INCDIR)/baygui/lang
	rm -r $(INCDIR)/baygui/awt/event
	rm -r $(INCDIR)/baygui/awt
	rm -r $(INCDIR)/baygui
	rm -f $(BINDIR)/APPS/BAYGUI.INI
	rm -f $(BINDIR)/APPS/BAYGUI.MSH
	rm -f $(BINDIR)/APPS/MONAWALL.BM5

-include dependencies
