#include "midiIO.h"
#include "workerThread.h"

extern std::atomic<uint64_t> dspFrameTime;
extern std::chrono::time_point<std::chrono::high_resolution_clock> dspStart;
extern std::atomic<bool> dspRunning;

MidiQueue::MidiQueue()
{}

bool MidiQueue::push(const MidiEvent& e) {
    size_t h = head.load(std::memory_order_relaxed);
    size_t next = (h + 1) % MIDI_QUEUE_SIZE;

    if (next == tail.load(std::memory_order_acquire)) {
        return false; // overflow
    }

    buffer[h] = e;
    head.store(next, std::memory_order_release);
    return true;
}

bool MidiQueue::pop(MidiEvent& e) {
    size_t t = tail.load(std::memory_order_relaxed);

    if (t == head.load(std::memory_order_acquire)) {
        return false;
    }

    e = buffer[t];
    tail.store((t + 1) % MIDI_QUEUE_SIZE, std::memory_order_release);
    return true;
}

bool MidiQueue::peek(MidiEvent& e) {
    size_t t = tail.load(std::memory_order_relaxed);

    if (t == head.load(std::memory_order_acquire))
        return false;

    e = buffer[t];
    return true;
}

MidiIO::MidiIO(MidiQueue& midiQ, const qefirParams& qefParams, const uint8_t coreId, const int priority):
                WorkerThread(coreId, priority, "MidiIOthread"),
                midiQ{midiQ},
                qefParams{qefParams}

{
    initMidi();
}

MidiIO::~MidiIO()
{
    stop();
}

void MidiIO::initMidi()
{
    if (snd_rawmidi_open(&midi_in, nullptr, "hw:0,0,0", SND_RAWMIDI_NONBLOCK) < 0) {
        std::cerr << "Cannot open MIDI device\n";
        return;
    }
    std::cout << "RAW MIDI opened\n";
}

void MidiIO::run()
{
    uint8_t byte;
    uint8_t msg[3];
    int msgIndex = 0;

    auto start = std::chrono::high_resolution_clock::now();
    struct pollfd pfd;
    snd_rawmidi_poll_descriptors(midi_in, &pfd, 1);

    while (dspRunning.load()) {
        int ret = poll(&pfd, 1, 200); // timeout 100 ms
        int res = 0;
        if (ret > 0)
        {
            res = snd_rawmidi_read(midi_in, &byte, 1);
        }

        if (res == 1) {

            // prosty parser MIDI (NOTE ON/OFF, CC)
            if (byte & 0x80) {
                // status byte
                msg[0] = byte;
                msgIndex = 1;
            } else {
                if (msgIndex < 3) {
                    msg[msgIndex++] = byte;
                }

                if (msgIndex == 3) {

                    MidiEvent ev{};
                    ev.status = msg[0];
                    ev.data1 = msg[1];
                    ev.data2 = msg[2];

                    ev.dspFrameTime = dspFrameTime.load();
                    auto now = std::chrono::high_resolution_clock::now();
                    double seconds = std::chrono::duration<double>(now - dspStart).count();
                    ev.sampleTime = static_cast<uint64_t>(seconds * qefParams.sampleRate) - ev.dspFrameTime;

                    midiQ.push(ev);
                    std::cout << "MIDI: "
                              << std::hex << int(ev.status) << " "
                              << int(ev.data1) << " "
                              << int(ev.data2) << "\n";

                    msgIndex = 1; // running status (opcjonalnie)
                }
            }
        }
    }

    snd_rawmidi_close(midi_in);
}