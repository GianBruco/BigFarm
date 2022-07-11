#define _GNU_SOURCE		// permette di usare estensioni GNU
#include <stdio.h>		// permette di usare scanf printf etc ...
#include <stdlib.h>		// conversioni stringa exit() etc ...
#include <stdbool.h>	// gestisce tipo bool
#include <assert.h>		// permette di usare la funzione ass
#include <string.h>		// funzioni per stringhe
#include <errno.h>		// richiesto per usare errno
#include <unistd.h>		//
#include <arpa/inet.h>	// conversione dati in formato inet
#include <sys/socket.h>	// permette l'utilizzo dei socket
#include "libBigFarm.h"	// funzione termina, xpthread ecc.

#define HOST "127.0.0.1"
#define PORT 57937
#define MAX_STRING 255

int main(int argc, char const *argv[])
{
	int fd_skt = 0;			// file descriptor per l'invio di dati
	size_t e;				// variabile di ritorno (per errori)
	struct sockaddr_in serv_addr;

	// creo socket
	if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		termina("Creazione socket client fallita");

	serv_addr.sin_family = AF_INET;		// assegno l'indirizzo
	serv_addr.sin_port = htons(PORT);	// converto il numero di porta
	serv_addr.sin_addr.s_addr = inet_addr(HOST);

	// apro la connessione
	if (connect(fd_skt, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		termina("Errore apertura connessione client");

	// puts("Invio la tipologia di richiesta");
	int scelta;
	if (argc == 1)
	{
		scelta = htonl(1);		// invio 1 per la stampa di tutte le coppie [somma, nomeFile]
		e = writen(fd_skt, &scelta, sizeof(scelta));
		if (e != sizeof(int))
			termina("Errore write scelta client");

		int nc;					// numero di coppie in arrivo dal server
		e = readn(fd_skt, &nc, sizeof(nc));
		if (e != sizeof(nc))
			termina("Errore read somma");
		long ncoppie = ntohl(nc);

		// recupero le coppie inviate dal server
		for (int h = 0; h < ncoppie; h++)
		{
			// leggo la lunghezza della stringa somma
			int s;
			e = readn(fd_skt, &s, sizeof(s));		
			if (e != sizeof(s))
				termina("Errore read lung somma");
			long lungSomma = ntohl(s);
			
			// leggo la stringa somma
			char sommafile[lungSomma];
			e = readn(fd_skt, &sommafile, (sizeof(char) * lungSomma));
			if (e != (sizeof(char) * lungSomma))
				termina("Errore read somma");
			sommafile[lungSomma] = '\0';

			// leggo la lunghezza del nome file
			int dimen;
			e = readn(fd_skt, &dimen, sizeof(dimen));
			if (e != sizeof(dimen))
				termina("Errore read lung nomefile");
			long lungnome = ntohl(dimen);

			// leggo il nome del file
			char nomefile[lungnome];
			e = readn(fd_skt, &nomefile, (sizeof(char) * lungnome));
			if (e != (sizeof(char) * lungnome))
				termina("Errore read nomefile");
			nomefile[lungnome] = '\0';

			fprintf(stdout, "%s %s\n", sommafile, nomefile);
		}
	}
	else
	{
		scelta = htonl(2);		// invio 2 per la ricerca di un file con una data somma
		e = writen(fd_skt, &scelta, sizeof(scelta));
		if (e != sizeof(int))
			termina("Errore write scelta client");

		// invio il numero di long
		int nlong;
		nlong = htonl(argc - 1);
		e = writen(fd_skt, &nlong, sizeof(nlong));
		if (e != sizeof(nlong))
			termina("Errore write numero longs");

		// invio tutti i long ricevuti da stdin
		int tmp;
		char number[MAX_STRING];
		for (int j = 1; j < argc; j++)
		{
			// invio la dimensione del long (in formato stringa)
			tmp = htonl(strlen(argv[j]));
			e = writen(fd_skt, &tmp, sizeof(tmp));
			if (e != sizeof(tmp))
				termina("Errore write dimensione long");

			// invio il long in formato stringa
			strncpy(number, argv[j], strlen(argv[j]));
			e = writen(fd_skt, number, (sizeof(char) * (strlen(number))));
			if (e != (sizeof(char) * (strlen(number))))
				termina("Errore write long");
		}

		for (int u = 1; u < argc; u++)
		{
			// leggo la lunghezza della stringa output
			int s;
			e = readn(fd_skt, &s, sizeof(s));		
			if (e != sizeof(s))
				termina("Errore read lung somma");
			long lungOut = ntohl(s);
			
			// leggo la stringa somma
			char strOut[lungOut];
			e = readn(fd_skt, &strOut, (sizeof(char) * lungOut));
			if (e != (sizeof(char) * lungOut))
				termina("Errore read somma");
			strOut[lungOut] = '\0';

			fprintf(stdout, "%s\n", strOut);
		}
		
	}

	if (close(fd_skt) < 0)
		perror("Errore chiusura socket");

	// puts("Connessione terminata");
	return 0;
}