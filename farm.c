#include "xerrori.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


// struct contenente i parametri di input e output di ogni thread 
typedef struct {
  long sommaFile; // output
  int *indWork; // indice nel buffer (puntatore così non cambia per tutti i Worker)
  int *buffer;  // puntatore al buffer condiviso
	pthread_mutex_t *cmutex; // puntatore al mutex dei Worker
  sem_t *sem_free_slots;  // puntatore agli slot liberi
  sem_t *sem_data_items;  // puntatore ai dati inseriti
} dati;




// funzione eseguita dai thread Worker
void *tbody(void *arg)
{  
  dati *a = (dati *)arg;
  a->sommaFile = 0;
  int n;
  do {
    xsem_wait(a->sem_data_items,__LINE__,__FILE__); // attende la presenza di qualcosa nel buffer
		xpthread_mutex_lock(a->cmutex,__LINE__,__FILE__); // blocco la scrittura agli altri Worker
    n = a->buffer[*(a->indWork) % lBuffer]; // n è il file da analizzare
    *(a->indWork) +=1;  // vado avanti con l'indice nel buffer
		xpthread_mutex_unlock(a->cmutex,__LINE__,__FILE__); // rilascio il blocco mutex
    xsem_post(a->sem_free_slots,__LINE__,__FILE__); // rilascio il semaforo
    

    sommaFile+=n;


  } while(n!= -1);
  pthread_exit(NULL); 
} 



int main(int argc, char *argv[])
{
  // controlla numero argomenti
  if(argc<2) {
      printf("Uso: %s file [file ...] \n",argv[0]);
      return 1;
  }
	
  int opz; // opzioni utente
  int nopz = 1; // numero di opzioni usate (Conta il l'indice di argv dove iniziano i file)
  int nthread = 4;  // numero di thread Worker del processo master (default 4)
  int lBuffer = 8;  // lunghezza del buffer produttori/consumatori (default 8)
  int delay = 0;    // tempo in millisecondi tra una scrittura e l'altra del Master (default 0)
  long tot_somma = 0;

  // controllo le opzioni date dall'utente
  while((opz = getopt(argc, argv, "n:q:t:")) != -1) { 
    switch(opz) { 
      case 'n':
        nthread = atoi(optarg);
        nopz+=2;
        break;
      case 'q':
        lBuffer = atoi(optarg);
        nopz+=2;
        break;
      case 't':
        delay = atoi(optarg);
        nopz+=2;
        break;
      default:
        exit(EXIT_FAILURE);
    }
  }


//-------------------------------

  int buffer[lBuffer];  // inizializzo il buffer di dimensione lBuffer
  int pindex = 0; // indice del produttore
  pthread_t thr[nthread]; // thread Worker
  dati a[nthread];  // inizializzo contenuto dei thread Worker

  // dichiaro i semafori
  sem_t sem_free_slots, sem_data_items;
  xsem_init(&sem_free_slots,0,lBuffer,__LINE__,__FILE__); // gli slots liberi all'inizio sono tutti (la dimensione del buffer)
  xsem_init(&sem_data_items,0,0,__LINE__,__FILE__); // gli oggetti pronti all'inizio sono 0


  // generazione dei thread
  for(int i = 0; i<nthread;i++) {
    a[i].buffer = buffer;
    a[i].indCons = &indCons;
    a[i].cmutex = &cmutex;
    a[i].sem_data_items = &sem_data_items;
    a[i].sem_free_slots = &sem_free_slots;
    xpthread_create(&thr[i],NULL,tbody,a+1,__LINE__,__FILE__);
  }


  // lettura dei file
  for(int k = nopz;k<argc;k++) {
    FILE *f = fopen(argv[k],"r"); // apre IL PRIMO file
    if(f==NULL) {perror("Errore apertura file"); return 1;}

    // WORKER
    while(true) {
      e = fscanf(f,"%ld", &n);
      if(e!=1) break; // se il valore e' letto correttamente e==1
      assert(n>0);    // i valori del file devono essere positivi
      xsem_wait(&sem_free_slots,__LINE__,__FILE__); // blocco il semaforo
      buffer[pindex++ % lBuffer]= n; // scrivo sempre nel modulo lBuffer (Se pindex è 15 e lBuffer è 10 scrivo in 5)
      xsem_post(&sem_data_items,__LINE__,__FILE__); // sblocco il semaforo
      usleep(delay);
    }
  }
  // terminazione threads
  for(int i=0;i<nthread;i++) {
    xsem_wait(&sem_free_slots,__LINE__,__FILE__);
    buffer[pindex++ % lBuffer]= -1; // scrivo un valore illegale per far capire al consumatore che devo terminare
    xsem_post(&sem_data_items,__LINE__,__FILE__);
  }
  // join dei thread e calcolo risulato
  for(int i=0;i<nthread;i++) {
    xpthread_join(thr[i],NULL,__LINE__,__FILE__);
    tot_somma += a[i].sommaFile; // somma finale del thread principaòe
  }




  fprintf(stderr,"Trovati %d primi con somma %ld\n",tot_primi,tot_somma);





//printf("argc = %d\nargv[2] = %s\nargv[nopz] = %s\n",argc,argv[2],argv[nopz]);




  //printf("nthread = %d\nbuf = %d\ndel = %d\n",nthread,lBuffer,delay);

	return 0;
}