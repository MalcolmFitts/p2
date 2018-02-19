CC = gcc
ARGS = -Wall -O2 -I .

all: pa dw bbbserver

pa: parser.c parser.h
	$(CC) $(ARGS) -o palib.o -c parser.c

dw: datawriter.c datawriter.h
	$(CC) $(ARGS) -o dwlib.o -c datawriter.c

bbbserver: bbbserver.c datawriter.h parser.h
	$(CC) $(ARGS) -o bbbserver bbbserver.c dwlib.o palib.o

clean:
	rm -f *.o bbbserver palib.o dwlib.o *~
