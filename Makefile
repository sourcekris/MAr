
# MCr Makefile

# Variables

BIN_DIR = ../bin

OBJS = algo.o archive.o ari.o bitio.o bwt.o crc.o delta.o hufblock.o \
       huffman.o lz77.o mtf.o ppm.o rle.o

EXES = mar mcr

CP = cp -f

# Rules

all: $(EXES)

install: $(EXES)
	$(CP) $(EXES) $(BIN_DIR)
	cd $(BIN_DIR) && chmod 755 $(EXES)

clean:
	$(RM) *~ *.o $(EXES)

mrproper: clean
	cd $(BIN_DIR) && $(RM) $(EXES)

# General

CC      = gcc
CFLAGS  = 
LDFLAGS = -lm

# Warnings

CFLAGS  += -ansi -pedantic # -Wall

# CFLAGS  += -W -Wbad-function-cast -Wcast-align -Wwrite-strings \
#            -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes \
#            -Wmissing-declarations

# Optimize

CFLAGS  += -O3 -funroll-loops -fomit-frame-pointer
LDFLAGS += -s

# Debug

# CFLAGS  += -g -DDEBUG

# Profile

# CFLAGS  += -pg
# LDFLAGS += -pg

# Dependencies

mar: $(OBJS) debug.o mar.o
	$(CC) $(LDFLAGS) -o mar $(OBJS) debug.o mar.o

mcr: $(OBJS) debug.o mcr.o
	$(CC) $(LDFLAGS) -o mcr $(OBJS) debug.o mcr.o

algo.o: algo.c algo.h types.h bitio.h bwt.h crc.h debug.h delta.h \
        hufblock.h lz77.h mtf.h ppm.h rle.h

archive.o: archive.c archive.h types.h bitio.h algo.h bwt.h crc.h \
           debug.h delta.h

ari.o: ari.c ari.h types.h bitio.h debug.h

bitio.o: bitio.c bitio.h types.h debug.h

bwt.o: bwt.c bwt.h types.h algo.h bitio.h debug.h hufblock.h mtf.h \
       rle.h

crc.o: crc.c crc.h types.h

debug.o: debug.c debug.h types.h

delta.o: delta.c delta.h types.h algo.h

hufblock.o: hufblock.c hufblock.h types.h algo.h bitio.h bwt.h debug.h \
            huffman.h mtf.h

hufblock_normal.o: hufblock_normal.c hufblock.h types.h algo.h bitio.h \
                   bwt.h debug.h huffman.h mtf.h

hufblock_simple.o: hufblock_simple.c hufblock.h types.h algo.h bitio.h \
                   bwt.h debug.h huffman.h mtf.h

huffman.o: huffman.c types.h huffman.h bitio.h debug.h

lz77.o: lz77.c lz77.h types.h algo.h bitio.h debug.h huffman.h

mar.o: mar.c mar.h types.h algo.h archive.h bitio.h bwt.h crc.h \
       debug.h delta.h

mcr.o: mcr.c mcr.h types.h algo.h

mtf.o: mtf.c mtf.h types.h debug.h

ppm.o: ppm.c ppm.h types.h algo.h ari.h bitio.h debug.h

rle.o: rle.c rle.h types.h algo.h bwt.h hufblock.h debug.h

