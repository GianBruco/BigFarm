#define _GNU_SOURCE     // permette di usare estensioni GNU
#include <stdio.h>      // permette di usare scanf printf etc ...
#include <stdlib.h>     // conversioni stringa exit() etc ...
#include <stdbool.h>    // gestisce tipo bool
#include <assert.h>     // permette di usare la funzione ass
#include <string.h>     // funzioni per stringhe
#include <errno.h>      // richiesto per usare errno
#include <unistd.h>     // 
#include <arpa/inet.h>  // conversione dati in formato inet
#include <sys/socket.h> // permette l'utilizzo dei socket
#include "xerrori.h"    // funzione termina, xpthread ecc.

#define HOST "127.0.0.1"
#define PORT 57937



/* Read "n" bytes from a descriptor */
ssize_t readn(int fd, void *ptr, size_t n) {  
   size_t   nleft;
   ssize_t  nread;
 
   nleft = n;
   while (nleft > 0) {
     if((nread = read(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount read so far */
     } else if (nread == 0) break; /* EOF */
     nleft -= nread;
     ptr   += nread;
   }
   return(n - nleft); /* return >= 0 */
}


/* Write "n" bytes to a descriptor */
ssize_t writen(int fd, void *ptr, size_t n) {  
   size_t   nleft;
   ssize_t  nwritten;
 
   nleft = n;
   while (nleft > 0) {
     if((nwritten = write(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount written so far */
     } else if (nwritten == 0) break; 
     nleft -= nwritten;
     ptr   += nwritten;
   }
   return(n - nleft); /* return >= 0 */
}



int main(int argc, char const* argv[])
{
  //if(argc<2) {
  //  printf("Uso\n\t%s inizio fine\n", argv[0]);
  //  exit(1);
  //}
  int fd_skt = 0;
  size_t e;
  int tmp;
  struct sockaddr_in serv_addr; // default

  // Creo socket
  if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0) termina("Creazione socket fallita");


  // assegna indirizzo
  serv_addr.sin_family = AF_INET;
  // il numero della porta deve essere convertito 
  // in network order 
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = inet_addr(HOST);
  // Apro la connessione con il server
  if (connect(fd_skt, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) termina("Errore apertura connessione");

  puts("Invio la tipologia di richiesta");
  if(argc==1) {
    tmp = htons(1); // invio 1 per la stampa di tutte le coppie [somma, nomeFile]
    e = writen(fd_skt,&tmp,sizeof(tmp));
    if(e!=sizeof(int)) termina("Errore write");
    puts("Ho inviato 1");

  }
  else{
    tmp = htons(2); // invio 2 per la richiesta una ricerca per un dato long
    e = writen(fd_skt,&tmp,sizeof(tmp));
    if(e!=sizeof(int)) termina("Errore write");
    puts("Ho inviato 2");

    puts("invio il numero di long");
    tmp = htons(argc-1); // invio 2 per la richiesta una ricerca per un dato long
    e = writen(fd_skt,&tmp,sizeof(tmp));
    if(e!=sizeof(int)) termina("Errore write");
    puts("Ho inviato argc-1");

    fprintf(stdout,"Invio %d long da leggere",argc-1);
    for(int j=1;j<argc;j++) {
      tmp = htonl(atol(argv[j]));
      fprintf(stdout,"invio %s\n",argv[j]);
      e = writen(fd_skt,&tmp,sizeof(tmp));
      if(e!=sizeof(long)) termina("Errore write");
    }

  }
  


  if(close(fd_skt)<0)
    perror("Errore chiusura socket");
  puts("Connessione terminata"); 
  return 0;
}