#include <iostream>
#include <SDL.h>
#include <stdio.h>
#include <SDL2/SDL_ttf.h>
#include <vector>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

void blinkCursor(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int textX, int textY) {
  Uint32 currentTime = SDL_GetTicks();
  static Uint32 lastToggleTime = 0;
  static bool cursorVisible = true;
  const Uint32 BLINK_INTERVAL = 500;

  if (currentTime - lastToggleTime >= BLINK_INTERVAL) {
    cursorVisible = !cursorVisible;
    lastToggleTime = currentTime;
  }

  if (cursorVisible) {
    int textWidth = 0;
    int textHeight = 0;
    if (TTF_SizeUTF8(font, text.c_str(), &textWidth, &textHeight) != 0) {
      textWidth = 0;
      textHeight = TTF_FontHeight(font);
      
    } else {
      textHeight = TTF_FontHeight(font);
    }
    SDL_Rect cursorRect = { textX + textWidth + 2, textY, 3, textHeight };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &cursorRect);
  }
}

int main(int argc, char* argv[]) {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }

  if (TTF_Init() == -1) {
    std::cerr << "TTF Init Error: " << TTF_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  // Create window
  SDL_Window* window = SDL_CreateWindow("Text Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  // Create renderer
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  TTF_Font* font = TTF_OpenFont("Fonts/PressStart2P-Regular.ttf", 24);
  if (!font) {
    std::cerr << "Font Load Error: " << TTF_GetError() << std::endl;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    return 1;
  }

  int running = 1;
  SDL_Event event;
  std::string inputText;
  std::vector<std::string> totalText {inputText};
  int textBoxX = 2;
  int textBoxY = 2;
  int lineNum = 1;
  int charWidth = SCREEN_WIDTH / 24;

  SDL_StartTextInput(); // Enable OS text input events

  // Game/Application loop
  while (running) {
    // Handle events on queue
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        running = 0;
      }

      if (event.type == SDL_TEXTINPUT) {
        inputText += event.text.text;
        if (inputText.size() >= charWidth) {
          totalText.push_back(inputText);
          inputText = "";
          textBoxY += 25;
          lineNum++;

        }
      }

      // Handle Enter key
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN) {
          totalText.push_back(inputText);
          inputText = "";
          textBoxY += 25;
          lineNum++;
        }
      }



      // Handle BackSpace
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_BACKSPACE && !inputText.empty()) {
          inputText.pop_back();
        } else if (event.key.keysym.sym == SDLK_BACKSPACE && inputText.empty() && textBoxY > 2) {
          textBoxY -= 25;
          inputText = totalText[lineNum - 1];
          totalText.pop_back();
          lineNum--;
        }
      }
    }

    // --- Rendering Phase --- 
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);


      SDL_Color textColor = { 255, 255, 255, 255 }; // White text
      
      // Starting dimensions for text
      int startX = 10;
      int startY = -TTF_FontHeight(font);
      int lineSpace = TTF_FontHeight(font);

      for (size_t i = 0; i < totalText.size(); i++) {
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, totalText[i].c_str(), textColor);
        if (!textSurface) continue; 

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);


        SDL_Rect lineSpaceRECT = {startX, startY + static_cast<int>(i * lineSpace), textSurface->w, textSurface->h};

        SDL_RenderCopy(renderer, textTexture, NULL, &lineSpaceRECT);

        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);

      }


      if (!inputText.empty()) {
        SDL_Surface* inputSurface = TTF_RenderText_Blended(font, inputText.c_str(), textColor);
        SDL_Texture* inputTexture = SDL_CreateTextureFromSurface(renderer, inputSurface);
        if (!inputTexture) continue; 
        int inputY = startY + static_cast<int>(totalText.size() * lineSpace);
        SDL_Rect inputRECT = {startX, inputY, inputSurface->w, inputSurface->h};
        SDL_RenderCopy(renderer, inputTexture, NULL, &inputRECT);
        SDL_FreeSurface(inputSurface);
        SDL_DestroyTexture(inputTexture);
      }

    // Draw Cursor
    blinkCursor(renderer, font, inputText, textBoxX, textBoxY);

    SDL_RenderPresent(renderer);
  } // <-- Game loop ends here

  // Destroy resources and quit
  SDL_StopTextInput();
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();

  return 0;
}

