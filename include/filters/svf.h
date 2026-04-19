#include <arm_neon.h>
#include "filterNode.h"

struct Upsampler2x {
    float32x4_t prev;

    Upsampler2x() {
        prev = vdupq_n_f32(0.0f);
    }

    inline void process(float32x4_t x, float32x4_t& out0, float32x4_t& out1)
    {
        out0 = x;

        // (x + prev) * 0.5
        float32x4_t sum = vaddq_f32(x, prev);
        out1 = vmulq_n_f32(sum, 0.5f);

        prev = x;
    }
};

struct Biquad {
    float b0 = 0.206572, b1 = 0.413144, b2 = 0.206572, a1 = -0.369527, a2 = 0.195816;
    float z1 = 0, z2 = 0;

    inline float process(float x)
    {
        float y = b0*x + z1;
        z1 = b1*x - a1*y + z2;
        z2 = b2*x - a2*y;
        return y;
    }
};

class SVF : public FilterNode
{
public:
    SVF(const qefirParams& qefParams);
    ~SVF();
    void process(const float* input, const size_t inputSize, float* output, const size_t outputSize, std::vector<MidiEvent> ev) override;
    void midiEvents2filt(const std::vector<MidiEvent>& midiIn, std::vector<std::pair<uint64_t, float32x4_t>>& paramsFromMidi);
    void ladder(const float32x4_t& in, float32x4_t& out, const float32x4_t& gTemp);
private:
    float32x4_t state1;
    float32x4_t state2;
    float32x4_t state3;
    float32x4_t state4;
    float32x4_t _last_y;
    float32x4_t _freqRezo;
    float32x4_t _freqCutOff;
    float32x4_t _g;
    const float _ccMidiInnov{0.0008};
    const float _ccMidiPast{0.9992};
    float32x4_t _gPast;
    float32x4_t _freqRezoPast;
    Biquad _lpf;
    Upsampler2x _upSmplr;
    const float _freqCutOff_min{20.0f};
    const float _freqqCutOff_max{20000.0f};
};