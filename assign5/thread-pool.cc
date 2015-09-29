/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
using namespace std;

/*
 *  Constructor. Initializes data structures and prepares thread pool for
 *  operation
 */
ThreadPool::ThreadPool(size_t numThreads) : numThreads(numThreads), ws(numThreads) {
    threadRun = true;
    jobsCount=0;
    currentThreads = 0;
    numAvailableThreads = 0;
    // Spawning dispatcher thread.
    dt = thread(
                [this]() {
                    dispatcher();
                }
               );
    for (size_t workerID = 0; workerID < numThreads; workerID++) { 
        ws[workerID].available = false;
    }
}

/*
 * Public method defintion of wait.
 * Suspends if jobsCount is greater than 0.
 */
void ThreadPool::wait()  {waitForCompletion();}
void ThreadPool::waitForCompletion() {
    lock_guard<mutex> lg(m);
    cv.wait(m, [this]{ return jobsCount == 0;});
}

/*
 * Increments jobsCount
 */
void ThreadPool::incrementJobsCount() {
    lock_guard<mutex> lg(m);
    jobsCount++;
}

/*
 * Decrements jobs and notifiies cv
 */
void ThreadPool::decrementJobsCount() {
    lock_guard<mutex> lg(m);
    jobsCount--;
    if (jobsCount == 0) cv.notify_all();
}

/*
 * Scheduler definition
 */
void ThreadPool::schedule(const std::function<void(void)>& thunk) {
    addJob(thunk);
    incrementJobsCount();
    addedJob.signal();
}

/*
 * Assigns job to worker, dequeues job from jobQueue
 */
void ThreadPool::assignJobAndDequeue(workerStatus& w) {
    lock_guard<mutex> lg(jobMutex);
    w.thunk = jobs.front();
    jobs.pop();
}

/*
 * Adds job to jobQueue
 */
void ThreadPool::addJob(const std::function<void(void)>& thunk) {
    lock_guard<mutex> lg(jobMutex);
    jobs.push(thunk);
}

/*
 * Increments available threads
 */
void ThreadPool::incrementAvailableThreads() {
    lock_guard<mutex> lg(availableMutex);
    numAvailableThreads += 1;
}

/*
 * Decrements available threads
 */
void ThreadPool::decrementAvailableThreads() {
    lock_guard<mutex> lg(availableMutex);
    numAvailableThreads -= 1;
}

/*
 * Gets available threads count
 */
int ThreadPool::getAvailableThreadCount() {
    lock_guard<mutex> lg(availableMutex);
    return numAvailableThreads;
}

/*
 * Private method for spawning new thread
 */
void ThreadPool::createNewThread() {
    if (!threadRun)
        return;
    size_t workerID = currentThreads;
    wts.push_back(thread(
                           [this](size_t workerID) {
                                worker(workerID);
                           }, workerID 
                          ));
    availableThread.signal();
    ws[workerID].available = true;
    currentThreads++;
    incrementAvailableThreads();
}

/*
 * Dispatches job to available worker.
 * Gets awoken by a signal to availableThread semaphore
 */
void ThreadPool::dispatchJob() {
    if (!currentThreads)
        return;
    availableThread.wait();
    if (!threadRun)
        return;

    int depth = 0; 
    for (workerStatus& w: ws) {
        if (depth >= currentThreads)
            break;
        if (w.available) {
                assignJobAndDequeue(w);
                w.available = false; 
                decrementAvailableThreads();
                w.workerSemaphore.signal();
                break;
        }
        depth++; 
    }
}

/*
 * Dispatcher definition
 */
void ThreadPool::dispatcher() {
    while (true) {
        addedJob.wait();
        if (!threadRun) 
            return;

        /*
         * Spawns new threads only if necessary
         */
        if (!getAvailableThreadCount()) {
            if ((currentThreads + 1) <= numThreads)
                createNewThread();
        }

        if (!threadRun) 
            return;

        dispatchJob();

        if (!threadRun) 
            return;
    }
}

/*
 * Worker definition
 */
void ThreadPool::worker(int workerId) {
    workerStatus& me = ws.at(workerId);
    while (true) {
        me.workerSemaphore.wait();
        if (!threadRun)
            return;
        me.thunk();
        decrementJobsCount();
        me.available = true;
        incrementAvailableThreads();
        availableThread.signal();
    }
}

/*
 * Destructor definition
 */
ThreadPool::~ThreadPool() {
    wait();
    // Change state variable
    threadRun = false;
    // Signalling all semaphores 
    addedJob.signal();
    availableThread.signal();
    int depth = 0;
    for (workerStatus& worker: ws) {
        if (depth >= currentThreads)
            break;
        worker.workerSemaphore.signal(); 
        depth++;
    }
    dt.join();
    for (thread& t: wts) {
        t.join();
    }
}
