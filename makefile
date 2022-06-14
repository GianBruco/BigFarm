# Flag di compilazione
CC=gcc
CFLAGS=-g -Wall -O -std=gnu99
LDLIBS=-lm -lrt -pthread 

# eseguibili
EXECS=farm client

# da ignorare
.PHONY: clean

# di default make cerca di realizzare il primo target 
all: $(EXECS)

# non devo scrivere il comando associato ad ogni target 
# perché il default di make in questo caso va bene

farm: farm.o xerrori.o
client: client.o xerrori.o

# target che cancella eseguibili e file oggetto
clean:
	rm -f $(EXECS) *.o  

# target che crea l'archivio dei sorgenti
zip:
	zip ProgettoWolynski.zip makefile *.c *.h *.py *.md