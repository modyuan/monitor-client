#include "UserWindow.h"


void UserWindow::CreateWindow(int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
    }
    screen = SDL_CreateWindow("Video Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              width, height,
                              SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if (!screen) {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return;
    }
    //if(fullScreen) SDL_SetWindowFullscreen(screen,SDL_WINDOW_FULLSCREEN);
    sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    sdlRect.x = 0;
    sdlRect.y = 0;
    sdlRect.w = width;
    sdlRect.h = height;

}

void UserWindow::Refresh(void *pix, int pitch) {

    if(!keep) return;
    SDL_UpdateTexture(sdlTexture, NULL, pix, pitch);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);

}

void UserWindow::EventLoop() {
    keep = true;
    SDL_Event event;
    while (keep) {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    event.type = SDL_QUIT;
                    SDL_PushEvent(&event);
                }
            case SDL_QUIT:
                keep = false;
                SDL_DestroyWindow(screen);
                SDL_Quit();
                break;
        }
    }

}

void UserWindow::SendQuit() {
    SDL_Event e;
    e.type = SDL_QUIT;
    SDL_PushEvent(&e);
}
