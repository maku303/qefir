#include <fstream>
#include "audioIO.h"

std::atomic<bool> audioRunning(true);
std::atomic<bool> g_record{false};
extern std::atomic<bool> dspRunning;


RingBuffer::RingBuffer(size_t blocks, size_t blockSize, std::atomic<bool>& run)
        : buffer(blocks, std::vector<int32_t>(blockSize)), 
                        writeIndex(0), 
                        readIndex(0), 
                        blockCount(blocks), 
                        blockSize(blockSize), 
                        filled(0),
                        running(run)
{}

// producer calls
void RingBuffer::push(const std::vector<int32_t>& blk) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&]{ return filled < blockCount || !running; });
    buffer[writeIndex] = blk;
    writeIndex = (writeIndex + 1) % blockCount;
    filled++;
    cv.notify_all();
}

// consumer calls
std::vector<int32_t> RingBuffer::pop() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&]{ return filled > 0 || !running; });
    if (filled == 0)
    {
        std::cout<<"Underrun"<<std::endl;
        return std::vector<int32_t>(blockSize, 0); // cisza  
    }
    auto blk = buffer[readIndex];
    readIndex = (readIndex + 1) % blockCount;
    filled--;
    cv.notify_all();
    return blk;
}

struct WavHeader {
    char riff[4] = {'R','I','F','F'};
    uint32_t chunkSize;
    char wave[4] = {'W','A','V','E'};

    char fmt[4] = {'f','m','t',' '};
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;

    char data[4] = {'d','a','t','a'};
    uint32_t dataSize;
};

AudioIO::AudioIO(RingBuffer& rb, const qefirParams& qefParams, uint8_t coreId, const int priority):
                WorkerThread(coreId, priority, "AudioIOthread"),
                qefParams(qefParams),
                audioBuff(rb),
                captureBuff{16, qefParams.blockSize, g_record},
                rec{captureBuff, coreId, priority - 20, qefParams.numOfChannel, qefParams.sampleRate}
{    
    initAudio();
}

void AudioIO::initAudio()
{
    int err;
    if ((err = snd_pcm_open(&pcm, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cerr << "Cannot open PCM: " << snd_strerror(err) << "\n";
        return;
    }

    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t period_size = qefParams.blockSize;
    snd_pcm_uframes_t buffer_size = qefParams.buffSize; // 2 bloki (low latency)

    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm, params);
    snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    err = snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S32_LE);
    if (err < 0) {
        std::cout << "pcm_hw_params_set_format error: " << snd_strerror(err) << "\n";
        return;
    }
    err = snd_pcm_hw_params_set_channels(pcm, params, 4);
    if (err < 0) {
        std::cout << "pcm_hw_params_set_channels error: " << snd_strerror(err) << "\n";
        return;
    }
    snd_pcm_hw_params_set_period_size_near(pcm, params, &period_size, 0);
    snd_pcm_hw_params_set_buffer_size_near(pcm, params, &buffer_size);
    unsigned int rate = qefParams.sampleRate;
    snd_pcm_hw_params_set_rate_near(pcm, params, &rate, 0);
    snd_pcm_hw_params(pcm, params);
    snd_pcm_hw_params_free(params);

    //----print format info
    snd_pcm_format_t fmt;
    snd_pcm_hw_params_get_format(params, &fmt);
    std::cout << "FORMAT: " << snd_pcm_format_name(fmt) << "\n";
    unsigned int rate1;
    snd_pcm_hw_params_get_rate(params, &rate1, 0);
    std::cout << "RATE: " << rate1 << "\n";
    unsigned int channels;
    snd_pcm_hw_params_get_channels(params, &channels);
    std::cout << "CHANNELS: " << channels << "\n";
    //----   
}

AudioIO::~AudioIO()
{
    stop();
}

void AudioIO::run()
{
    int err = 0;
    int counter = 0;
    size_t blockNum = 0;
    audioRunning = true;
    while (dspRunning || blockNum) {
        auto block = audioBuff.pop();

        err = snd_pcm_writei(pcm, block.data(), qefParams.blockSize);
        if (err < 0) {
            err = snd_pcm_recover(pcm, err, 0);
            if (err < 0) {
                std::cerr << "ALSA write error: " << snd_strerror(err) << "\n";
                break;
            }
        }
        if (++counter % 100 == 0) {
            //std::cout << "Blocks played: " << counter << std::endl;
        }

        if (g_record)
        {
            if (!rec.isRunning())
            {
                rec.start();
            }
            captureBuff.push(block);
        }
        else
        {
            if (rec.isRunning())
            {
                rec.stopRunning();
            } 
        }


        blockNum = (++blockNum) % qefParams.queueSize;
    }
    audioRunning = false;
    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
}

AudioCapture::AudioCapture(RingBuffer& buff, const uint8_t coreId, const int priority, const size_t numOfChannels, const size_t sampleRate) : 
                                                                WorkerThread(coreId, priority, "CaptureThread"),
                                                                buffRec{buff},
                                                                numOfChannels{numOfChannels},
                                                                sampleRate{sampleRate}
{

}

AudioCapture::~AudioCapture()
{
    stop();
}

void AudioCapture::run()
{
    WavHeader h;
    h.numChannels = numOfChannels;
    h.sampleRate = sampleRate;
    h.bitsPerSample = 32;

    h.byteRate = h.sampleRate * h.numChannels * (h.bitsPerSample / 8);
    h.blockAlign = h.numChannels * (h.bitsPerSample / 8);
    h.dataSize = 0; // uzupełnisz później
    h.chunkSize = 36 + h.dataSize;

    std::ofstream file("out.wav", std::ios::binary);
    file.write((char*)&h, sizeof(h));  
    std::cout<<"AudioCapture::run() - started recording"<<std::endl;
    while (g_record.load())
    {
        std::vector<int32_t> buffer = buffRec.pop();
        file.write((char*)buffer.data(),  buffer.size() * sizeof(int32_t));
        h.dataSize += buffer.size() * sizeof(int32_t);
    }

    file.seekp(0);
    h.chunkSize = 36 + h.dataSize;
    file.write((char*)&h, sizeof(h));
    file.close();
    std::cout<<"AudioCapture::run() - stopped recording"<<std::endl;
    
}