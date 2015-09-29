/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector
#include <queue>
#include <ostream>
#include <iostream>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "ostreamlock.h"
#include "semaphore.h"

class ThreadPool {
 public:

/**
 * Constructs a ThreadPool configured to spawn up to the specified
 * number of threads.
 */
  ThreadPool(size_t numThreads);

/**
 * Schedules the provided thunk (which is something that can
 * be invoked as a zero-argument function without a return value)
 * to be executed by one of the ThreadPool's threads as soon as
 * all previously scheduled thunks have been handled.
 */
  void schedule(const std::function<void(void)>& thunk);

/**
 * Blocks and waits until all previously scheduled thunks
 * have been executed in full.
 */
  void wait();

/**
 * Waits for all previously scheduled thunks to execute, then waits
 * for all threads to be be destroyed, and then otherwise brings
 * down all resources associated with the ThreadPool.
 */
  ~ThreadPool();
  
 private:
  // Keeps track of max number of threads that could be spawned in a thread pool
  int numThreads;
  // Counter for active jobs
  int jobsCount;
  // State variable for dispatcher and workers to exist
  bool threadRun;
  // Currently spawned threads
  int currentThreads;
  // Count of Available threads for dispatching a job
  int numAvailableThreads;

  // Worker Status struct definition
typedef struct {
    // Flag
    bool available;
    // Semaphore on which worker suspends
    semaphore workerSemaphore;
    // Function to be executed.
    std::function<void(void)> thunk;
} workerStatus;

  std::thread dt;                // dispatcher thread handle
  std::vector<std::thread> wts;  // worker thread handles
  // Mutex used by condition variable
  std::mutex m;
  // Mutex guarding available thread counter
  std::mutex availableMutex;
  // Mutex for job queue
  std::mutex jobMutex;
  // Condition variable for wait method
  std::condition_variable_any cv;
  // This worker is not protected by mutex because 
  // dispatcher and workers cannot race against each other
  // while manipulating its contents.
  std::vector<workerStatus> ws;   // worker status
  // Semaphore between workers and dispatcher
  semaphore availableThread;
  // Semaphore between dispatcher and scheduler
  semaphore addedJob;
  // Job queue
  std::queue<std::function<void(void)>> jobs;
  // Private methods
  void dispatcher();
  void worker(int);
  void waitForCompletion();
  void incrementJobsCount();
  void decrementJobsCount();
  void incrementAvailableThreads();
  void decrementAvailableThreads();
  int getAvailableThreadCount();
  void createNewThread();
  void dispatchJob();
  void assignJobAndDequeue(workerStatus&);
  void addJob(const std::function<void(void)>&);

/**
 * ThreadPools are the type of thing that shouldn't be cloneable, since it's
 * not clear what it means to clone a ThreadPool (should copies of all outstanding
 * functions to be executed be copied?).
 *
 * In order to prevent cloning, we remove the copy constructor and the
 * assignment operator.  By doing so, the compiler will ensure we never clone
 * a ThreadPool.
 */
  ThreadPool(const ThreadPool& original) = delete;
  ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif
