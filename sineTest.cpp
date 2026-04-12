#include <alsa/asoundlib.h>
#include <cmath>
#include <iostream>

int main() {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;

    const char  = "default";
    unsigned int sample_rate = 48000;
    int channels = 2;
    snd_pcm_uframes_t frames = 256;

    int err;

    // Open PCM device
    if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cerr << "Error opening device: " << snd_strerror(err) << std::endl;
        return 1;
    }

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);

    // Set parameters
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT_LE);
    snd_pcm_hw_params_set_channels(handle, params, channels);
    snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, 0);
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, 0);

    if ((err = snd_pcm_hw_params(handle, params)) < 0) {
        std::cerr << "Error setting HW params: " << snd_strerror(err) << std::endl;
        return 1;
    }

    // Buffer
    float *buffer = new float[frames * channels];

    double phase = 0.0;
    double freq = 440.0;
    double phase_inc = 2.0 * M_PI * freq / sample_rate;

    std::cout << "Playing sine wave... Ctrl+C to stop\n";

    while (true) {
        for (int i = 0; i < frames; ++i) {
            float sample = 0.2f * sin(phase); // cicho żeby nie zabić uszu 😉

            for (int ch = 0; ch < channels; ++ch) {
                buffer[i * channels + ch] = sample;
            }

            phase += phase_inc;
            if (phase >= 2.0 * M_PI)
                phase -= 2.0 * M_PI;
        }

        int rc = snd_pcm_writei(handle, buffer, frames);

        if (rc == -EPIPE) {
            // underrun
            std::cerr << "XRUN!\n";
            snd_pcm_prepare(handle);
        } else if (rc < 0) {
            std::cerr << "Error writing: " << snd_strerror(rc) << std::endl;
        }
    }

    snd_pcm_close(handle);
    delete[] buffer;

    return 0;
}