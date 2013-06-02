/*
 ============================================================================
 Name        : Trunks_versionSemaphores.c
 Author      : manuel
 Version     :
 Copyright   : copyright notice
 Description : Message in a bottle in C, Ansi-style
 ============================================================================
*/

/*
 *
 *	Short	: A solution to the multi-TRUNK's, one ETHER problem.
 *	The producer does the following repeatedly:

EHTER produce:
    WAIT (emptyCount)
    WAIT (useQueue)
    putItemIntoQueue(item)
    SiGNAL (useQueue)
    SiGNAL (fullCount)

The consumer does the following repeatedly:

TRUNK consume:
    WAIT (fullCount)
    WAIT (useQueue)
    item ← getItemFromQueue()
    SIGNAL (useQueue)
    SIGNAL (emptyCount)
 *
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
/**
 * constants
 *
 * *********************************************************/
#define MAXCOUNT 5

#define NUMTRUNKS	4
#define TRUNK1   50000
#define TRUNK2   100000
#define TRUNK3   400000
#define TRUNK4   800000

#define ETHER1  150000

/**
 * @brief: if CAPACITYETHER is superior to a multiple of CAPACITYTRUNK (CAPACITYETHER%CAPACITYTRUNK>0),
 * subsequent boxes sent by ETHER are lost because TRUNKS won't have the capacity to process it
 * e.g. CAPACITYETHER=  (CAPACITYTRUNK*4)+1
 *
 * On the other hand if CAPACITYETHER if less than the sum of CAPACITYTRUNK
 * there will always be 1 or more TRUNKS waiting forever until self capacity
 * e.g. CAPACITYETHER=  (CAPACITYTRUNK*4)-1 will let 1 TRUNK waiting
 *
 * BEST scenario is CAPACITYETHER multiple of CAPACITYTRUNK (CAPACITYETHER%CAPACITYTRUNK=0)
 * This means that each and every box spawned by ETHER will be processed by 1 TRUNK
 *
 * ALSO , no matter if QUEUESIZE > CAPACITYETHER or QUEUESIZE < CAPACITYETHER.
 */
#define QUEUESIZE 10
#define CAPACITYTRUNK 2
#define CAPACITYETHER (CAPACITYTRUNK*4)
/**
 * Structures
 *
 * *********************************************************/
typedef struct {
        int buf[QUEUESIZE];
        long head, tail;
        int full, empty;
        pthread_mutex_t *mut;	//a mutex (binary semaphore) to guard access to queue (i.e. buf)
    	sem_t  boxesAvailable;	//number of free elements available in the queue (free space) initially a semaphore for QUEUESIZE
    	sem_t  boxesFull;	//the number of elements in the queue - initially a semaphore of size 0
    	int ETHERs;
    	int TRUNKs;
    	int waiting;
} queue;


typedef struct _ETHER{
	queue *q;
	int id;
	int waiting;
	unsigned int delay;
	unsigned int Capacity;//bytes
} ETHER;

typedef struct _TRUNK{
	queue* q;
	int id;
	int waiting;
	unsigned int delay;//ms
	unsigned int Capacity;//bytes
} TRUNK;

typedef struct _argumentos{
	ETHER *lock;
	queue* fifo;
	int id;
	long delay;
	unsigned int Capacity;
} argumentos;

typedef struct _argumentosTRUNK{
	TRUNK *lock;
	queue* fifo;
	int id;
	long delay;
	unsigned int Capacity;
} argumentosTRUNK;


/**
 * Definitions
 *
 * *********************************************************/
queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, int in);
void queueDel (queue *q, int *out);

ETHER *initlock (void);
TRUNK *initTRUNK(void);



argumentos *newRWargs (ETHER *l, int i, long d, unsigned int capac, queue* fifo);
argumentosTRUNK *newRWargsTr (TRUNK *l, int i, long d, unsigned int capac, queue* fifo);

void *runTRUNK (void *args);
void *runETHER (void *args);



/**
 * Main
 *
 * *********************************************************/

int main (void )
{
	pthread_t r1, r2, r3, r4, w1;
	argumentos *a1=NULL;
	argumentosTRUNK *a2, *a3, *a4, *a5;


	ETHER *lock=NULL;
	TRUNK *lockTr1,*lockTr2,*lockTr3,*lockTr4 ;
	queue *fifo=NULL;



	lock = initlock ();
	if (lock==NULL)
		return EXIT_FAILURE;

	fifo=lock->q;
	if (fifo==NULL)
		return EXIT_FAILURE;

	a1 = newRWargs (lock, 1, ETHER1,CAPACITYETHER, fifo);
	if (a1==NULL)
		return EXIT_FAILURE;


	pthread_create (&w1, NULL, runETHER, a1);

	lockTr1=initTRUNK();
	lockTr2=initTRUNK();
	lockTr3=initTRUNK();
	lockTr4=initTRUNK();


	a2 = newRWargsTr (lockTr1, 1, TRUNK1, CAPACITYTRUNK, fifo);
	pthread_create (&r1, NULL, runTRUNK, a2);
	a3 = newRWargsTr (lockTr2, 2, TRUNK2, CAPACITYTRUNK, fifo);
	pthread_create (&r2, NULL, runTRUNK, a3);
	a4 = newRWargsTr (lockTr3, 3, TRUNK3, CAPACITYTRUNK, fifo);
	pthread_create (&r3, NULL, runTRUNK, a4);
	a5 = newRWargsTr (lockTr4, 4, TRUNK4, CAPACITYTRUNK, fifo);
	pthread_create (&r4, NULL, runTRUNK, a5);

	pthread_join (w1, NULL);
	pthread_join (r1, NULL);
	pthread_join (r2, NULL);
	pthread_join (r3, NULL);
	pthread_join (r4, NULL);

	free (a1); free (a2); free (a3); free (a4); free (a5);
    	queueDelete (lock->q);
    	free(lock);free(lockTr1),free(lockTr2),free(lockTr3),free(lockTr4);

	return EXIT_SUCCESS;
}

argumentos *newRWargs (ETHER *l, int i, long d, unsigned int capac, queue* fifo)
{
	argumentos *args;

	args = malloc (sizeof (argumentos));
	if (args == NULL)
		return (NULL);

	args->lock = l; args->id = i; args->delay = d; args->Capacity = capac; args->fifo=fifo;
	return (args);
}

argumentosTRUNK *newRWargsTr (TRUNK *l, int i, long d, unsigned int capac, queue* fifo)
{
	argumentosTRUNK *args;

	args = (argumentosTRUNK *)malloc (sizeof (argumentosTRUNK));
	if (args == NULL)
		return (NULL);

	args->lock = l; args->id = i; args->delay = d; args->Capacity = capac, args->fifo=fifo;

	return (args);
}

void *runTRUNK (void *args)
{
	argumentosTRUNK *a=(argumentosTRUNK *)args;
	queue *fifo;
	int i;
	int d;


	a->lock->id=a->id;
	a->lock->Capacity=a->Capacity;
	a->lock->delay=a->delay;
	a->lock->q=a->fifo;


	fifo=a->fifo;

	printf ("<--runTRUNK%d\n",a->id);

    	//processes capacity
    	for (i = 0; i < a->lock->Capacity; i++) {


	    //each time a thread waits on this semaphore, the count decreases until boxesAvailable=0 (as defined)
            //when boxesAvailable=0 ,the thread blocks, otherwise the thread can go further
            sem_wait(&fifo->boxesAvailable);


	    pthread_mutex_lock (fifo->mut);
            queueDel (fifo, &d); //threadSafe op
            pthread_mutex_unlock (fifo->mut);

            //each time a thread POSTS on this semaphore, the count increases by 1 until boxesFull=QUEUESIZE
            sem_post(&fifo->boxesFull);

            printf ("trunk%d: received %d.\n",a->id, d);
            usleep(a->lock->delay);

        }


	printf ("TRUNK %d: Finished.\n", a->id);

	return (NULL);
}

void *runETHER (void *args)
{
	argumentos *a;
	queue *fifo;
	int i;



	a = (argumentos *)args;
	a->lock->id=a->id;
	a->lock->Capacity=a->Capacity;
	a->lock->delay=a->delay;
	a->lock->q=a->fifo;
	fifo = (queue*) (a->fifo);

	printf ("--> runETHER %d\n",a->id);

		for (i = 0; i < a->lock->Capacity; i++) {

			//generates capacity


			//each time a thread waits on this semaphore, the count decreases until boxesFull=0
			//it then blocks
			//boxesFull is defined  = QUEUESIZE, so that the thread will not block until it waits QUEUESIZE times
			//This make it possible to not having to maintain the logic for q->Full and q->Empty in the circular FIFO used
			sem_wait(&fifo->boxesFull);

			pthread_mutex_lock(fifo->mut);
			queueAdd(fifo, i + 1);//threadSafe op
			pthread_mutex_unlock(fifo->mut);

			printf("\tether%d: sends %d.\n", a->id, i + 1);

			sem_post(&fifo->boxesAvailable);

			usleep(a->delay);

		}

	//ETHER has consumed the entire input, Notify
	printf ("ETHER %d: Finishing...\n", a->id);
	printf ("ETHER %d: Finished.\n", a->id);


	return (NULL);
}

ETHER *initlock (void)
{
	ETHER *lock;

    	lock = (ETHER *)malloc (sizeof (ETHER));
	if (lock == NULL) return (NULL);

    	lock->q = queueInit ();
    	if (lock->q ==  NULL) {
        	fprintf (stderr, "initlock: Main Queue Init failed.\n");
		return (NULL);
    	}

	lock->id=0;
	lock->waiting = 0;
	lock->Capacity = CAPACITYETHER;
	lock->delay = ETHER1;

	return (lock);
}


TRUNK *initTRUNK (void)
{
	TRUNK *lock;

	lock = (TRUNK *)malloc (sizeof (TRUNK));
	if (lock == NULL)
		return (NULL);

	lock->id=0;
	lock->waiting = 0;
	lock->Capacity = CAPACITYTRUNK;
	lock->delay = TRUNK1;

	return (lock);
}


queue *queueInit (void)
{
        queue *q;

        q = (queue *)malloc (sizeof (queue));
        if (q == NULL) return (NULL);

        q->empty = 1;
        q->full = 0;
        q->head = 0;
        q->tail = 0;
        q->TRUNKs = 0;
        q->ETHERs = 0;


        q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
        pthread_mutex_init (q->mut, NULL);


        sem_init(&q->boxesAvailable, 0, 0);
        sem_init(&q->boxesFull, 0, sizeof(q->buf));

        return (q);
}

void queueDelete (queue *q)
{
        pthread_mutex_destroy (q->mut);
        free (q->mut);
        sem_destroy(&q->boxesAvailable);
        sem_destroy(&q->boxesFull);
        free (q);
}

void queueAdd (queue *q, int in)
{
        q->buf[q->tail] = in;
        q->tail++;
        if (q->tail == QUEUESIZE)
                q->tail = 0;
        if (q->tail == q->head)
                q->full = 1;
        q->empty = 0;

        return;
}

void queueDel (queue *q, int *out)
{
        *out = q->buf[q->head];

        q->head++;
        if (q->head == QUEUESIZE)
                q->head = 0;
        if (q->head == q->tail)
                q->empty = 1;
        q->full = 0;

        return;
}

