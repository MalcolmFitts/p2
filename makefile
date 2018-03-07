CC = gcc
ARGS = -Wall -O2 -I .

all: clean sl pa dw be fe bbbserver

t: clean sl pa dw be fe test

sl: serverlog.c serverlog.h
	$(CC) $(ARGS) -o sllib.o -c serverlog.c

pa: parser.c parser.h
	$(CC) $(ARGS) -o palib.o -c parser.c

dw: datawriter.c datawriter.h
	$(CC) $(ARGS) -o dwlib.o -c datawriter.c

be: backend.c backend.h
	$(CC) $(ARGS) -o belib.o -c backend.c

fe: frontend.c frontend.h
	$(CC) $(ARGS) -o felib.o -c frontend.c

bbbserver: bbbserver.c datawriter.h parser.h serverlog.h backend.h frontend.h
	$(CC) $(ARGS) -o bbbserver bbbserver.c dwlib.o palib.o sllib.o belib.o felib.o

test: test.c datawriter.h parser.h serverlog.h backend.h frontend.h
	$(CC) $(ARGS) -o test test.c dwlib.o palib.o sllib.o belib.o felib.o

clean:
	rm -f *.o bbbserver test palib.o dwlib.o sllib.o belib.o felib.o *~
