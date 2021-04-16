#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

//  Aggregate variables
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
bool done = false;
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sumLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t writeCond = PTHREAD_COND_INITIALIZER;

//  TaskQueueNode - contains number to square and pointer to next node in queue
struct TaskQueueNode {
  long data;
  struct TaskQueueNode * next;
};

//  TaskQueue - Contains pointers to TaskQueueNodes at front and back of queue
struct TaskQueue {
  struct TaskQueueNode * front, * back;
};

//  newNode - returns new TaskQueueNode with data set
struct TaskQueueNode * newNode(long data) {
  struct TaskQueueNode * temp = (struct TaskQueueNode *) malloc(sizeof(struct TaskQueueNode));

  temp->data = data;

  temp->next = NULL;

  return temp;
}

//  createQueue - Returns a new TaskQueue
struct TaskQueue * createQueue() {
  struct TaskQueue * q = (struct TaskQueue *) malloc(sizeof(struct TaskQueue));

  q->front = q->back = NULL;

  return q;
}

//  enQueue - Creates a new TaskQueueNode
void enQueue(struct TaskQueue * queue, long data) {
  struct TaskQueueNode * temp = newNode(data);

  //  If the queue is empty set front and back to the new node
  if (queue->back == NULL) {
    queue->front = queue->back = temp;

    return;
  }

  //  Set next pointer at back of queue
  queue->back->next = temp;

  //  Change the back of the queue
  queue->back = temp;
}

long deQueue(struct TaskQueue * queue) {
  //  If queue is empty, return 0 so we don't change the sum
  if (queue->front == NULL) return 0;

  //  Get the node at the front of the queue
  struct TaskQueueNode * temp = queue->front;

  //  Set the front of the queue to the second node in the queue
  queue->front = queue->front->next;

  //  If there was only 1 node in the queue, set the back to null
  if (queue->front == NULL) queue->back = NULL;

  //  Get the number to be squared from the front node
  const long data = temp->data;

  //  Delete node from memory
  free(temp);

  //  Return the number to be squared
  return data;
}

void freeQueue(struct TaskQueue * queue) {
  //  While the queue is not empty, delete nodes
  while (queue->front) deQueue(queue);

  //  Then delete teh queue
  free(queue);
}

// function prototypes
void calculate_square(long number);
void * workerFxn(void * args);

/*
 *  Update global aggregate variables given a number
 */
void calculate_square(long number)
{
  // calculate the square
  long the_square = number * number;

  // ok that was not so hard, but let's pretend it was
  // simulate how hard it is to square this number!
  sleep(number);

  //  Lock the modification of the sum, odd, min, max variables
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

  //  Release lock for aggregate varables
  pthread_mutex_unlock(&sumLock);
}


int main(int argc, char* argv[])
{
  // check and parse command line options
  if (argc != 3) {
    printf("Usage: par_sumsq <infile>\n");
    exit(EXIT_FAILURE);
  }

  //  Get the number of workers specified in arguments
  const char * numWorkersArg = argv[2];

  //  Parse the char version of the number to an int
  const int numWorkers = atoi(numWorkersArg);

  //  Init our threads
  pthread_t workers[numWorkers];

  //  Create our TaskQueue
  struct TaskQueue * queue = createQueue();

  //  Bind our threads to the worker function
  for (int i = 0; i < numWorkers; i++) {
    pthread_create(&workers[i], NULL, workerFxn, (void *) queue);
  }

  //  Open the file
  char *fn = argv[1];
  FILE* fin = fopen(fn, "r");
  char action;
  long num;

  //  Read from text file
  while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
    //  Process, do some work
    if (action == 'p') {
      //  Lock the queue
      pthread_mutex_lock(&queueLock);

      //  Push the number on the queue
      enQueue(queue, num);

      //  Signal a process that is asleep to start de-queueing
      pthread_cond_signal(&writeCond);

      //  Unlock the queue
      pthread_mutex_unlock(&queueLock);
    } 
    //  Wait, nothing new happening
    else if (action == 'w') {
      sleep(num);
    }
    //  Something bad happened ¯\_(ツ)_/¯
    else {
      printf("ERROR: Unrecognized action: '%c'\n", action);
      exit(EXIT_FAILURE);
    }
  }

  //  Close the file
  fclose(fin);
  
  //  Set done to true to stop threads from trying to de-queue numbers
  done = true;

  //  Lock the queue
  pthread_mutex_lock(&queueLock);

  //  Broadcast to all threads who are stuck inside while(!done) loop in workerFxn
  pthread_cond_broadcast(&writeCond);

  //  Unlock the queue
  pthread_mutex_unlock(&queueLock);

  //  Terminate our threads
  void * workerReturn;
  for (int i = 0; i < numWorkers; i++) {
    pthread_join(workers[i], &workerReturn);
  }

  //  Delete the queue from memory
  freeQueue(queue);

  //  Print results
  printf("%ld %ld %ld %ld\n", sum, odd, min, max);
  
  //  Clean up and return
  return (EXIT_SUCCESS);
}

void * workerFxn(void * args) {
  //  Get our task queue
  struct TaskQueue * queue = (struct TaskQueue *) args;
  
  //  While we are still queueing numbers
  while (!done) {
    //  Lock the queue
    pthread_mutex_lock(&queueLock);

    //  If there are no numbers on the queue but we aren't done queueing, wait for broadcast from master
    while (!queue->front && !done) {
      pthread_cond_wait(&writeCond, &queueLock);
    }

    //  If master broadcasts writeCond but all numbers are already processed, exit loop
    if (done) {
      //  Unlock the queue
      pthread_mutex_unlock(&queueLock);

      //  Break freeeeeee
      break;
    }

    //  Pull a number from the queue
    const long number = deQueue(queue);

    //  Unlock the queue
    pthread_mutex_unlock(&queueLock);

    //  Calculate the square of the number!
    calculate_square(number);
  }

  //  Exit worker function
  return (EXIT_SUCCESS);
}