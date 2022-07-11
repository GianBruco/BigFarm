#define _GNU_SOURCE		// permette di usare estensioni GNU
#include <stdio.h>		// permette di usare scanf printf etc ...
#include <stdlib.h>		// conversioni stringa exit() etc ...
#include <stdbool.h>	// gestisce tipo bool
#include <assert.h>		// permette di usare la funzione ass
#include <string.h>		// funzioni per stringhe
#include <errno.h>		// richiesto per usare errno
#include <unistd.h>		//
#include <arpa/inet.h>	// conversione dati in formato inet
#include <sys/socket.h> // permette l'utilizzo dei socket
#include "libBigFarm.h"	// funzione termina, xpthread ecc.

// variabili per la trasmissione tramite TCP localhost
#define HOST "127.0.0.1"
#define PORT 57937

#define MAX_STRING 100	// dimensione massima di una stringa (per conversioni da long)

volatile bool fine = false;		// booleano per la terminazione del programma con SIGINT

// struct con parametri input e output dei thread
typedef struct
{
	int *BuffSize;				// grandezza buffer
	long sommaFile;				// somma totale dei long
	int *indWork;				// puntatore all'indice corrente nel buffer
	char **buffer;				// puntatore al buffer condiviso
	pthread_mutex_t *cmutex;	// puntatore alla mutex dei Worker
	sem_t *sem_free_slots;		// semaforo slot liberi
	sem_t *sem_data_items;		// semaforo dati inseriti
} dati;

// funzione per l'invio di coppie [somma, nomeFile] tramite socket TCP
void invioCoppia(long s, char *nome)
{
	int fd_skt = 0;				// file descriptor per l'invio dei dati
	int tmp;					// variabile usata per l'invio dei dati
	char s_somma[MAX_STRING];	// stringa nella quale verrà inserito il long
	size_t e;					// variabile per il valore di ritorno
	struct sockaddr_in serv_addr;

	// creo socket
	if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		termina("Creazione socket farm fallita");

	serv_addr.sin_family = AF_INET;					// assegnazione dell'indirizzo
	serv_addr.sin_port = htons(PORT); 				// conversione della porta in network order
	serv_addr.sin_addr.s_addr = inet_addr(HOST);

	// apro la connessione con il server
	if (connect(fd_skt, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		termina("Errore apertura connessione farm");

	// invio il numero 3 per specificare il tipo di connessione
	tmp = htonl(3);
	e = writen(fd_skt, &tmp, sizeof(tmp));
	if (e != sizeof(tmp))
		termina("Errore write scelta");

	// invio la lunghezza della stringa somma
	sprintf(s_somma, "%ld", s);		// trasformo il long in stringa
	tmp = htonl(strlen(s_somma));
	e = writen(fd_skt, &tmp, sizeof(tmp));
	if (e != sizeof(tmp))
		termina("Errore write lung somma");

	// invio la somma
	e = writen(fd_skt, s_somma, (sizeof(char) * (strlen(s_somma))));
	if (e != (sizeof(char) * (strlen(s_somma))))
		termina("Errore write somma");

	// invio la lunghezza di nomefile
	tmp = htonl(strlen(nome));
	e = writen(fd_skt, &tmp, sizeof(tmp));
	if (e != sizeof(tmp))
		termina("Errore write lung nome");

	// invio il nome del file
	e = writen(fd_skt, nome, (sizeof(char) * (strlen(nome))));
	if (e != (sizeof(char) * (strlen(nome))))
		termina("Errore write nome");

	// chiudo socket
	if (close(fd_skt) < 0)
		perror("Errore chiusura socket");
}

// handler gestore di SIGINT
void handlerInt(int numSegnale)
{
	if (numSegnale == SIGINT)
	{
		puts("SIGINT arrivato!");
		fine = true;
	}
}

// funzione eseguita dai thread Worker
void *tbody(void *arg)
{
	dati *a = (dati *)arg;	// struct dati con le variabili del caso
	a->sommaFile = 0;		// inizializzo la somma totale del file binario
	char *nomeFile;			// nome del file binario
	xsem_wait(a->sem_data_items, __LINE__, __FILE__);		// attende la presenza di qualcosa nel buffer
	xpthread_mutex_lock(a->cmutex, __LINE__, __FILE__);		// blocco la scrittura agli altri Worker
	nomeFile = a->buffer[*(a->indWork) % *(a->BuffSize)];	// leggo il nome del file da analizzare
	*(a->indWork) += 1;										// vado avanti con l'indice nel buffer
	xpthread_mutex_unlock(a->cmutex, __LINE__, __FILE__);
	xsem_post(a->sem_free_slots, __LINE__, __FILE__);

	// controllo se il nome file si chiama "FINE" e quindi segna la terminazione del programma
	if (strcmp(nomeFile, "FINE") == 0)
		pthread_exit(NULL);
	else
	{
		// Leggo il file passato come argomento
		FILE *f = xfopen(nomeFile, "rb", __LINE__, __FILE__);
		if (f == NULL)
		{
			perror("Errore apertura file");
			pthread_exit(NULL);
		}

		// sommo i long presenti nel file
        long somma = 0;
		long numero;
		int i = 0;
		while (fread(&numero, sizeof(long), 1, f) == 1)
		{
			somma += (i * numero);
			i++;
		}
        a->sommaFile=somma;
		fclose(f);		// chiudo il file
		
		invioCoppia(a->sommaFile, nomeFile);	// funzione di invio coppie tramite socket 
		pthread_exit(NULL);						// chiudo il thread
	}
}

int main(int argc, char *argv[])
{
	// controlla numero argomenti
	if (argc < 2)
	{
		printf("Uso: %s file [file ...] \n", argv[0]);
		return 1;
	}

	int opz;			// numero di opzioni usate dall'utente
	int nopz = 1;		// indice di argv dove iniziano i file
	int nthread = 4;	// numero di thread Worker del processo master (default 4)
	int lBuffer = 8;	// lunghezza del buffer produttori/consumatori (default 8)
	int delay = 0;		// tempo in millisecondi tra una scrittura e l'altra del Master (default 0)

	// controllo le opzioni date dall'utente
	while ((opz = getopt(argc, argv, "n:q:t:")) != -1)
	{
		switch (opz)
		{
		case 'n':
			nthread = strtol(optarg, NULL, 10);
			nopz += 2;
			break;
		case 'q':
			lBuffer = strtol(optarg, NULL, 10);
			nopz += 2;
			break;
		case 't':
			delay = strtol(optarg, NULL, 10);
			nopz += 2;
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}

	// controllo i valori
	assert(nthread >= 1);
	assert(lBuffer >= 1);
	assert(delay >= 0);

	
	struct sigaction sa;			// definisco un signal Handler per SIGINT
	sa.sa_handler = &handlerInt;	// definisco la funzione da usare per il sengnale
	sigaction(SIGINT, &sa, NULL);

	char *buffer[lBuffer];								// inizializzo il buffer di dimensione lBuffer
	for (int i = 0; i < nthread; i++)
	{
		buffer[i] = malloc(sizeof(char) * 255);
	}
	int indWork = 0;									// indice del Worker
	int indMast = 0;									// indice del Master
	pthread_mutex_t cmutex = PTHREAD_MUTEX_INITIALIZER; // inizializzo la mutex del Worker
	pthread_t thr[nthread];								// array di thread Worker
	dati a[nthread];									// inizializzo contenuto dei thread Worker

	sem_t sem_free_slots, sem_data_items;						// dichiaro i semafori
	xsem_init(&sem_free_slots, 0, lBuffer, __LINE__, __FILE__);	// gli slots liberi all'inizio sono uguali alla dimensione del buffer
	xsem_init(&sem_data_items, 0, 0, __LINE__, __FILE__);		// gli oggetti pronti all'inizio sono 0

	// generazione dei thread
	for (int i = 0; i < nthread; i++)
	{
		a[i].buffer = buffer;
		a[i].indWork = &indWork;
		a[i].cmutex = &cmutex;
		a[i].BuffSize = &lBuffer;
		a[i].sem_data_items = &sem_data_items;
		a[i].sem_free_slots = &sem_free_slots;
		xpthread_create(&thr[i], NULL, tbody, a + 1, __LINE__, __FILE__);
	}

	// lettura dei file
	int k = nopz; // inizio dei file
	while ((k < argc) && fine==false)
	{
		xsem_wait(&sem_free_slots, __LINE__, __FILE__);
		// controllo se nel buffer non è presente la stringa "FINE"
		if (fine==false) 
		{
			buffer[indMast % lBuffer] = argv[k]; // scrivo in modulo lBuffer il nome del file
			indMast += 1;
		}
		else
		{
			puts("Programma interrotto!");
		}
		xsem_post(&sem_data_items, __LINE__, __FILE__); // sblocco il semaforo
		k++;											// contatore dei file input
		usleep(delay * 1000);							// aspetto prima di inserire un nuovo file nel buffer
	}

	// scrivo il carattere fine
	for (int i = 0; i < nthread; i++)
	{
		xsem_wait(&sem_free_slots, __LINE__, __FILE__);
		buffer[indMast % lBuffer] = "FINE"; // scrivo un valore illegale per far capire al consumatore che devo terminare
		indMast += 1;
		xsem_post(&sem_data_items, __LINE__, __FILE__);
	}

	for (int i = 0; i < nthread; i++)
		xpthread_join(thr[i], NULL, __LINE__, __FILE__);

	sem_close(&sem_free_slots);
	sem_close(&sem_data_items);
	xpthread_mutex_destroy(&cmutex, __LINE__, __FILE__);

	return 0;
}