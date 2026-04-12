#pragma once
#include <alsa/asoundlib.h>
#include <cmath>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <csignal>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include "qefir.h"
#include "workerThread.h"

// ========================== Ring Buffer ==========================
class RingBuffer {
public:
    RingBuffer(size_t blocks, size_t blockSize);

    // producer calls
    void push(const std::vector<int32_t>& blk);
    // consumer calls
    std::vector<int32_t> pop();
    void notifyAll() {cv.notify_all();}

private:
    std::vector<std::vector<int32_t>> buffer;
    size_t writeIndex;
    size_t readIndex;
    size_t blockCount;
    size_t blockSize;
    size_t filled;
    std::mutex mtx;
    std::condition_variable cv;
};


class AudioIO : public WorkerThread
{
public:
    AudioIO(RingBuffer& rb, const qefirParams& qefParams, const uint8_t coreId, const int priority);
    ~AudioIO();

private:
    void initAudio();
    void run() override; // pętla wątku

    std::thread worker;
    //std::atomic<bool> running{false};
    RingBuffer& audioBuff;
    const qefirParams& qefParams;
    snd_pcm_t *pcm;
};