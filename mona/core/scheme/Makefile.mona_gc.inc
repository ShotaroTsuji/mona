GC_CFLAGS  = -DMONASH_DONT_FREE_MEMORY -DUSE_MONA_GC #-DGC_TRACE
GC_LDFLAGS = 
GC_SOURCES = gc/GCRecord.cpp gc/gc.cpp
GC_OBJECTS = $(GC_SOURCES:.cpp=.o)

