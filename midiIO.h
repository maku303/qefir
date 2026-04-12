#pragma once
#include <alsa/asoundlib.h>
#include <atomic>
#include <thread>
#include <vector>
#include <iostream>
#include "qefir.h"
#include "workerThread.h"

constexpr int MIDI_QUEUE_SIZE = 1024;

struct MidiEvent {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    uint64_t sampleTime;
};

class MidiQueue {
public:
    MidiQueue();
    MidiEvent buffer[MIDI_QUEUE_SIZE];
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};

    bool push(const MidiEvent& e);
    bool pop(MidiEvent& e);
    bool peek(MidiEvent& e);
};


class MidiIO: public WorkerThread {
public:
    MidiIO(MidiQueue& midiQ, const qefirParams& qefParams, const uint8_t coreId, const int priority);
    ~MidiIO();

private:
    void initMidi();
    void run() override;

    MidiQueue& midiQ;
    std::thread worker;
    const qefirParams& qefParams;
    snd_rawmidi_t* midi_in;
};