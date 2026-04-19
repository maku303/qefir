#include "dspPipeline.h"
#include "oscillator.h"
#include "svf.h"

using namespace std;

std::atomic<bool> dspRunning(true);
extern std::atomic<uint64_t> dspFrameTime;
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
    size_t blockNum = 0;
    size_t counter = 0;
    MidiEvent ev;
    dspStart = std::chrono::high_resolution_clock::now();
    uint64_t dspFrameTimePrev;

    //create pipeline
    alignas(16) float buffer128_fm_x4[qefParams.blockSize*4]{0};
    alignas(16) float buffer128_osc_x4[qefParams.blockSize*4]{0};
    alignas(16) float buffer128_svf_x4[qefParams.blockSize*4]{0};
    alignas(16) float buffer128_outSt_x2[qefParams.blockSize*2]{0};
    //alignas(16) float buffer128_4lane[qefParams.blockSize*4]{0};
    Oscillator osc1_x4 = Oscillator(qefParams);
    Oscillator osc2FmMod_x4 = Oscillator(qefParams);
    SVF svfFilter_x4 = SVF(qefParams);
    //TODO
    //convert to array
    std::vector<int32_t> block;
    block.resize(qefParams.numOfChannel * qefParams.blockSize);


    dspRunning = true;
    while (audioRunning || blockNum)
    {
        auto now = std::chrono::high_resolution_clock::now();
        double seconds = std::chrono::duration<double>(now - dspStart).count();
        dspFrameTimePrev = dspFrameTime;
        dspFrameTime.store(static_cast<uint64_t>(seconds * qefParams.sampleRate));

        vector<MidiEvent> midiVecFilt;
        while (midiQ.peek(ev) && ev.dspFrameTime < dspFrameTime)
        {
            midiQ.pop(ev);
            //filter midi event
            midiVecFilt.push_back(ev);
            std::cout<<"DspManager::run() - MIDI event("<<(int)ev.status<<" "<<(int)ev.data1<<" "<<(int)ev.data2<<"), sample in frame:"<<ev.sampleTime<<", DSP sample time:"<<dspFrameTime<<std::endl;
        }
        //INSERT PROCESSING GRAPH
        osc2FmMod_x4.process(nullptr, 0, buffer128_fm_x4, qefParams.blockSize, midiVecFilt);
        osc1_x4.process(buffer128_fm_x4, qefParams.blockSize, buffer128_osc_x4, qefParams.blockSize, midiVecFilt);
        //svFilterx4
        svfFilter_x4.process(buffer128_osc_x4, qefParams.blockSize, buffer128_svf_x4, qefParams.blockSize, midiVecFilt);

        //TODO, add stereo mixing module
        for (size_t i = 0; i < (qefParams.blockSize); ++i)
        {
            buffer128_outSt_x2[i*2] = buffer128_svf_x4[i*4];
            buffer128_outSt_x2[i*2+1] = buffer128_svf_x4[i*4]; //buffer128_svf_x4[i*4+1]
        }
        
        
        //output 2 channels
        for (size_t i = 0; i < (2*qefParams.blockSize); i+=4) {
            float32x4_t inFloat = vld1q_f32(&buffer128_outSt_x2[i]);
            float32x4_t clamp = vmaxq_f32(vdupq_n_f32(-1.0f), vminq_f32(vdupq_n_f32(1.0f), inFloat));
            float32x4_t scaled = vmulq_n_f32(clamp, 2147483647.0f);
            int32x4_t out = vcvtq_s32_f32(scaled);
            int32x2_t low  = vget_low_s32(out);   // [a, b]
            int32x2_t high = vget_high_s32(out);  // [c, d]
            int32x4_t v1 = vcombine_s32(low, vdup_n_s32(0));
            int32x4_t v2 = vcombine_s32(high, vdup_n_s32(0));
            vst1q_s32(&block[i*2], v1);
            vst1q_s32(&block[i*2+4], v2);
            //block[i * qefParams.numOfChannel] = static_cast<int32_t>(buffer128_x4[i] * 2147483647.0f);
            //block[i * qefParams.numOfChannel +1] = static_cast<int32_t>(buffer128_x4[i] * 2147483647.0f);
        }
        //
        audioBuff.push(block);
        blockNum = (++blockNum) % qefParams.queueSize;
        //if (++counter % 100 == 0) {
            //std::cout << "Blocks generated: " << counter << std::endl;
        //}
    }
    dspRunning.store(false);
}
