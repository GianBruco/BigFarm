#include "xerrori.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


// struct contenente i parametri di input e output di ogni thread 
typedef struct {
  int *BuffSize; // randezza buffer
  long sommaFile; // output
  int *indWork; // indice nel buffer (puntatore così non cambia per tutti i Worker)
  char* *buffer;  // puntatore al buffer condiviso
	pthread_mutex_t *cmutex; // puntatore al mutex dei Worker
  sem_t *sem_free_slots;  // puntatore agli slot liberi
  sem_t *sem_data_items;  // puntatore ai dati inseriti
} dati;


// funzione eseguita dai thread Worker
void *tbody(void *arg)
{  
  dati *a = (dati *)arg;
  a->sommaFile = 0;
  char * nomeFile; 
  xsem_wait(a->sem_data_items,__LINE__,__FILE__); // attende la presenza di qualcosa nel buffer
	xpthread_mutex_lock(a->cmutex,__LINE__,__FILE__); // blocco la scrittura agli altri Worker
  fprintf(stdout,"posizione: %d\n",(*(a->indWork) % *(a->BuffSize)));
  nomeFile = a->buffer[*(a->indWork) % *(a->BuffSize)]; // n è il file da analizzare
  *(a->indWork)+=1;  // vado avanti con l'indice nel buffer
	xpthread_mutex_unlock(a->cmutex,__LINE__,__FILE__); // rilascio il blocco mutex
  xsem_post(a->sem_free_slots,__LINE__,__FILE__); // rilascio il semaforo
    
  fprintf(stdout,"leggo il file %s\n",nomeFile);
  // Leggo il file passato come argomento
  FILE *f = fopen(nomeFile,"rb");
  if(f==NULL) {
		perror("Errore apertura file"); 
    pthread_exit(NULL);
  }
  // sommo tutti i long presenti nel filme
	long numero=0;
  size_t e;
  do {
     a->sommaFile+=numero;
    e=fread(&numero,sizeof(long),1,f);
   
  } while(e==sizeof(long))
  // stampo la somma
  fprintf(stdout,"la somma totale per il file %s e' di %ld",nomeFile,a->sommaFile);
	fclose(f);
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

  char * buffer[lBuffer];  // inizializzo il buffer di dimensione lBuffer
  int indWork = 0; // indice del Worker
  int indMast = 0; // indice del Master
  pthread_mutex_t cmutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_t thr[nthread]; // thread Worker
  dati a[nthread];  // inizializzo contenuto dei thread Worker

  // dichiaro i semafori
  sem_t sem_free_slots, sem_data_items;
  xsem_init(&sem_free_slots,0,lBuffer,__LINE__,__FILE__); // gli slots liberi all'inizio sono tutti (la dimensione del buffer)
  xsem_init(&sem_data_items,0,0,__LINE__,__FILE__); // gli oggetti pronti all'inizio sono 0


  // generazione dei thread
  for(int i = 0; i<nthread;i++) {
    a[i].buffer = buffer;
    a[i].indWork = &indWork;
    a[i].cmutex = &cmutex;
    a[i].BuffSize = &lBuffer;
    a[i].sem_data_items = &sem_data_items;
    a[i].sem_free_slots = &sem_free_slots;
    xpthread_create(&thr[i],NULL,tbody,a+1,__LINE__,__FILE__);
  }


  // lettura dei file
  for(int k = nopz;k<argc;k++) {
      xsem_wait(&sem_free_slots,__LINE__,__FILE__); // blocco il semaforo
      fprintf(stdout,"scrivo il file %s nel buffer alla posizione%d\n",argv[k],indMast % lBuffer);
      buffer[indMast % lBuffer] = argv[k]; // scrivo in modulo lBuffer il nome del file
      fprintf(stdout,"buffer in posizione %d: %s\n",indMast % lBuffer,argv[k]);
      indMast+=1;
      xsem_post(&sem_data_items,__LINE__,__FILE__); // sblocco il semaforo
      usleep(delay);
  }

  // terminazione threads
  for(int i=0;i<nthread;i++) {
    xsem_wait(&sem_free_slots,__LINE__,__FILE__);
    buffer[indMast % lBuffer]= NULL; // scrivo un valore illegale per far capire al consumatore che devo terminare
    indMast+=1;
    xsem_post(&sem_data_items,__LINE__,__FILE__);
  }

// DEVO GESTIRE SIGINT
// DEVO DEALLOCARE E TERMINARE TUTTO PERBENE ALLLA FINE

	return 0;
}