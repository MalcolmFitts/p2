CC = gcc
ARGS = -g -pthread -Wall -O2 -I .

all: clean sl pa nd ne pkt dw gos be fe con bbbserver

sl: serverlog.c serverlog.h
	$(CC) $(ARGS) -o sllib.o -c serverlog.c

pa: parser.c parser.h
	$(CC) $(ARGS) -o palib.o -c parser.c

nd: node.c node.h
	$(CC) $(ARGS) -o nodelib.o -c node.c

ne: neighbor.c neighbor.h
	$(CC) $(ARGS) -o neighblib.o -c neighbor.c

pkt: packet.c packet.h
	$(CC) $(ARGS) -o pktlib.o -c packet.c

dw: datawriter.c datawriter.h
	$(CC) $(ARGS) -o dwlib.o -c datawriter.c

gos: gossip.c gossip.h
	$(CC) $(ARGS) -o goslib.o -c gossip.c

be: backend.c backend.h
	$(CC) $(ARGS) -o belib.o -c backend.c

fe: frontend.c frontend.h
	$(CC) $(ARGS) -o felib.o -c frontend.c

con: configlib.c configlib.h
	$(CC) $(ARGS) -o conlib.o -c configlib.c

bbbserver: bbbserver.c datawriter.h parser.h node.h packet.h serverlog.h backend.h frontend.h configlib.h neighbor.h gossip.h
	$(CC) $(ARGS) -o bbbserver bbbserver.c dwlib.o palib.o nodelib.o neighblib.o pktlib.o sllib.o belib.o felib.o conlib.o goslib.o

clean:
	rm -f *.o bbbserver send recv serv palib.o nodelib.o neighblib.o dwlib.o pktlib.o sllib.o belib.o felib.o conlib.o*~
