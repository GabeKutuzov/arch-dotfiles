#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
int main() {

   if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
      printf("error initializing SDL: %s\n", SDL_GetError());
      return 1;
   }

   SDL_Window* win = SDL_CreateWindow("Title", 
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      640, 480, 0);
   if(!win) {
      printf("error creating window %s\n", SDL_GetError());
      SDL_Quit();
      return 1;
   }

   SDL_Delay(5000);

   SDL_DestroyWindow(win);
   SDL_Quit();

   return 0;
}
