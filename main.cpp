#include <iostream>
#include <SDL.h>
#include <stdio.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <fstream>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 960

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

  // Initialize TTF
  if (TTF_Init() == -1) {
    std::cerr << "TTF Init Error: " << TTF_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  // Create window
  SDL_Window* window = SDL_CreateWindow("Text Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN || SDL_WINDOW_RESIZABLE);
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

  // Set Font
  TTF_Font* font = TTF_OpenFont("Fonts/SourceCodePro-VariableFont_wght.ttf", 32);
  if (!font) {
    std::cerr << "Font Load Error: " << TTF_GetError() << std::endl;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    return 1;
  }

  std::ofstream tmpFile("tmpFile.txt");


  if (!tmpFile.is_open()) {
    std::cerr << "Error opening temp file!" << std::endl;
    return 1;
  }

  bool running = true;
  SDL_Event event;
  std::string inputText;
  std::vector<std::string> totalText {inputText};
  int textBoxX = 2;
  int textBoxY = 2;
  int lineNum = 1;
  int charWidth = SCREEN_WIDTH / 24;
  int charHeight = TTF_FontHeight(font);
  bool isSaving = false;
  std::string saveText = "Please enter file name followed by the enter key.";

  SDL_StartTextInput(); // Enable OS text input events

  // Application loop
  while (running) {
    // Handle events on queue
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        for (std::string text : totalText) {
          tmpFile << text << "\n"; 
        }
        if (!inputText.empty()) {
          tmpFile << inputText << "\n";
        }

        tmpFile.close();

        running = false;
      }

      // Add input text to string 
      if (event.type == SDL_TEXTINPUT) {
        inputText += event.text.text;
        if (inputText.size() >= charWidth) {
          totalText.push_back(inputText);
          inputText = "";
          textBoxY += charHeight;
          lineNum++;

        }
      }

      // Handle Enter key
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN) {
          totalText.push_back(inputText);
          inputText = "";
          textBoxY += charHeight;
          lineNum++;
        }
      }



      
      // Handle Backspace.
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_BACKSPACE && !inputText.empty()) {
          inputText.pop_back();
        } else if (event.key.keysym.sym == SDLK_BACKSPACE && inputText.empty() && textBoxY > 2) {
          textBoxY -= charHeight;
          inputText = totalText[lineNum - 1];
          totalText.pop_back();
          lineNum--;
        }
      }
    }


    // Handle Save Button CTRL/S NOT WORKING DEFAULT TO SAVE ON QUIT
    if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.mod & KMOD_CTRL) {
        if (event.key.keysym.sym == SDLK_s) {
          totalText.push_back(inputText);
          inputText = "";
          textBoxY += charHeight;
          lineNum++;
          isSaving = true;
          std::cout << "Saved!";
        }
      }
    }


    // --- Rendering Phase --- 
    SDL_SetRenderDrawColor(renderer, 5, 0, 15, 255);
    

    // --- Rendering Phase --- (FIXED: Now safely inside the game loop)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);


      SDL_Color textColor = { 225, 200, 255, 255 };
      
      // Starting dimensions for text
      int startX = 10;
      int startY = -TTF_FontHeight(font) + 1;
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


      if (isSaving) {
        SDL_Surface* saveSurface = TTF_RenderText_Blended(font, saveText.c_str(), textColor);
        SDL_Texture* saveTexture = SDL_CreateTextureFromSurface(renderer, saveSurface);
        if (!saveTexture) continue; 
        int inputY = startY + static_cast<int>(totalText.size() * lineSpace);
        SDL_Rect saveRECT = {startX, inputY, saveSurface->w, saveSurface->h};
        SDL_RenderCopy(renderer, saveTexture, NULL, &saveRECT);
        SDL_FreeSurface(saveSurface);
        SDL_DestroyTexture(saveTexture);
        
      }

    // Draw the dynamic cursor at the end of the text
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

