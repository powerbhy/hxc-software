#CC=i386-pc-linux-gcc
CC=gcc
AR=ar

UNAME := $(shell uname)

BASEDIR=../sources
INCLUDES = -I$(BASEDIR)/ -I ../../../libhxcfe/trunk/sources -I ../../../libusbhxcfe/trunk/sources

CFLAGS=-O3 $(INCLUDES)
LDFLAGS= -g -shared -lc -lm  -ldl

ifeq ($(UNAME), Linux)
CFLAGS += -fPIC
endif

ifeq ($(UNAME), Darwin)
CFLAGS += -arch i386 -mmacosx-version-min=10.5
LDFLAGS += -arch i386 -mmacosx-version-min=10.5
endif

EXEC=libhxcadaptor.a

all: $(EXEC)

libhxcadaptor.a: libhxcadaptor.o fs.o
	$(AR) r $@    $^
	cp $@ ../../../build

libhxcadaptor.o: $(BASEDIR)/libhxcadaptor.c
	$(CC) -o $@ -c $< $(CFLAGS)

fs.o: $(BASEDIR)/fs.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
