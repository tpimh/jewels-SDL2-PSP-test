#include <SDL2/SDL.h>
#include <stdio.h>

#include "theora.h"

#define NAME "Jewels"
#define WIDTH 480
#define HEIGHT 272

int closed = 0;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

/**
 * The decoder context. Must be global because SDL mixer callbacks know no better
 */
theora_t ctx = { 0 };

/**
 * The SDL mixer's callback, which, thanks to the wrapper is brainfuck simple.
 */
static void callback(int channel)
{
    Mix_Chunk *audio = theora_audio(&ctx);
    /* could we get some audio data? If so, play it! */
    if(audio)
        Mix_PlayChannel(channel, audio, 0);
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

    Mix_Init(0);
    Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096);

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
    FILE *f;
    SDL_Texture *texture = NULL;
    SDL_Rect rect;                  /* the rectangle where we want to display the video */

    /* open the video file */
    f = fopen("video.ogv", "rb");
    if(!f) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to open file");
        return 1;
    }

    printf("Starting game\n");
    if (setupSdl() != 0) {
        return 1;
    }

    /************************************************************************/
    /***                       start the decoder                          ***/
    /************************************************************************/
    /* yeah, really this simple. */
    theora_start(&ctx, f);

    /* the decoder was kind enough to tell us the video dimensions in advance */
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, ctx.w, ctx.h);
    if (texture == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture for video! SDL Error: %s\n", SDL_GetError());
        return 1;
    }
    /* the video should be in the center of the screen */
    rect.w = ctx.w;
    rect.h = ctx.h;
    rect.x = (WIDTH - rect.w) / 2;
    rect.y = (HEIGHT - rect.h) / 2;

    /************************************************************************/
    /***                  get the duration in millisec                    ***/
    /************************************************************************/
    printf("Duration: %lu msec\n", theora_getduration(f));

    /************************************************************************/
    /***                          audio player                            ***/
    /************************************************************************/

    /* this is going to be extremely simple, because we're using SDL mixer to play it in the background */
    Mix_ChannelFinished(callback);
    callback(0);

    /************************************************************************/
    /***                          video player                            ***/
    /************************************************************************/

    printf("Playing started...\n");
    /* in this simple example, we quit when we are finished playing, but you don't have to */
    while (theora_playing(&ctx) && !closed) {

        /* is there a frame to play? If so, refresh the texture */
        theora_video(&ctx, texture);

        handleEvents();

        /* render as usual. The video is just another texture to add. You could resize and position it if you want */
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_RenderPresent(renderer);
    }
    printf("Finished playing\n");

    /************************************************************************/
    /***                        stop the decoder                          ***/
    /************************************************************************/

    /* the theora thread has absolutely no clue about the SDL mixer usings its buffers, so we MUST stop the mixer first. */
    if(Mix_Playing(-1)) {
        Mix_ChannelFinished(NULL);
        Mix_HaltChannel(-1);
    }

    /* ok, now that nobody is relying on the audio buffer, we can stop the decoder and destroy the context */
    theora_stop(&ctx);

    /* as usual, free SDL resources */
    printf("Tear down\n");
    fclose(f);
    if(texture) SDL_DestroyTexture(texture);
    if(renderer) SDL_DestroyRenderer(renderer);
    if(window) SDL_DestroyWindow(window);
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();

    return 0;
}
