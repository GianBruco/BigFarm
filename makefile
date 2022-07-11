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

farm: farm.o libBigFarm.o
client: client.o libBigFarm.o

# target che cancella eseguibili e file oggetto
clean:
	rm -f $(EXECS) *.o  

# target che crea l'archivio dei sorgenti
zip:
	zip ProgettoWolynski.zip makefile *.c *.h *.py *.md