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
    RingBuffer(size_t blocks, size_t blockSize, std::atomic<bool>& run);

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
    std::atomic<bool>& running;
};

class AudioCapture : public WorkerThread
{
public:
    AudioCapture(RingBuffer& buff, const uint8_t coreId, const int priority, const size_t numOfChannels, const size_t sampleRate);
    ~AudioCapture();
    bool isRunning() const
    {
        return running_;
    }
    void stopRunning()
    {
        running_ = false;
        buffRec.notifyAll();
        stop();
    }

private:
    void run() override; // pętla wątku
    RingBuffer& buffRec;
    const size_t numOfChannels;
    const size_t sampleRate;
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
    const qefirParams& qefParams;
    RingBuffer& audioBuff;
    RingBuffer captureBuff;
    snd_pcm_t *pcm;
    AudioCapture rec;
};
