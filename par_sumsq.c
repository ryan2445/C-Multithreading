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
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sumLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t writeCond = PTHREAD_COND_INITIALIZER;

struct TaskQueueNode {
  long data;
  struct TaskQueueNode * next;
};

struct TaskQueue {
  struct TaskQueueNode * front, * back;
};

struct TaskQueueNode * newNode(long data) {
  struct TaskQueueNode * temp = (struct TaskQueueNode *) malloc(sizeof(struct TaskQueueNode));

  temp->data = data;

  temp->next = NULL;

  return temp;
}

struct TaskQueue * createQueue() {
  struct TaskQueue * q = (struct TaskQueue *) malloc(sizeof(struct TaskQueue));

  q->front = q->back = NULL;

  return q;
}

void enQueue(struct TaskQueue * queue, long data) {
  struct TaskQueueNode * temp = newNode(data);

  if (queue->back == NULL) {
    queue->front = queue->back = temp;

    return;
  }

  queue->back->next = temp;

  queue->back = temp;
}

long deQueue(struct TaskQueue * queue) {
  if (queue->front == NULL) return 0;

  struct TaskQueueNode * temp = queue->front;

  queue->front = queue->front->next;

  if (queue->front == NULL) queue->back = NULL;

  const long data = temp->data;

  free(temp);

  return data;
}

void freeQueue(struct TaskQueue * queue) {
  while (queue->front) deQueue(queue);

  free(queue);
}

// function prototypes
void calculate_square(long number);
void * workerFxn(void * args);

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

  pthread_mutex_lock(&sumLock);

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

  pthread_mutex_unlock(&sumLock);
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
  struct TaskQueue * queue = createQueue();
  for (int i = 0; i < numWorkers; i++) {
    pthread_create(&workers[i], NULL, workerFxn, (void *) queue);
  }

  // load numbers and add them to the queue
  char *fn = argv[1];
  FILE* fin = fopen(fn, "r");
  char action;
  long num;

  while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
    if (action == 'p') {            // process, do some work
      pthread_mutex_lock(&queueLock);
      enQueue(queue, num);
      pthread_cond_signal(&writeCond);
      pthread_mutex_unlock(&queueLock);
    } else if (action == 'w') {     // wait, nothing new happening
      sleep(num);
    } else {
      printf("ERROR: Unrecognized action: '%c'\n", action);
      exit(EXIT_FAILURE);
    }
  }
  fclose(fin);
  
  done = true;

  pthread_mutex_lock(&queueLock);
  pthread_cond_broadcast(&writeCond);
  pthread_mutex_unlock(&queueLock);

  void * workerReturn;
  for (int i = 0; i < numWorkers; i++) {
    pthread_join(workers[i], &workerReturn);
  }

  freeQueue(queue);

  // print results
  printf("%ld %ld %ld %ld\n", sum, odd, min, max);
  
  // clean up and return
  return (EXIT_SUCCESS);
}

void * workerFxn(void * args) {
  struct TaskQueue * queue = (struct TaskQueue *) args;
  
  while (!done) {
    pthread_mutex_lock(&queueLock);

    while (!queue->front && !done) {
      pthread_cond_wait(&writeCond, &queueLock);
    }

    if (done) {
      pthread_mutex_unlock(&queueLock);

      break;
    }

    const long number = deQueue(queue);

    pthread_mutex_unlock(&queueLock);

    calculate_square(number);
  }

  return (EXIT_SUCCESS);
}