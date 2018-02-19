CC = gcc
ARGS = -Wall -O2 -I .

all: pa dw bbbserver testclient

pa: parser.c parser.h
	$(CC) $(ARGS) -o palib.o -c parser.c

dw: datawriter.c datawriter.h
	$(CC) $(ARGS) -o dwlib.o -c datawriter.c

bbbserver: bbbserver.c datawriter.h parser.h
	$(CC) $(ARGS) -o bbbserver bbbserver.c dwlib.o palib.o

testclient: testclient.c
	$(CC) $(ARGS) -o testclient testclient.c

clean:
	rm -f *.o bbbserver testclient *~
