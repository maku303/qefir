#include "dspPipeline.h"

std::atomic<bool> dspRunning(true);
extern std::atomic<uint64_t> dspSampleTime;
std::chrono::time_point<std::chrono::high_resolution_clock> dspStart;


DspManager::DspManager(RingBuffer& rb, MidiQueue& midiQ, const qefirParams qefParams, const uint8_t coreId, const int priority):
                WorkerThread(coreId, priority, "DspManagerThread"),  
                audioBuff{rb},
                midiQ{midiQ},
                qefParams{qefParams}
{
}

DspManager::~DspManager()
{
    stop();
}

void DspManager::run()
{
    int phase = 0;
    size_t blockNum = 0;
    size_t counter = 0;
    MidiEvent ev;
    dspStart = std::chrono::high_resolution_clock::now();
    uint64_t dspSampleTimePrev;
    dspRunning = true;
    while (audioRunning || blockNum)
    {
        auto now = std::chrono::high_resolution_clock::now();
        double seconds = std::chrono::duration<double>(now - dspStart).count();
        dspSampleTimePrev = dspSampleTime;
        dspSampleTime.store(static_cast<uint64_t>(seconds * qefParams.sampleRate));
        while (midiQ.peek(ev) && ev.sampleTime <= dspSampleTime)
        {
            midiQ.pop(ev);
            std::cout<<"midi ev sample time:"<<ev.sampleTime - dspSampleTimePrev<<", dsp sample time:"<<dspSampleTime<<std::endl;
        }
        //TODO 
        //UPMIX 2 N channels
        std::vector<int32_t> block(qefParams.numOfChannel * qefParams.blockSize);
        for (int i = 0; i < qefParams.blockSize; ++i) {
            block[i * qefParams.numOfChannel] = static_cast<int32_t>(sinf(2.0f * M_PI * phase / qefParams.blockSize) * 2147483647.0f);
            block[i * qefParams.numOfChannel +1] = static_cast<int32_t>(sinf(2.0f * M_PI * phase / qefParams.blockSize) * 2147483647.0f);
            phase = (phase + 1) % qefParams.blockSize;
        }
        //
        audioBuff.push(block);
        blockNum = (++blockNum) % qefParams.queueSize;
        if (++counter % 100 == 0) {
            //std::cout << "Blocks generated: " << counter << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    dspRunning.store(false);
}
