#include <iostream>
#include "audio.h"
#include <SDL.h>
#include <math.h>

const int sample_rate = 44100;
const int buffer_size = 4096;

typedef struct {
    float current_step;
    float step_size;
    float volume;
} oscillator;

oscillator oscillate(float rate, float volume) {
    oscillator o = {
        .current_step = 0,
        .step_size = static_cast<float>((2.0 * M_PI) / rate),
        .volume = volume,
    };
    return o;
}

float nextee(oscillator *os) {
    os->current_step += os->step_size;
    return sinf(os->current_step) * os->volume;
}

float A4_freq = (float)sample_rate / 261.6f;
oscillator a4 = oscillate(A4_freq, 0.8f);



void callback(void *userdata, Uint8 *stream, int len) {
    float *fstream = (float *)stream;    
    for (int i = 0; i < buffer_size; i++) {
        float v = nextee(&a4);
        fstream[i] = v;
    }
}

int audio_main() {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        printf("hell nah");
        return 1;
    }
    SDL_AudioSpec spec = {
        .freq = sample_rate,
        .format = AUDIO_F32,
        .channels = 1,
        .samples = buffer_size,
        .callback = callback,
    };

    if (SDL_OpenAudio(&spec, NULL) < 0) {
        printf("failed to open audio device");
        return 1;
    }

    SDL_PauseAudio(0);
    while (true) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                return 0;
            }
        }
    }
    return 0;
 
}