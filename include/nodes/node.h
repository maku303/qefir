#pragma once
#include <vector>
#include "../midiIO.h"
#include "../qefir.h"

class ProcessNode
{
public:
    ProcessNode(const qefirParams qefParams);
    ~ProcessNode();
    virtual void process(const float* input, const size_t inputSize, float* output, const size_t outputSize, std::vector<MidiEvent> ev) = 0;
protected:
    alignas(64) float buffer[BLOCK_SIZE * NUM_OF_CHANNELS];
    const qefirParams qefParams;
    float _phase;
    float _freqHz;
};