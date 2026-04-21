#pragma once
#include "audioIO.h"
#include "midiIO.h"
#include "workerThread.h"


extern std::atomic<bool> audioRunning;

class DspManager : public WorkerThread {
public:
    DspManager(RingBuffer& rb, MidiQueue& midiQ, const qefirParams qefParams, const uint8_t coreId, const int priority);
    ~DspManager();

private:
    void run() override; // pętla wątku

    std::thread worker;
    RingBuffer& audioBuff;
    MidiQueue& midiQ;
    const qefirParams qefParams;
};
