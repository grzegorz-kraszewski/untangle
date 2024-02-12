CC = gcc
LD = gcc
CFLAGS = -nostdlib -O2 -fbaserel -msmall-code -mregparm -D__NOLIBBASE__ -DUSE_INLINE_STDARG
LDFLAGS = -s -fbaserel -nostartfiles -nostdlib
LIBS = 
OBJS = start.o main.o menu.o game.o loader.o lscm.o savelevel.o strutils.o
EXE = Untangle

all: $(OBJS)
	@echo "Linking $(EXE)..."
	@$(LD) $(LDFLAGS) -o $(EXE) $^ $(LIBS)
	@List P=$(EXE) NODATES

clean:
	rm -f $(OBJS) $(EXE)

%.o: %.c
	@echo "Compiling $@..."
	@$(CC) $(CFLAGS) -c -o $@ $<

# dependencies

game.o: game.c main.h loader.h lscm.h strutils.h version.h
loader.o: loader.c loader.h main.h strutils.h
lscm.o: lscm.c lscm.h main.h
main.o: main.c main.h menu.h strutils.h version.h
menu.o: menu.c main.h game.h
savelevel.o: savelevel.c savelevel.h main.h loader.h
start.o: start.c version.h
strutils.o: strutils.c strutils.h
