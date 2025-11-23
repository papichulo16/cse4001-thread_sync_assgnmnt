//
// Example from: http://www.amparo.net/ce155/sem-ex.c
//
// Adapted using some code from Downey's book on semaphores
//
// Compilation:
//
//       g++ main.cpp -lpthread -o main -lm
// or 
//      make
//

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */
#include <iostream>
#include <cstdint>
using namespace std;

#define LEFT(a) (a % 5)
#define RIGHT(a) ((a + 1) % 5)

typedef void* (*func)(void*);

/*
 This wrapper class for semaphore.h functions is from:
 http://stackoverflow.com/questions/2899604/using-sem-t-in-a-qt-project
 */
class Semaphore {
public:
    // Constructor
    Semaphore(int initialValue)
    {
        sem_init(&mSemaphore, 0, initialValue);
    }
    // Destructor
    ~Semaphore()
    {
        sem_destroy(&mSemaphore); /* destroy semaphore */
    }
    
    // wait
    void wait()
    {
        sem_wait(&mSemaphore);
    }
    // signal
    void signal()
    {
        sem_post(&mSemaphore);
    }
    
    
private:
    sem_t mSemaphore;
};

class LightSwitch {
public:
  LightSwitch() {
    counter = 0;
  }

  void lock(Semaphore sem) {
    mutex.wait();
    counter++;

    if (counter == 1) {
      sem.wait();
    }

    mutex.signal();
  }

  void unlock(Semaphore sem) {
    mutex.wait();
    counter--;

    if (counter == 0) {
      sem.signal();
    }

    mutex.signal();
  }

private:
  int counter;
  Semaphore mutex = Semaphore(1);
};

/* global vars */
const int bufferSize = 5;
const int numConsumers = 5; 
const int numProducers = 5; 

/* semaphores are declared global so they can be accessed
 in main() and in thread routine. */
Semaphore Mutex(1);
Semaphore Spaces(bufferSize);
Semaphore Items(0);             

/*
    Producer function 
*/
void* Producer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;

    while( 1 )
    {
        sleep(3); // Slow the thread down a bit so we can see what is going on
        Spaces.wait();
        Mutex.wait();
        printf("Producer %d adding item to buffer \n", x);
        fflush(stdout);
        Mutex.signal();
        Items.signal();
    }
}

/*
    Consumer function 
*/
void *Consumer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;
    
    while( 1 )
    {
        Items.wait();
        Mutex.wait();

        printf("Consumer %d removing item from buffer \n", x);
        fflush(stdout);

        Mutex.signal();
        Spaces.signal();

        sleep(5);   // Slow the thread down a bit so we can see what is going on
    }
}

// no starve readers-writers problem

int var = 0;
Semaphore ns_turnstyle(1);
Semaphore ns_room_empty(1);
LightSwitch ns_read_switch = LightSwitch();

void* nostarve_reader(void* t) {
    int x = (long) t;

    while( 1 )
    {
        sleep(3); // Slow the thread down a bit so we can see what is going on
        ns_turnstyle.wait();
        ns_turnstyle.signal();

        ns_read_switch.lock(ns_room_empty);
        printf("reader %d reading var at value %d\n", x, var);
        fflush(stdout);
        ns_read_switch.unlock(ns_room_empty);
    }
}

void* nostarve_writer(void* t) {
    int x = (long) t;

    while( 1 )
    {
        ns_turnstyle.wait();
        ns_room_empty.wait();

        var++;

        printf("writer %d writing var at value %d \n", x, var);
        fflush(stdout);

        ns_turnstyle.signal();
        ns_room_empty.signal();

        sleep(5);   // Slow the thread down a bit so we can see what is going on
    }
}

// writer priority readers-writers problem

Semaphore wp_no_readers(1);
Semaphore wp_no_writers(1);
LightSwitch wp_read_switch = LightSwitch();
LightSwitch wp_write_switch = LightSwitch();

void* wprio_reader(void* t) {
    int x = (long) t;

    while( 1 )
    {
        sleep(3); // Slow the thread down a bit so we can see what is going on
        wp_no_readers.wait();
        wp_read_switch.lock(wp_no_writers);
        wp_no_readers.signal();

        printf("reader %d reading var at value %d\n", x, var);
        fflush(stdout);

        wp_read_switch.unlock(wp_no_writers);
    }
}

void* wprio_writer(void* t) {
    int x = (long) t;

    while( 1 )
    {
        wp_write_switch.lock(wp_no_readers);
        wp_no_writers.wait();

        var++;

        printf("writer %d writing var at value %d \n", x, var);
        fflush(stdout);

        wp_no_writers.signal();
        wp_write_switch.unlock(wp_no_readers);

        sleep(5);   // Slow the thread down a bit so we can see what is going on
    }
}

// philosophers solution one

Semaphore forks[5] = { Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1) };
Semaphore footman = Semaphore(4);
Semaphore id = Semaphore(1);
int pnum = 0;

void get_forks(int i) {
  forks[RIGHT(i)].wait();
  forks[LEFT(i)].wait();
}

void put_forks(int i) {
  forks[RIGHT(i)].signal();
  forks[LEFT(i)].signal();
}

void* pone_philosopher(void* t) {
    int x = (long) t;

    id.wait();
    int p = pnum++;
    id.signal();

    while( 1 )
    {
        sleep(3); // Slow the thread down a bit so we can see what is going on
                  
        printf("philosopher %d thinking...\n", p);
        fflush(stdout);

        footman.wait();
        get_forks(p);

        printf("philosopher %d eating...\n", p);
        fflush(stdout);

        put_forks(p);
        footman.signal();
    }
}

void _get_forks(int i) {
  if (i == 0) {
    forks[RIGHT(i)].wait();
    forks[LEFT(i)].wait();
  } else {
    forks[LEFT(i)].wait();
    forks[RIGHT(i)].wait();
  }
}

void _put_forks(int i) {
  if (i == 0) {
    forks[RIGHT(i)].signal();
    forks[LEFT(i)].signal();
  } else {
    forks[LEFT(i)].signal();
    forks[RIGHT(i)].signal();
  }
}

void* ptwo_philosopher(void* t) {
    int x = (long) t;

    id.wait();
    int p = pnum++;
    id.signal();

    while( 1 )
    {
        sleep(3); // Slow the thread down a bit so we can see what is going on
                  
        printf("philosopher %d thinking...\n", p);
        fflush(stdout);

        _get_forks(p);

        printf("philosopher %d eating...\n", p);
        fflush(stdout);

        _put_forks(p);
    }
}

void* nothing(void* t) {
    int x = (long) t;

    while( 1 );
}

func producer_funcs[] = { &nostarve_reader, &wprio_reader, &pone_philosopher, &ptwo_philosopher };
func consumer_funcs[] = { &nostarve_writer, &wprio_writer, &nothing, &nothing };

int main(int argc, char **argv )
{
    pthread_t producerThread[ numProducers ];
    pthread_t consumerThread[ numConsumers ];

    if (argc < 2) {
      printf("usage: ./cse4001_sync  <q_num>\n");
      exit(-1);
    }

    uint32_t i = atoi(argv[1]) - 1;

    if (i >= sizeof(consumer_funcs) / 8) {
      printf("agnmnt num too large\n");
      exit(-1);
    }

    // Create the producers 
    for( long p = 0; p < numProducers; p++ )
    {
        int rc = pthread_create ( &producerThread[ p ], NULL, 
                                  producer_funcs[i], (void *) (p+1) );
        if (rc) {
            printf("ERROR creating producer thread # %d; \
                    return code from pthread_create() is %d\n", p, rc);
            exit(-1);
        }
    }

    // Create the consumers 
    for( long c = 0; c < numConsumers; c++ )
    {
        int rc = pthread_create ( &consumerThread[ c ], NULL, 
                                  consumer_funcs[i], (void *) (c+1) );
        if (rc) {
            printf("ERROR creating consumer thread # %d; \
                    return code from pthread_create() is %d\n", c, rc);
            exit(-1);
        }
    }

    // To allow other threads to continue execution, the main thread 
    // should terminate by calling pthread_exit() rather than exit(3). 
    pthread_exit(NULL); 
} /* main() */


