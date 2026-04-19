#pragma once
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

class WorkerThread {
public:
    WorkerThread(const int coreId, const int priority, const std::string& name);
    virtual ~WorkerThread();
    void start();
    void stop();

protected:
    std::atomic<bool> running_{false};
    virtual void run() = 0;

private:
    std::thread worker_;
    const int coreId_;
    const int priority_;
    std::string name_;

    void threadEntry();
    void setAffinity(int coreId);
    void setRealtimePriority(int priority);
};
