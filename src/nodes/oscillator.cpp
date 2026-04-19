#include <cmath>
#include "oscillator.h"
#include "node.h"

using namespace std;


Oscillator::Oscillator(const qefirParams qefParams) : ProcessNode{qefParams},
                                                    _freqModGain{0},
                                                    _freqModGainPast{0},
                                                    _freqDivider{0x0100},
                                                    _freqMult{0.5f}
{

}

Oscillator::~Oscillator()
{

}

void Oscillator::midiEvents2osc(const std::vector<MidiEvent>& midiIn, std::vector<std::pair<uint64_t, float>>& freqFromMidi, std::vector<std::pair<uint64_t, float>>& modFromMidi)
{
    for (auto mi : midiIn)
    {   switch (mi.status)
        {
        case 0x91:
            switch (mi.data1)
            {
            case 0x25:
                _freqHz = _freqMult * scaleHz[0];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[0]));
                break;
            case 0x24:
                _freqHz = _freqMult * scaleHz[1];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[1]));
                break;
            case 0x2a:
                _freqHz = _freqMult * scaleHz[2];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[2]));
                break;
            case 0x52:
                _freqHz = _freqMult * scaleHz[3];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[3]));
                break;
            case 0x28:
                _freqHz = _freqMult * scaleHz[4];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[4]));
                break;
            case 0x26:
                _freqHz = _freqMult * scaleHz[5];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[5]));
                break;
            case 0x2e:
                _freqHz = _freqMult * scaleHz[6];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[6]));
                break;
            case 0x2c:
                _freqHz = _freqMult * scaleHz[7];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[7]));
                break;
            case 0x30:
                _freqHz = _freqMult * scaleHz[8];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[8]));
                break;
            case 0x2f:
                _freqHz = _freqMult * scaleHz[9];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[9]));
                break;
            case 0x2d:
                _freqHz = _freqMult * scaleHz[10];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[10]));
                break;
            case 0x2b:
                _freqHz = _freqMult * scaleHz[11];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[11]));
                break;
            case 0x31:
                _freqHz = _freqMult * scaleHz[12];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[12]));
                break;
            case 0x37:
                _freqHz = _freqMult * scaleHz[13];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[13]));
                break;
            case 0x33:
                _freqHz = _freqMult * scaleHz[14];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[14]));
                break;
            case 0x35:
                _freqHz = _freqMult * scaleHz[15];
                freqFromMidi.push_back(std::make_pair(mi.sampleTime, _freqMult * scaleHz[15]));
                break;
            case 0x5f:
                if (_freqDivider >> 1 > 0 && _freqDivider < 0x8000)
                {
                    _freqDivider = _freqDivider >> 1;
                    std::cout<<"Diver:"<<_freqDivider<<std::endl;
                    _freqMult = static_cast<float>(_freqDivider) / static_cast<float>(0x0100);
                }
                break;
            case 0x60:
                if (_freqDivider << 1 > 0 && _freqDivider < 0x8000)
                {
                    _freqDivider = _freqDivider << 1;
                    std::cout<<"Diver:"<<_freqDivider<<std::endl;
                    _freqMult = static_cast<float>(_freqDivider) / static_cast<float>(0x0100);
                }
                break;
            default:
                break;
            
            }
        case 0x81:
            break;
        case 0xb1:
            switch (mi.data1)
            {
            case 0x00:
                _freqModGain = static_cast<float>(mi.data2) / 128.0f;
                modFromMidi.push_back(std::make_pair(mi.sampleTime, _freqModGain));
                break;
            default:
                break;
            }
            break;
        default:
            break;
    }
    }

}

void Oscillator::process(const float* input, const size_t inputSize, float* output, const size_t outputSize, std::vector<MidiEvent> ev) 
{
    float deltaPh = _freqHz / qefParams.sampleRate;
    std::vector<std::pair<uint64_t, float>> freqFromMidi;
    std::vector<std::pair<uint64_t, float>> modFromMidi;
    float out[4];

    midiEvents2osc(ev, freqFromMidi, modFromMidi);
    std::vector<std::pair<uint64_t, float>>::iterator itMidi = freqFromMidi.begin();
    std::vector<std::pair<uint64_t, float>>::iterator itModMidi = modFromMidi.begin();
    float freqModGainTemp = _freqModGainPast;

    for (size_t i = 0; i < outputSize; ++i)
    {
        if (itMidi < freqFromMidi.end() && (*itMidi).first < i)
        {
            deltaPh = (*itMidi).second  / qefParams.sampleRate;
            itMidi++;
        }
        if (input && inputSize == outputSize)
        {
            if (itModMidi < modFromMidi.end() && (*itModMidi).first < i)
            {
                _freqModGain = (*itModMidi).second;
                itMidi++;
            }
            out[0] = sinf(2.0f * M_PI * _phase);
            out[1] = sinf(2.0f * M_PI * _phase);
            out[2] = sinf(2.0f * M_PI * _phase);
            out[3] = sinf(2.0f * M_PI * _phase);
            vst1q_f32(&output[i*4], vld1q_f32(out));
            
            freqModGainTemp = _ccMidiInnov * _freqModGain + _ccMidiPast * freqModGainTemp;
            deltaPh = (_freqHz + freqModGainTemp * 10 * _freqHz * input[i*4]) / qefParams.sampleRate;
            _phase += deltaPh ;
        }
        else
        {
            out[0] = sinf(2.0f * M_PI * _phase);
            out[1] = sinf(2.0f * M_PI * _phase);
            out[2] = sinf(2.0f * M_PI * _phase);
            out[3] = sinf(2.0f * M_PI * _phase);
            vst1q_f32(&output[i*4], vld1q_f32(out));
            _phase += deltaPh;
        }
        if (_phase >= 1.0f)
        {
            _phase -= 1.0f;
        }
        _freqModGainPast = freqModGainTemp;
    }
}