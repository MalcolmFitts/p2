CC = gcc
ARGS = -Wall -O2 -I .

all: sl pa dw be bbbserver

sl: serverlog.c serverlog.h
	$(CC) $(ARGS) -o sllib.o -c serverlog.c

pa: parser.c parser.h
	$(CC) $(ARGS) -o palib.o -c parser.c

dw: datawriter.c datawriter.h
	$(CC) $(ARGS) -o dwlib.o -c datawriter.c

be: backend.c backend.h serverlog.h
	$(CC) $(ARGS) -o belib.o -c backend.c

bbbserver: bbbserver.c datawriter.h parser.h serverlog.h backend.h
	$(CC) $(ARGS) -o bbbserver bbbserver.c dwlib.o palib.o sllib.o belib.o

clean:
	rm -f *.o bbbserver palib.o dwlib.o sllib.o belib.o *~
