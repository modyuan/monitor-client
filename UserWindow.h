//
// Created by yuansu on 2019-04-09.
//

#ifndef FFMPEG_TEST_USERWINDOW_H
#define FFMPEG_TEST_USERWINDOW_H

#include <string>
#include <atomic>

extern "C"{
#define SDL_MAIN_HANDLE
#define __STDC_CONSTANT_MACROS
#include "SDL2/SDL.h"
};


using namespace std;

class UserWindow {
private:
    SDL_Window *screen;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* sdlTexture;
    SDL_Rect sdlRect;
    atomic_bool keep;
public:

    UserWindow():keep(false){}
    void CreateWindow(int width,int height);
    void EventLoop();
    void Refresh(void* pix, int pitch);
    void SendQuit();

};


#endif //FFMPEG_TEST_USERWINDOW_H
