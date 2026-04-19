#include <arm_neon.h>
#include "node.h"

class Oscillator : public ProcessNode
{
    public:
    Oscillator(const qefirParams qefParams);
    ~Oscillator();
    void process(const float* input, const size_t inputSize, float* output, const size_t outputSize, std::vector<MidiEvent> ev) override;
    private:
    void midiEvents2osc(const std::vector<MidiEvent>& midiIn, std::vector<std::pair<uint64_t, float>>& freqFromMidi, std::vector<std::pair<uint64_t, float>>& modFromMidi);
    const float scaleHz[18]{432, 450.02, 468.79, 488.34, 508.71, 529.92, 552.02, 575.04, 
                            599.02, 624.00, 650.02, 677.12, 705.35, 734.76, 765.39, 797.30, 830.54, 864.00};
    float _freqModGain;
    float _freqModGainPast;
    const float _ccMidiInnov{0.0008};
    const float _ccMidiPast{0.9992};
    uint16_t _freqDivider;
    float _freqMult;
};