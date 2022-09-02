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
#include "libBigFarm.h" // funzione termina, xpthread ecc.

// variabili per la trasmissione tramite TCP localhost
#define HOST "127.0.0.1"
#define PORT 57937

#define MAX_STRING 100									// dimensione massima di una stringa (per conversioni da long)
pthread_mutex_t cmutex = PTHREAD_MUTEX_INITIALIZER; 	// inizializzo la mutex del Worker

volatile bool fine = false;		// booleano per la terminazione del programma con SIGINT

// Struct per i threads
typedef struct {
	int *BuffSize;				// grandezza buffer
	char **buffer;				// puntatore al buffer condiviso
	pthread_mutex_t *cmutex;	// puntatore alla mutex dei Worker
	sem_t *sem_free_slots;		// semaforo slot liberi
	sem_t *sem_data_items;		// semaforo dati inseriti
} dati;



// funzione per l'invio di coppie [somma, nomeFile] tramite socket TCP
void invioCoppia(long s, char *nome)
{
	int fd_skt = 0;					// file descriptor per l'invio dei dati
	int tmp;						// variabile usata per l'invio dei dati
	char s_somma[MAX_STRING];		// stringa nella quale verrà inserito il long
	size_t e;						// variabile per il valore di ritorno
	struct sockaddr_in serv_addr;

	// creo socket
	if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		termina("Creazione socket farm fallita");

	serv_addr.sin_family = AF_INET;	  				// assegnazione dell'indirizzo
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
		termina("Errore write lunghezza stringa somma");

	// invio la somma
	e = writen(fd_skt, s_somma, (sizeof(char) * (strlen(s_somma))));
	if (e != (sizeof(char) * (strlen(s_somma))))
		termina("Errore write stringa somma");

	// invio la lunghezza di nomefile
	tmp = htonl(strlen(nome));
	e = writen(fd_skt, &tmp, sizeof(tmp));
	if (e != sizeof(tmp))
		termina("Errore write lunghezza stringa nome file");

	// invio il nome del file
	e = writen(fd_skt, nome, (sizeof(char) * (strlen(nome))));
	if (e != (sizeof(char) * (strlen(nome))))
		termina("Errore write stringa nome file");

	// chiudo socket
	if (close(fd_skt) < 0)
		perror("Errore nella chiusura del socket");
}


// handler gestore di SIGINT
void handlerInt(int numSegnale)
{
	if (numSegnale == SIGINT)
	{
		puts("segnale SIGINT arrivato!");
		fine = true;
	}
}


// funzione eseguita dai thread Worker
void *tbody(void *arg)
{
	dati *a = (dati *)arg; // casting della struct del thread
	char *nomeFile = malloc(sizeof(char) * MAX_STRING);		// alloco la stringa che conterrà il nome del file da analizzare
	
	while (1)
	{
		xsem_wait(a->sem_data_items, __LINE__, __FILE__);	// attende la presenza di qualcosa nel buffer
		xpthread_mutex_lock(a->cmutex, __LINE__, __FILE__);	// blocco la scrittura agli altri Worker
		int h = 0;
		int contafine = 0;

		// controllo quanti slot del buffer hanno la stringa "FINE"
		for (int j = 0; j < *(a->BuffSize); j++)
			if (strcmp("FINE", a->buffer[j]) == 0)
				contafine++;

		// se tutti gli slot del buffer ha la stringa "FINE" esco dal thread
		if (contafine == *(a->BuffSize))
		{
			xpthread_mutex_unlock(a->cmutex, __LINE__, __FILE__);
			xsem_post(a->sem_data_items, __LINE__, __FILE__);
			break;
		}

		// cerco una posizione nel buffer contenente un file
		while ((h < *(a->BuffSize)) && (strcmp(a->buffer[h], "LIBERO") == 0 || strcmp(a->buffer[h], "FINE") == 0))
		{
			h++;
		}

		if (h < *(a->BuffSize))
		{
			// copio il nome del file dal buffer alla stringa nomeFile
			nomeFile = realloc(nomeFile, sizeof(char) * strlen(a->buffer[h]) + 1);
			nomeFile = strcpy(nomeFile, a->buffer[h]);

			// segnalo di aver liberato lo slot scrivendo "LIBERO"
			a->buffer[h] = realloc(a->buffer[h], sizeof(char) * (strlen("LIBERO") + 1));
			a->buffer[h] = strcpy(a->buffer[h], "LIBERO");

			xpthread_mutex_unlock(a->cmutex, __LINE__, __FILE__);
			xsem_post(a->sem_free_slots, __LINE__, __FILE__);

			// Leggo il file passato come argomento
			FILE *f = fopen(nomeFile, "rb");
			if (f == NULL)
				continue;

			// sommo i long presenti nel file
			long somma = 0;
			long numero;
			int i = 0;
			while (fread(&numero, sizeof(long), 1, f) == 1)
			{
				somma += (i * numero);
				i++;
			}
			invioCoppia(somma, nomeFile); // funzione di invio coppie tramite socket
			fclose(f);					  // chiudo il file
		}
		else
		{
			xpthread_mutex_unlock(a->cmutex, __LINE__, __FILE__);
			xsem_post(a->sem_data_items, __LINE__, __FILE__);
		}
	}
	free(nomeFile);
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	// controlla numero argomenti
	if (argc < 2)
	{
		printf("Uso: %s [-n -q -t] [file ...] \n", argv[0]);
		return 1;
	}

	int opz;		 // numero di opzioni usate dall'utente
	int nopz = 1;	 // indice di argv dove iniziano i file
	int nthread = 4; // numero di thread Worker del processo master (default 4)
	int lBuffer = 8; // lunghezza del buffer produttori/consumatori (default 8)
	int delay = 0;	 // tempo in millisecondi tra una scrittura e l'altra del Master (default 0)

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

	// controllo i valori di input
	if (nthread < 1)
		termina("Numero thread troppo piccolo!");
	if (lBuffer < 1)
		termina("Buffer troppo piccolo!");
	if (delay < 0)
		termina("Delay troppo piccolo!");

	struct sigaction sa;			// definisco un signal Handler per SIGINT
	sa.sa_handler = &handlerInt;	// definisco la funzione da usare per il sengnale
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;		// restart system calls
	sigaction(SIGINT, &sa, NULL);

	char *buffer[lBuffer];			// inizializzo il buffer di dimensione lBuffer

	// inizializzo il buffer
	for (int i = 0; i < lBuffer; i++)
	{
		buffer[i] = malloc(sizeof(char) * (strlen("LIBERO") + 1));
		strcpy(buffer[i], "LIBERO");
	}

	pthread_t thr[nthread]; // array di thread Worker
	dati a[nthread];		// inizializzo contenuto dei thread Worker

	sem_t sem_free_slots, sem_data_items;						// dichiaro i semafori
	xsem_init(&sem_free_slots, 0, lBuffer, __LINE__, __FILE__); // gli slots liberi all'inizio sono uguali alla dimensione del buffer
	xsem_init(&sem_data_items, 0, 0, __LINE__, __FILE__);		// gli oggetti pronti all'inizio sono 0

	// generazione dei thread
	for (int i = 0; i < nthread; i++)
	{
		a[i].buffer = buffer;
		a[i].cmutex = &cmutex;
		a[i].BuffSize = &lBuffer;
		a[i].sem_data_items = &sem_data_items;
		a[i].sem_free_slots = &sem_free_slots;
		xpthread_create(&thr[i], NULL, tbody, a, __LINE__, __FILE__);
	}

	// lettura dei file
	int k = nopz; // inizio dei file
	while ((k < argc) && fine == false)
	{
		xsem_wait(&sem_free_slots, __LINE__, __FILE__);
		xpthread_mutex_lock(a->cmutex, __LINE__, __FILE__);

		// se non è arrivato un segnale SIGINT
		if (fine == false)
		{
			int i = 0;
			while (strcmp(a->buffer[i], "LIBERO") != 0)
			{
				i++;
			}
			buffer[i] = realloc(buffer[i], sizeof(char) * (strlen(argv[k]) + 1));
			strcpy(buffer[i], argv[k]); // scrivo nel buffer il nome del file
			k++;
		}
		xpthread_mutex_unlock(a->cmutex, __LINE__, __FILE__);
		xsem_post(&sem_data_items, __LINE__, __FILE__); // sblocco il semaforo

		usleep(delay * 1000); // aspetto prima di inserire un nuovo file nel buffer
	}

	// Scrivo "FINE" negli slot del buffer
	for (int j = 0; j < lBuffer; j++)
	{
		xsem_wait(&sem_free_slots, __LINE__, __FILE__);
		xpthread_mutex_lock(a->cmutex, __LINE__, __FILE__);
		int i = 0;
		while (strcmp(a->buffer[i], "LIBERO") != 0)
		{
			i++;
		}
		buffer[i] = realloc(buffer[i], sizeof(char) * (strlen("FINE") + 1));
		strcpy(buffer[i], "FINE");

		xpthread_mutex_unlock(a->cmutex, __LINE__, __FILE__);
		xsem_post(&sem_data_items, __LINE__, __FILE__);
	}
	
	for (int i = 0; i < nthread; i++)
	{
		xpthread_join(thr[i], NULL, __LINE__, __FILE__);
	}

	// libero il buffer
	for (int i = 0; i < lBuffer; i++)
	{
		free(buffer[i]);
	}

	sem_close(&sem_free_slots);
	sem_close(&sem_data_items);
	xpthread_mutex_destroy(&cmutex, __LINE__, __FILE__);
	return 0;
}