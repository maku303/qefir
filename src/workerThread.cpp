#include <cstring>
#include "workerThread.h"


WorkerThread::WorkerThread(const int coreId, const int priority, const std::string& name)
        : coreId_(coreId), priority_(priority), name_(name) {}

WorkerThread::~WorkerThread()
{
    stop();
}

void WorkerThread::start()
{
    stop();
    running_ = true;
    worker_ = std::thread(&WorkerThread::threadEntry, this);
}

void WorkerThread::stop()
{
    running_ = false;
    if (worker_.joinable())
        worker_.join();
}

void WorkerThread::threadEntry()
{
    setAffinity(coreId_);
    setRealtimePriority(priority_);
    run();
}

void WorkerThread::setRealtimePriority(int priority)
{
    sched_param sch{};
    sch.sched_priority = priority;

    int rc = pthread_setschedparam(
        pthread_self(),
        SCHED_FIFO,
        &sch
    );

    if (rc != 0)
    {
        std::cerr << "Failed to set RT priority: "
                  << std::strerror(rc)
                  << std::endl;
    }

    int policy;
    pthread_getschedparam(pthread_self(), &policy, &sch);
    std::cout << name_ << ": Policy: " << policy
            << " priority: " << sch.sched_priority
            << std::endl;
}

void WorkerThread::setAffinity(int coreId) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);

    int rc = pthread_setaffinity_np(
        pthread_self(),
        sizeof(cpu_set_t),
        &cpuset
    );

    if (rc != 0) {
        std::cerr << name_
                    << ": affinity error = "
                    << rc << std::endl;
    } else {
        std::cout << name_
                    << " pinned to core "
                    << coreId << std::endl;
    }
}

