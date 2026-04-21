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
#include "audioIO.h"
#include "dspPipeline.h"
#include "midiIO.h"

extern std::atomic<bool> audioRunning;
extern std::atomic<bool> g_record;
std::atomic<uint64_t> dspFrameTime;

void sigint_handler(int sig)
{
    if (sig == SIGINT)
    {
        audioRunning = false;
    }
    else if (sig == SIGUSR1)
    {
        g_record = !g_record;
    }
}


// ========================== Main ==========================
int main() {
    signal(SIGINT, sigint_handler);
    signal(SIGUSR1, sigint_handler);

    qefirParams qefParams{SAMPLE_RATE, BLOCK_SIZE, QUEUE_SIZE, BUFF_SIZE, NUM_OF_CHANNELS};
    RingBuffer audioBuff(QUEUE_SIZE, BLOCK_SIZE, audioRunning);
    MidiQueue midiQueue{};

    DspManager dsp(audioBuff, midiQueue, qefParams, CORE_NUM_DSP_MASTER_THREAD, PRIORITY_DSP);
    dsp.start();
    AudioIO io(audioBuff, qefParams, CORE_NUM_KERNEL_THREAD, PRIORITY_AUDIO_IO);
    io.start();
    MidiIO midi(midiQueue, qefParams, CORE_NUM_DSP_MASTER_THREAD, PRIORITY_MIDI_IO);
    midi.start();

    return 0;
}