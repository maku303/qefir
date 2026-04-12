#pragma once
#include <iostream>
constexpr size_t SAMPLE_RATE(48000);
constexpr size_t BLOCK_SIZE = 128;
constexpr size_t QUEUE_SIZE = 2;
constexpr size_t BUFF_SIZE = BLOCK_SIZE * 2;
constexpr size_t NUM_OF_CHANNELS = 4;

constexpr uint8_t CORE_NUM_KERNEL_THREAD = 1;
constexpr uint8_t CORE_NUM_DSP_MASTER_THREAD = 2;
constexpr uint8_t CORE_NUM_DSP_SLAVE_THREAD = 3;
constexpr uint8_t CORE_NUM_DSP_EFFECTS_THREAD = 4;
constexpr uint8_t CORE_NUM_DSP_GPU_THREAD = 5;
constexpr uint8_t CORE_NUM_DSP_GUI_THREAD = 6;

constexpr int PRIORITY_AUDIO_IO = 50;
constexpr int PRIORITY_MIDI_IO = 15;
constexpr int PRIORITY_DSP = 40;

struct qefirParams
{
    size_t sampleRate;
    size_t blockSize;
    size_t queueSize;
    size_t buffSize;
    size_t numOfChannel;
};