CC = gcc
LD = gcc
CFLAGS = -nostdlib -O2 -fbaserel -msmall-code -fomit-frame-pointer -mregparm -D__NOLIBBASE__ -DUSE_INLINE_STDARG
LDFLAGS = -s -fbaserel -nostartfiles -nostdlib
LIBS = 
OBJS = start.o main.o menu.o game.o loader.o lscm.o savelevel.o strutils.o selector.o savestate.o loadstate.o
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

game.o: game.c main.h selector.h loader.h lscm.h strutils.h version.h
loader.o: loader.c loader.h main.h selector.h strutils.h
loadstate.o: loadstate.c main.h selector.h savestate.h
lscm.o: lscm.c lscm.h main.h selector.h
main.o: main.c main.h selector.h menu.h strutils.h version.h savestate.h
menu.o: menu.c main.h selector.h game.h
savelevel.o: savelevel.c savelevel.h main.h selector.h loader.h
savestate.o: savestate.c main.h selector.h savestate.h
selector.o: selector.c main.h selector.h
start.o: start.c version.h
strutils.o: strutils.c strutils.h

