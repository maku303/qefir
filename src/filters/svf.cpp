#include <cmath>
#include "svf.h"



// float process_down(float y0, float y1)
// {
//     float f0 = lpf.process(y0);
//     float f1 = lpf.process(y1);
//     return f1; // ↓2
// }

// for (...)
// {
//     float32x4_t x = vld1q_f32(input);

//     float32x4_t x0, x1;
//     upsampler.process(x, x0, x1);

//     float32x4_t y0 = ladder.process(x0);
//     float32x4_t y1 = ladder.process(x1);

//     // downsample (scalar per lane albo później wektorowo)
//     float out[4];
//     for (int i = 0; i < 4; ++i)
//         out[i] = downsample(y0[i], y1[i]);

//     vst1q_f32(output, vld1q_f32(out));
// }


SVF::SVF(const qefirParams& qefParams): FilterNode{qefParams}, 
                                        state1{0},
                                        state2{0},
                                        state3{0},
                                        state4{0},
                                        _last_y{0},
                                        _freqRezo{1},
                                        _freqCutOff{1},
                                        _gPast{0},
                                        _freqRezoPast{0}
{}
    
SVF::~SVF()
{
}

void SVF::midiEvents2filt(const std::vector<MidiEvent>& midiIn, std::vector<std::pair<uint64_t, float32x4_t>>& paramsFromMidi)
{
    for (auto mi : midiIn)
    {   
        switch (mi.status)
        {
        case 0x91:
            default:
                break;
        case 0xb1:
            switch (mi.data1)
            {
            case 0x01:
                _freqCutOff = vdupq_n_f32(_freqCutOff_min * powf(_freqqCutOff_max / _freqCutOff_min, mi.data2 / 127.0f));
                //TODO maintain 4x tanf
                _g = vdupq_n_f32(tanf(_freqCutOff[0] * M_PI / (qefParams.sampleRate * 2.0f))); //oversampling frequency
                paramsFromMidi.push_back(std::make_pair(mi.sampleTime, _freqCutOff));

                break;
            case 0x02:
                _freqRezo = vdupq_n_f32(static_cast<float>(mi.data2) / 16.0f);
                paramsFromMidi.push_back(std::make_pair(mi.sampleTime, _freqRezo));
                break;
            default:
                break;
            }
            break;
        }
    }
}

inline float32x4_t fast_tanh(float32x4_t x)
{
    // x / (1 + |x|)
    float32x4_t absx = vabsq_f32(x);
    float32x4_t denom = vaddq_f32(vdupq_n_f32(1.0f), absx);
    return vdivq_f32(x, denom);
}

inline float32x4_t fast_tanh2(float32x4_t x)
{
    float32x4_t x2 = vmulq_f32(x, x);
    float32x4_t num = vmulq_f32(x, vaddq_f32(vdupq_n_f32(27.0f), x2));
    float32x4_t den = vaddq_f32(vdupq_n_f32(27.0f), vmulq_n_f32(x2, 9.0f));
    return vdivq_f32(num, den);
}

inline float32x4_t asym_tanh(float32x4_t x)
{
    float32x4_t pos = vmaxq_f32(x, vdupq_n_f32(0.0f));
    float32x4_t neg = vminq_f32(x, vdupq_n_f32(0.0f));

    pos = fast_tanh(pos);
    neg = vmulq_n_f32(fast_tanh(neg), 0.8f); // asymetria

    return vaddq_f32(pos, neg);
}

void SVF::ladder(const float32x4_t& in, float32x4_t& out, const float32x4_t& gTemp)
{
    float32x4_t y = _last_y; // initial guess

    for (int i = 0; i < 2; ++i) // 1–2 iteracje wystarczą
    {
        float32x4_t u = vsubq_f32(in, vmulq_f32(_freqRezo, y));

        float32x4_t v1 = asym_tanh(vsubq_f32(u, state1));
        float32x4_t v2 = asym_tanh(vsubq_f32(state1, state2));
        float32x4_t v3 = asym_tanh(vsubq_f32(state2, state3));
        float32x4_t v4 = asym_tanh(vsubq_f32(state3, state4));


        float32x4_t x1 = vmlaq_f32(state1, gTemp, v1);
        float32x4_t x2 = vmlaq_f32(state2, gTemp, v2);
        float32x4_t x3 = vmlaq_f32(state3, gTemp, v3);
        float32x4_t x4 = vmlaq_f32(state4, gTemp, v4);

        y = fast_tanh(x4);
    }

    // update state
    float32x4_t u = vsubq_f32(in, vmulq_f32(_freqRezo, y));

    state1 = vmlaq_f32(state1, gTemp, asym_tanh(vsubq_f32(u, state1)));
    state2 = vmlaq_f32(state2, gTemp, asym_tanh(vsubq_f32(state1, state2)));
    state3 = vmlaq_f32(state3, gTemp, asym_tanh(vsubq_f32(state2, state3)));
    state4 = vmlaq_f32(state4, gTemp, asym_tanh(vsubq_f32(state3, state4)));

    out = y;
    _last_y = y;
}

void SVF::process(const float* input, const size_t inputSize, float* output, const size_t outputSize, std::vector<MidiEvent> ev)
{
    //TODO create special structure for midi params
    std::vector<std::pair<uint64_t, float32x4_t>> paramsFromMidi;
    midiEvents2filt(ev, paramsFromMidi);

    float32x4_t gTemp = _gPast;
    float32x4_t freqRezoTemp = _freqRezoPast;
    float32x4_t y0{0};
    float32x4_t y1{0};
    float out[4];
    for (size_t i=0; i < inputSize*4; i+=4)
    {
        gTemp = _ccMidiInnov * _g + _ccMidiPast * gTemp;
        freqRezoTemp = _ccMidiInnov * _freqRezo + _ccMidiPast * freqRezoTemp;

        float32x4_t x = vld1q_f32(&input[i]);
        float32x4_t x0, x1;
        _upSmplr.process(x, x0, x1);

        ladder(x0, y0, gTemp);
        ladder(x1, y1, gTemp);

        for (size_t i = 0; i < 4; ++i)
        {
            float f0 = _lpf.process(y0[i]);
            float f1 = _lpf.process(y1[i]);
            out[i] = f1;
        }
        vst1q_f32(&output[i], vld1q_f32(out));
        
    }
    _gPast = gTemp;
    _freqRezoPast = freqRezoTemp;
}