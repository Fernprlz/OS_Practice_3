#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "../include/concurrency_layer.h"

// Mutex variables for mutual exclusion concurrency control.
pthread_mutex_t generalQ_mutex;
pthread_mutex_t brokerQ_mutex;
pthread_mutex_t read_mutex;
pthread_mutex_t stateQ_mutex;
pthread_mutex_t conc_readers_mutex;

// Mutex condition variable used to work with the state of the Queue.
pthread_cond_t freeQ_cond;

// Variable tracking the number of concurrent readers.
int conc_readers = 0;

/**
* Initializes the concurrency control mechanisms.
* Always called by the main method.
*/
void init_concurrency_mechanisms(){

  // Initialize the mutex variabels with default attributes.
  pthread_mutex_init(&generalQ_mutex, NULL);
  pthread_mutex_init(&brokerQ_mutex, NULL);
  pthread_mutex_init(&read_mutex, NULL);
  pthread_mutex_init(&stateQ_mutex, NULL);
  pthread_mutex_init(&conc_readers_mutex, NULL);

  // Initialize the mutex conditions.
  pthread_cond_init(&freeQ_cond, NULL);
}

/**
* Destroys the initialized concurrency control mechanisms.
* Always called by the main method upon exit.
*/
void destroy_concurrency_mechanisms(){

  // Destroy the mutex variales.
  pthread_mutex_destroy(&generalQ_mutex);
  pthread_mutex_destroy(&brokerQ_mutex);
  pthread_mutex_destroy(&read_mutex);
  pthread_mutex_destroy(&stateQ_mutex);
  pthread_mutex_destroy(&conc_readers_mutex);

  // Destroy the mute conditions.
  pthread_cond_destroy(&freeQ_cond);
}

/**
* Implements the functionality of broker threads + CC
* - char batch_file[256]: name of the batch file containing the transactions.
* - stock_market* market: pointer to the market on which the broker operates.
*/
void* broker(void* args){

  // Create a broker_info structure variable to dereference the arguments.
  struct broker_info * data = args;

  // Extract info from the argument.
  char file[256];
  memcpy(file, data->batch_file, 256);
  operations_queue* market_queue = data->market->stock_operations;

  // Create a new iterator to traverse the operations in the batch file.
  iterator *iter;
  if ((iter = new_iterator(file)) == NULL) {
    printf("ERROR: iterator could not be created. \n");
  }

  // Variables that will be filled with operation details
  char id[ID_LENGTH];
  int type;
  int num_shares;
  int price;

  // For every operation, perform the insertion.
  while (next_operation(iter, id, &type, &num_shares, &price) >= 0){

    // Create a pointer to hold the operation to be inserted.
    operation *op;
    // Allocate memory to hold the operation avoiding segmentation faults.
    op = (operation*) malloc(sizeof(operation));
    // The thread is stopped if there's no space in the queue.
    while(operations_queue_full(market_queue) > 0){
      // Freeze the thread until the signal is sent from the operation_executer;
      pthread_cond_wait(&freeQ_cond, &stateQ_mutex);
    }

    /* --- C R I T I C A L  -  S E C T I O N --- */
    // Lock the access for readers.
    pthread_mutex_lock(&read_mutex);
    // Lock the access to the queue.
    pthread_mutex_lock(&generalQ_mutex);
    // Lock the execution of the broker threads.
    pthread_mutex_lock(&brokerQ_mutex);

    // Pack the data of the operation for insertion.
    new_operation(op, id, type, num_shares, price);
    // Insert the operation into the market queue.
    enqueue_operation(market_queue, op);

    // Unlock the access for readers.
    pthread_mutex_unlock(&read_mutex);
    // Unlock the access to the queue.
    pthread_mutex_unlock(&generalQ_mutex);
    // Unlock the execution of the broker threads.
    pthread_mutex_unlock(&brokerQ_mutex);
    /* ------------------------------------------ */

  }
  // Destroy the iterator
  destroy_iterator(iter);
  printf("Broker thread ended \n");
}

/**
* Processes the operations introduced in the queue by the brokers.
* - int *exit: pointer to flag termination of the application. If true the
*   thread will process existing requests pending in the queue and terminate.
* - stock_market *market: pointer to the market on which the processor operates.
* - pthread_mutex_t *exit_mutex: pointer to mutex that protects the variable exit.
*   it must access the variable exit always with a locked mutex.
*/
void* operation_executer(void* args){

  // Create a exec_info structure variable to dereference the arguments.
  struct exec_info * data = args;

  // Extract info from the argument. It contains the mutex for the exit variable.
  int *exit = data->exit;
  pthread_mutex_t *exit_mutex = data->exit_mutex;
  stock_market *market = data->market;
  operations_queue *market_queue = data->market->stock_operations;

  // Lock exit mutex to check for the value of exit flag
  pthread_mutex_lock(exit_mutex);
  // Execution loop - Execute until exit flag is raised
  while(*exit == 0 || operations_queue_empty(market_queue) == 0){
    //Unlock exit mutex after checking exit flag
    pthread_mutex_unlock(exit_mutex);
    // The thread is stopped if there are no operations in the queue.
    while(operations_queue_empty(market_queue) > 0 && *exit==0 );
    // Create a pointer to hold the operation to be processed
    operation *op;
    // Allocate memory to hold the operation avoiding segmentation faults.
    op = (operation*) malloc(sizeof(operation));

    /* --- C R I T I C A L  -  S E C T I O N --- */

    // Lock the access for readers.
    pthread_mutex_lock(&read_mutex);
    // Lock the access to the queue.
    pthread_mutex_lock(&generalQ_mutex);

    // Dequeue the operation from the stock. Conditionals check for error cases.
    if (operations_queue_empty(market_queue) == 0){
      if (dequeue_operation(market_queue, op) == 0){
        // Carry out the operation.
        process_operation(market, op);
      }
    }

    // Unlock the access for readers.
    pthread_mutex_unlock(&read_mutex);
    // Unlock the access to the queue.
    pthread_mutex_unlock(&generalQ_mutex);
    /* ------------------------------------------ */

    // Send a signal to any thread waiting for space in the queue.
    pthread_cond_signal(&freeQ_cond);

    //Lock exit mutex before re-checking exit flag value
    pthread_mutex_lock(exit_mutex);
  }
  //Unlock exit mutex and end thread execution
  pthread_mutex_unlock(exit_mutex);
  printf("Executer thread ended \n");
}

/**
* Evaluates the stock exchange market
* - int *exit: pointer to flag termination of the application. When true.
* - stock_market *market: pointer to the market on which the consultant operates.
* - pthread_mutex_t *exit_mutex: pointer to the mutex that protects the variable exit.
*   it must access the variable exit always with a locked mutex.
* - unsigned int frequency: time that the thread should sleep after each query (refresh frequency of market values).
*/
void* stats_reader(void *args){

  // Create a reader_info structure variable to dereference the arguments.
  struct reader_info * data = args;

  // Extract info from te argument. It contains the mutex for the exit variable.
  int *exit = data->exit;
  stock_market *market = data->market;
  pthread_mutex_t *exit_mutex = data->exit_mutex;
  unsigned int frequency = data->frequency;

  //Reader loop - Execute until exit flag is raised. Lock exit mutex.
  pthread_mutex_lock(exit_mutex);
  while(*exit == 0){
    //Unlock exit mutex
    pthread_mutex_unlock(exit_mutex);

    /* --- C R I T I C A L  -  S E C T I O N --- */

    // conc_readers being a global variable, is shared among readers, so we lock it for evaluation and modification.
    pthread_mutex_lock(&conc_readers_mutex);
    // We keep track of the number of concurrent reader threads in execution.
    conc_readers++;

    // Lock the access for writers while there's a reading in progress.
    // The locking of the writers takes place only on the first reader thread
    // that is created. Subsequent reader threads won't be affected by the mutex
    // blockage.
    if (conc_readers == 1){
      pthread_mutex_lock(&read_mutex);
    }
    // Unlock the mutex once it has finished evaluating and modifying it.
    pthread_mutex_unlock(&conc_readers_mutex);

    // View stock market statistics and print them.
    print_market_status(market);

    pthread_mutex_lock(&conc_readers_mutex);
    // After finishing the read operation, we decrement the counter for concurrent readers.
    conc_readers--;


    // Once there are no readers operating, the lock for writers is unlocked.
    if (conc_readers == 0){
      pthread_mutex_unlock(&read_mutex);
    }
    // Unlock the mutex once it has finished evaluating and modifying it.
    pthread_mutex_unlock(&conc_readers_mutex);
    /* ------------------------------------------ */

    // Sleep until the next round of information analysis
    usleep(frequency);
    //Lock exit mutex to check for exit flag value
    pthread_mutex_lock(exit_mutex);
  }
  //Unlock exit mutex and end thread execution
  pthread_mutex_unlock(exit_mutex);
}
