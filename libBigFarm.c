#include "libBigFarm.h"
#define Buflen 100

// FUNZIONI TERMINAZIONE ERRORE
// funzione che termina un processo con eventuale messaggio d'errore
void termina(const char *messaggio)
{
	if (errno == 0)
		fprintf(stderr, "== %d == %s\n", getpid(), messaggio);
	else
		fprintf(stderr, "== %d == %s: %s\n", getpid(), messaggio,
				strerror(errno));
	exit(EXIT_FAILURE);
}

// funzione termina con aggiunta di linea e file
void xtermina(const char *messaggio, int linea, char *file)
{
	if (errno == 0)
		fprintf(stderr, "== %d == %s\n", getpid(), messaggio);
	else
		fprintf(stderr, "== %d == %s: %s\n", getpid(), messaggio,
				strerror(errno));
	fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);

	exit(1);
}

// funzione che stampa un messaggio di errore su stderr
void xperror(int en, char *msg) {
  char buf[Buflen];
  
  char *errmsg = strerror_r(en, buf, Buflen);
  if(msg!=NULL)
    fprintf(stderr,"%s: %s\n",msg, errmsg);
  else
    fprintf(stderr,"%s\n",errmsg);
}

// FUNZIONI SU FILE
// funzione per l'apertura di file con linea e file
FILE *xfopen(const char *path, const char *mode, int linea, char *file)
{
	FILE *f = fopen(path, mode);
	if (f == NULL)
	{
		perror("Errore apertura file");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		exit(1);
	}
	return f;
}

// funzione per la chiusura di file descriptor
void xclose(int fd, int linea, char *file)
{
	int e = close(fd);
	if (e != 0)
	{
		perror("Errore chiusura file descriptor");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		exit(1);
	}
	return;
}

// funzione che legge esattamente n bytes da un file descriptor
ssize_t readn(int fd, void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nread;

	nleft = n;
	while (nleft > 0)
	{
		if ((nread = read(fd, ptr, nleft)) < 0)
		{
			if (nleft == n)
				return -1; /* error, return -1 */
			else
				break; /* error, return amount read so far */
		}
		else if (nread == 0)
			break; /* EOF */
		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft); /* return >= 0 */
}

// funzione che scrive esattamente n bytes da un file descriptor
ssize_t writen(int fd, void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;

	nleft = n;
	while (nleft > 0)
	{
		if ((nwritten = write(fd, ptr, nleft)) < 0)
		{
			if (nleft == n)
				return -1; /* error, return -1 */
			else
				break; /* error, return amount written so far */
		}
		else if (nwritten == 0)
			break;
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n - nleft); /* return >= 0 */
}


// FUNZIONI PER LA GESTIONE DI SEMAFORI POSIX
// funzione per l'apertura di un semaforo posix
sem_t *xsem_open(const char *name, int oflag, mode_t mode, unsigned int value, int linea, char *file)
{
	sem_t *s = sem_open(name, oflag, mode, value);
	if (s == SEM_FAILED)
	{
		perror("Errore sem_open");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		exit(1);
	}
	return s;
}

// funzione per la chiusura di un semaforo posix
int xsem_close(sem_t *s, int linea, char *file)
{
	int e = sem_close(s);
	if (e != 0)
	{
		perror("Errore sem_close");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		exit(1);
	}
	return e;
}

// funzione che inizializza un semaforo anonimo
int xsem_init(sem_t *sem, int pshared, unsigned int value, int linea, char *file)
{
	int e = sem_init(sem, pshared, value);
	if (e != 0)
	{
		perror("Errore sem_init");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		pthread_exit(NULL);
	}
	return e;
}

// funzione che incrementa il puntatore del semaforo
int xsem_post(sem_t *sem, int linea, char *file)
{
	int e = sem_post(sem);
	if (e != 0)
	{
		perror("Errore sem_post");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		pthread_exit(NULL);
	}
	return e;
}

// funzione che decrementa il puntatore del semaforo
int xsem_wait(sem_t *sem, int linea, char *file)
{
	int e = sem_wait(sem);
	if (e != 0)
	{
		perror("Errore sem_wait");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		pthread_exit(NULL);
	}
	return e;
}

// FUNZIONI PER LA GESTIONE DI THREAD
// funzione che fa partire un nuovo thread
int xpthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg, int linea, char *file)
{
	int e = pthread_create(thread, attr, start_routine, arg);
	if (e != 0)
	{
		xperror(e, "Errore pthread_create");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		pthread_exit(NULL);
	}
	return e;
}

// funzione che attende che il thread abbia terminato
int xpthread_join(pthread_t thread, void **retval, int linea, char *file)
{
	int e = pthread_join(thread, retval);
	if (e != 0)
	{
		xperror(e, "Errore pthread_join");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		pthread_exit(NULL);
	}
	return e;
}

// FUNZIONI PER LA GESTIONE DI MUTEX
// funzione che inizializza una mutex
int xpthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr, int linea, char *file)
{
	int e = pthread_mutex_init(mutex, attr);
	if (e != 0)
	{
		xperror(e, "Errore pthread_mutex_init");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		pthread_exit(NULL);
	}
	return e;
}

// funzione che chiude una mutex
int xpthread_mutex_destroy(pthread_mutex_t *mutex, int linea, char *file)
{
	int e = pthread_mutex_destroy(mutex);
	if (e != 0)
	{
		xperror(e, "Errore pthread_mutex_destroy");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		pthread_exit(NULL);
	}
	return e;
}

// funzione che blocca una mutex
int xpthread_mutex_lock(pthread_mutex_t *mutex, int linea, char *file)
{
	int e = pthread_mutex_lock(mutex);
	if (e != 0)
	{
		xperror(e, "Errore pthread_mutex_lock");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		pthread_exit(NULL);
	}
	return e;
}

// funzione che sblocca una mutex
int xpthread_mutex_unlock(pthread_mutex_t *mutex, int linea, char *file)
{
	int e = pthread_mutex_unlock(mutex);
	if (e != 0)
	{
		xperror(e, "Errore pthread_mutex_unlock");
		fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
		pthread_exit(NULL);
	}
	return e;
}