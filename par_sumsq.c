/*
 * sumsq.c
 *
 * CS 446.646 Project 1 (Pthreads)
 *
 * Compile with --std=c99
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// aggregate variables
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
bool done = false;
struct TaskQueue {
  int data;
  struct TaskQueue * next;
};

// function prototypes
void calculate_square(long number);
void * workerFxn(void * vargp);

/*
 * update global aggregate variables given a number
 */
void calculate_square(long number)
{

  // calculate the square
  long the_square = number * number;

  // ok that was not so hard, but let's pretend it was
  // simulate how hard it is to square this number!
  sleep(number);

  // let's add this to our (global) sum
  sum += the_square;

  // now we also tabulate some (meaningless) statistics
  if (number % 2 == 1) {
    // how many of our numbers were odd?
    odd++;
  }

  // what was the smallest one we had to deal with?
  if (number < min) {
    min = number;
  }

  // and what was the biggest one?
  if (number > max) {
    max = number;
  }
}


int main(int argc, char* argv[])
{
  // check and parse command line options
  if (argc != 3) {
    printf("Usage: par_sumsq <infile>\n");
    exit(EXIT_FAILURE);
  }

  const char * numWorkersArg = argv[2];
  const int numWorkers = atoi(numWorkersArg);
  pthread_t workers[numWorkers];
  struct TaskQueue * queue = malloc(sizeof(struct TaskQueue));
  queue->next = NULL;
  for (int i = 0; i < numWorkers; i++) {
    pthread_create(&workers[i], NULL, workerFxn, (void *) queue);
  }

  for (int i = 0; i < numWorkers; i++) {
    pthread_join(workers[i], NULL);
  }

  // char *fn = argv[1];
  // // load numbers and add them to the queue
  // FILE* fin = fopen(fn, "r");
  // char action;
  // long num;

  // while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
  //   if (action == 'p') {            // process, do some work
  //     calculate_square(num);
  //   } else if (action == 'w') {     // wait, nothing new happening
  //     sleep(num);
  //   } else {
  //     printf("ERROR: Unrecognized action: '%c'\n", action);
  //     exit(EXIT_FAILURE);
  //   }
  // }
  // fclose(fin);
  
  // // print results
  // printf("%ld %ld %ld %ld\n", sum, odd, min, max);
  
  // clean up and return
  return (EXIT_SUCCESS);
}

void * workerFxn(void * vargp) {
  printf("Running worker\n");
  return NULL;
}

