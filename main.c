#include <SDL2/SDL.h>
#include <stdio.h>

#include "theoraplay.h"

#define NAME "Jewels"
#define WIDTH 480
#define HEIGHT 272

static Uint32 baseticks = 0;

int closed = 0;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

typedef struct AudioQueue {
    const THEORAPLAY_AudioPacket *audio;
    int offset;
    struct AudioQueue *next;
} AudioQueue;

static volatile AudioQueue *audio_queue = NULL;
static volatile AudioQueue *audio_queue_tail = NULL;

static void SDLCALL audio_callback(void *userdata, Uint8 *stream, int len) {
    // !!! FIXME: this should refuse to play if item->playms is in the future.
    //const Uint32 now = SDL_GetTicks() - baseticks;
    Sint16 *dst = (Sint16 *)stream;

    while (audio_queue && (len > 0)) {
        volatile AudioQueue *item = audio_queue;
        AudioQueue *next = item->next;
        const int channels = item->audio->channels;

        const float *src = item->audio->samples + (item->offset * channels);
        int cpy = (item->audio->frames - item->offset) * channels;
        int i;

        if (cpy > (len / sizeof(Sint16)))
            cpy = len / sizeof(Sint16);

        for (i = 0; i < cpy; i++) {
            const float val = *(src++);
            if (val < -1.0f)
                *(dst++) = -32768;
            else if (val > 1.0f)
                *(dst++) = 32767;
            else
                *(dst++) = (Sint16)(val * 32767.0f);
        }

        item->offset += (cpy / channels);
        len -= cpy * sizeof(Sint16);

        if (item->offset >= item->audio->frames) {
            THEORAPLAY_freeAudio(item->audio);
            SDL_free((void *)item);
            audio_queue = next;
        }
    }

    if (!audio_queue)
        audio_queue_tail = NULL;

    if (len > 0)
        memset(dst, '\0', len);
}

static void queue_audio(const THEORAPLAY_AudioPacket *audio) {
    AudioQueue *item = (AudioQueue *)SDL_malloc(sizeof(AudioQueue));
    if (!item) {
        THEORAPLAY_freeAudio(audio);
        return;  // oh well.
    }

    item->audio = audio;
    item->offset = 0;
    item->next = NULL;

    SDL_LockAudio();
    if (audio_queue_tail)
        audio_queue_tail->next = item;
    else
        audio_queue = item;
    audio_queue_tail = item;
    SDL_UnlockAudio();
}

int setupSdl() {
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS);

    window = SDL_CreateWindow(
        NAME,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WIDTH,
        HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initiation window. Error %s", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initialize renderer. Error: %s", SDL_GetError());
        return 1;
    }

    return 0;
}

void handleEvents(){
    SDL_Event event;
    if (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                closed = 1;
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    THEORAPLAY_Decoder *decoder = NULL;
    const THEORAPLAY_VideoFrame *video = NULL;
    const THEORAPLAY_AudioPacket *audio = NULL;
    SDL_AudioSpec spec;

    printf("Starting game\n");
    if (setupSdl() != 0) {
        return 1;
    }

    // load video
    decoder = THEORAPLAY_startDecodeFile("video.ogv", 30, THEORAPLAY_VIDFMT_IYUV);
    if (!decoder) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to start decoding!");
        return 1;
    }

    // wait until we have video and/or audio data, so we can set up hardware.
    while (!audio || !video) {
        if (!audio) audio = THEORAPLAY_getAudio(decoder);
        if (!video) video = THEORAPLAY_getVideo(decoder);
        SDL_Delay(10);
    }

    Uint32 framems = (video->fps == 0.0) ? 0 : ((Uint32)(1000.0 / video->fps));

    SDL_Texture* texture = NULL;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, video->width, video->height);
    if (texture == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture for video! SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    memset(&spec, '\0', sizeof(SDL_AudioSpec));
    spec.freq = audio->freq;
    spec.format = AUDIO_S16SYS;
    spec.channels = audio->channels;
    spec.samples = 2048;
    spec.callback = audio_callback;
    if (SDL_OpenAudio(&spec, NULL) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to open audio. Error: %s", SDL_GetError());
        return 1;
    }

    void *pixels = NULL;
    int pitch = 0;

    while (audio) {
        queue_audio(audio);
        audio = THEORAPLAY_getAudio(decoder);
    }

    baseticks = SDL_GetTicks();

    SDL_PauseAudio(0);

    while (!closed && THEORAPLAY_isDecoding(decoder)) {
        const Uint32 now = SDL_GetTicks() - baseticks;

        if (!video)
            video = THEORAPLAY_getVideo(decoder);

        // Play video frames when it's time.
        if (video && (video->playms <= now)) {
            //printf("Play video frame (%u ms)!\n", video->playms);
            if (framems && ((now - video->playms) >= framems)) {
                // Skip frames to catch up, but keep track of the last one
                //  in case we catch up to a series of dupe frames, which
                //  means we'd have to draw that final frame and then wait for
                //  more.
                const THEORAPLAY_VideoFrame *last = video;
                while ((video = THEORAPLAY_getVideo(decoder)) != NULL) {
                    THEORAPLAY_freeVideo(last);
                    last = video;
                    if ((now - video->playms) < framems)
                        break;
                }

                if (!video)
                    video = last;
            }

            SDL_LockTexture(texture, NULL, &pixels, &pitch);
            const int w = video->width;
            const int h = video->height;
            const Uint8 *y = (const Uint8 *)video->pixels;
            const Uint8 *u = y + (w * h);
            const Uint8 *v = u + ((w / 2) * (h / 2));
            Uint8 *dst = (Uint8 *)pixels;
            int i;

            // memcpy(pixels, video->pixels, video->height * pitch); // For RGBA texture

            for (i = 0; i < h; i++, y += w, dst += pitch) memcpy(dst, y, w);
            for (i = 0; i < h / 2; i++, u += w / 2, dst += pitch / 2) memcpy(dst, u, w / 2);
            for (i = 0; i < h / 2; i++, v += w / 2, dst += pitch / 2) memcpy(dst, v, w / 2);

            SDL_UnlockTexture(texture);

            THEORAPLAY_freeVideo(video);
            video = NULL;
        } else {
            // no new video frame? Give up some CPU
            SDL_Delay(10);
        }

        while ((audio = THEORAPLAY_getAudio(decoder)) != NULL)
            queue_audio(audio);

        handleEvents();
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        int audio_playing = 1;
        // Drain out the audio queue.
        while (!audio_playing) {
            SDL_LockAudio();
            audio_playing = (audio_queue == NULL);
            SDL_UnlockAudio();
            if (!audio_playing)
                SDL_Delay(100);  // wait for final audio packets to play out.
        }
    }

    if (THEORAPLAY_decodingError(decoder)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error decoding file");
        return 1;
    }

    printf("And that was it\n");
    if (texture) SDL_DestroyTexture(texture);
    if (video) THEORAPLAY_freeVideo(video);
    if (audio) THEORAPLAY_freeAudio(audio);
    if (decoder) THEORAPLAY_stopDecode(decoder);
    SDL_CloseAudio();
    SDL_Quit();

    return 0;
}
