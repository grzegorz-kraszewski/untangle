CC = gcc
LD = ld
CFLAGS = -noixemul -nostdlib -O2 -fomit-frame-pointer -m68000 -mregparm=2 -D__NOLIBBASE__ -DUSE_INLINE_STDARG
LDFLAGS = -noixemul -nostartfiles -nostdlib
LIBS = 
OBJS = start.o main.o menu.o game.o loader.o lscm.o
EXE = Untangle

all: $(OBJS)
	@echo "Linking $(EXE)..."
	@$(LD) $(LDFLAGS) -o $(EXE) $^ $(LIBS)

clean:
	rm -f $(OBJS) $(EXE)

start.o: start.c
	@echo "Compiling $@..."
	@$(CC) $(CFLAGS) -fwritable-strings -c -o $@ $<

%.o: %.c
	@echo "Compiling $@..."
	@$(CC) $(CFLAGS) -c -o $@ $<

# dependencies

game.o: game.c main.h loader.h
loader.o: loader.c loader.h main.h
lscm.o: lscm.c lscm.h main.h
main.o: main.c main.h menu.h
menu.o: menu.c main.h game.h
start.o: start.c
