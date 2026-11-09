#include <iostream>
#include <SDL.h>
#include <stdio.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <fstream>
#include <cctype> // For std::tolower()
#include <algorithm> // Delete after refactoring save input.

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 960

void blinkCursor(SDL_Renderer* renderer, TTF_Font* font, const std::string& currentLineText, int startX, int startY, int cursorLine, int cursorCol, int lineSpacing) {
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
    int textHeight = TTF_FontHeight(font);
    
    // Measure string up to the current column position
    if (!currentLineText.empty() && cursorCol > 0) {
      std::string substring = currentLineText.substr(0, cursorCol);
      TTF_SizeUTF8(font, substring.c_str(), &textWidth, nullptr);
    }

    int cursorY = startY + (cursorLine * (textHeight + lineSpacing));
    int cursorX = startX + textWidth;

    SDL_Rect cursorRect = { cursorX, cursorY, 3, textHeight };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &cursorRect);
  }
}


bool loadFile(const std::string& filename, std::vector<std::string>& totalText) {
  std::ifstream file(filename);
  if (!file) {
    std::ofstream file(filename);
    return true;
  }

  totalText.clear();
  std::string text;

  while (std::getline(file, text)) {
    totalText.push_back(text);
  }
  return true;
}

bool saveFile(const std::string& filename, std::vector<std::string>& totalText) {
  std::ofstream outputFile(filename);
  if (!outputFile) {
    std::cerr << "Could not open file" << filename << "\n";
    return false;
  }

  std::string text;

  for (const std::string& text : totalText) {
    outputFile << text << "\n";
  }
  outputFile.close();
  std::cout << "Save Succeded" << std::endl;
  return true;
}

int scrollLine = 0;
const int VISIBLE_LINES = 24;
const int scrollPadding = 5;

void updateScroll(int cursorLine, int cursorCol, int& scrollLine, const std::vector<std::string>& totalText, int visibleLines) {
    // --- VERTICAL SCROLLING ---
    if (cursorLine < scrollLine) {
        scrollLine = cursorLine;
    }
    else if (cursorLine >= scrollLine + visibleLines) {
        scrollLine = cursorLine - visibleLines + 1;
    }

}

int main(int argc, char* argv[]) {

  if (argc < 2) { // Makes sure at RunTime both ./textEditor and a file name are declared
    std::cerr << "Error: Must include filename\n" <<"Run The following:\n" << argv[0] << " <filename>\n";
    return 1;
  }

  std::vector<std::string> totalText; // Main vector to store our text

  if (!loadFile(argv[1], totalText)) {
    return 1;
  }

  if (totalText.empty()) {
    totalText.push_back("");
  }

  // Track cursor coordinates globally inside the file matrix
  int cursorLine = static_cast<int>(totalText.size()) - 1; 
  int cursorCol = static_cast<int>(totalText[cursorLine].size());

  // Create an isolated char buffer just for the Save overlay screen
  std::string saveInputBuffer = ""; 


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
  SDL_Window* window = SDL_CreateWindow("Text Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);
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

  std::string fileName = argv[1];
  bool running = true;
  SDL_Event event;
  int textBoxX = 11;
  int textBoxY = 2;
  int lineNum = 1;
  int charWidth = 13;
  int charHeight = TTF_FontHeight(font);
  bool isSaving = false;
  std::string saveText = "File Saved! Would you like to quit? Y or N";

  SDL_StartTextInput(); // Enable OS text input events

  // Application loop
  while (running) {
    // Handle events on queue
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        running = false;
        return 0;
      }


      if (isSaving) {
        if (event.type == SDL_TEXTINPUT) {
          saveInputBuffer += event.text.text;
        } else if (event.type == SDL_KEYDOWN) {
          if (event.key.keysym.sym == SDLK_RETURN) {
            std::transform(saveInputBuffer.begin(), saveInputBuffer.end(), saveInputBuffer.begin(), [](unsigned char c) {
                return std::tolower(c);
                });
            if (saveInputBuffer == "y") {
              running = false;
            } else {
              isSaving = false;
              saveInputBuffer = "";
            }
          } else if (event.key.keysym.sym == SDLK_BACKSPACE && !saveInputBuffer.empty()) {
            saveInputBuffer.pop_back();
          } else if (event.key.keysym.sym == SDLK_ESCAPE) {
            isSaving = false;
            saveInputBuffer = "";
          }
        }
      } else {
        // --- TEXT INPUT MODE ---
        if (event.type == SDL_TEXTINPUT) {
          totalText[cursorLine].insert(cursorCol, event.text.text);
          cursorCol += strlen(event.text.text);

          // Word wrap handling
          if (totalText[cursorLine].size() * charWidth >= SCREEN_WIDTH) {
            std::string remainder = totalText[cursorLine].substr(cursorCol);
            totalText[cursorLine] = totalText[cursorLine].substr(0, cursorCol);
            totalText.insert(totalText.begin() + cursorLine + 1, remainder);
            cursorLine++;
            cursorCol = 0;
            // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
          }
        }
        else if (event.type == SDL_KEYDOWN) {
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
            SDL_QUIT;
          }

          // Handle Enter Key (Splits a line mid-way)
          else if (event.key.keysym.sym == SDLK_RETURN) {
            std::string remainder = totalText[cursorLine].substr(cursorCol);
            totalText[cursorLine] = totalText[cursorLine].substr(0, cursorCol);
            totalText.insert(totalText.begin() + cursorLine + 1, remainder);
            cursorLine++;
            cursorCol = 0;
            // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
          }

          // Handle Backspace Key (Deletes character and merges lines)
          else if (event.key.keysym.sym == SDLK_BACKSPACE) {
            if (cursorCol > 0) {
              totalText[cursorLine].erase(cursorCol - 1, 1);
              cursorCol--;
              // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
            } else if (cursorLine > 0) {
              // Merge current line onto the end of the previous line
              cursorCol = static_cast<int>(totalText[cursorLine - 1].size());
              totalText[cursorLine - 1] += totalText[cursorLine];
              totalText.erase(totalText.begin() + cursorLine);
              cursorLine--;
              // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
            }
          }

          // ARROW KEY FUNCTIONALITY ---
          else if (event.key.keysym.sym == SDLK_UP) {
            if (cursorLine > 0) {
              cursorLine--;
              // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
              if (cursorCol > static_cast<int>(totalText[cursorLine].size())) {
                cursorCol = static_cast<int>(totalText[cursorLine].size());
              }
            }
          }
          else if (event.key.keysym.sym == SDLK_DOWN) {
            if (cursorLine < static_cast<int>(totalText.size()) - 1) {
              cursorLine++;
              // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
              if (cursorCol > static_cast<int>(totalText[cursorLine].size())) {
                cursorCol = static_cast<int>(totalText[cursorLine].size());
              }
            }
          }
          else if (event.key.keysym.sym == SDLK_LEFT) {
            if (cursorCol > 0) {
              cursorCol--;
              // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
            } else if (cursorLine > 0) {
              cursorLine--;
              // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
              cursorCol = static_cast<int>(totalText[cursorLine].size());
            }
          }
          else if (event.key.keysym.sym == SDLK_RIGHT) {
            if (cursorCol < static_cast<int>(totalText[cursorLine].size())) {
              cursorCol++;
              // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
            } else if (cursorLine < static_cast<int>(totalText.size()) - 1) {
              cursorLine++;
              // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
              cursorCol = 0;
            }
          }
          else if (event.key.keysym.sym == SDLK_TAB) {
            if (cursorLine >= 0 && cursorLine < static_cast<int>(totalText.size())) {

              // 1. Clamp cursorCol to the end of the line just in case it got out of sync
              int lineLength = static_cast<int>(totalText[cursorLine].size());
              if (cursorCol > lineLength) {
                cursorCol = lineLength;
              }

              // 2. Always insert the spaces at the cursor position
              totalText[cursorLine].insert(cursorCol, "  ");

              // 3. Move the cursor forward so it stays after the spaces
              cursorCol += 2;
              // Inside main(), right after handling a keypress event:
            updateScroll(cursorLine, cursorCol, scrollLine, totalText, VISIBLE_LINES);
            }
          }

          // Handle Control + S Shortcut Save State  
          if (event.key.keysym.mod & KMOD_CTRL) {
            if (event.key.keysym.sym == SDLK_s) {
              if (saveFile(argv[1], totalText)) {
                isSaving = true;
                saveInputBuffer = "";
              }
            }
          }
        }
      }

    }




      // --- Rendering Phase --- 
      SDL_SetRenderDrawColor(renderer, 5, 0, 15, 255);


      SDL_RenderClear(renderer);


      SDL_Color textColor = { 225, 200, 255, 255 };

      // Starting dimensions for text
      int startX = 20;
      int startY = 20;
      int lineSpace = TTF_FontHeight(font);

      // --- SCROLL COMPATIBLE RENDERING ---
      // Start loop from scrollLine and clamp lines rendered to your screen capacity
      int linesToRender = std::min(VISIBLE_LINES, static_cast<int>(totalText.size()) - scrollLine);

      for (int i = 0; i < linesToRender; i++) {
        int targetLineIndex = scrollLine + i; // Grab the correct array index offset

        SDL_Surface* textSurface = TTF_RenderText_Blended(font, totalText[targetLineIndex].c_str(), textColor);
        if (!textSurface) continue; 

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

        // Position on screen matches current visible viewport row 'i'
        SDL_Rect lineSpaceRECT = {startX, startY + static_cast<int>(i * lineSpace), textSurface->w, textSurface->h};

        SDL_RenderCopy(renderer, textTexture, NULL, &lineSpaceRECT);

        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
      }

      if (isSaving) {
        SDL_SetRenderDrawColor(renderer, 5, 0, 15, 255);
        SDL_RenderClear(renderer);
        SDL_Surface* saveSurface = TTF_RenderText_Blended(font, saveText.c_str(), textColor);
        SDL_Texture* saveTexture = SDL_CreateTextureFromSurface(renderer, saveSurface);
        if (saveTexture) {
          SDL_Rect saveRECT = {startX, startY + charHeight, saveSurface->w, saveSurface->h};
          SDL_RenderCopy(renderer, saveTexture, NULL, &saveRECT);
          SDL_FreeSurface(saveSurface);
          SDL_DestroyTexture(saveTexture);
        }
        if (!saveInputBuffer.empty()) {
          SDL_Surface* inputSurface = TTF_RenderText_Blended(font, saveInputBuffer.c_str(), textColor);
          SDL_Texture* inputTexture = SDL_CreateTextureFromSurface(renderer, inputSurface);
          if (inputTexture) {
            SDL_Rect inputRECT = {startX, startY + lineSpace, inputSurface->w, inputSurface->h};
            SDL_RenderCopy(renderer, inputTexture, NULL, &inputRECT);
            SDL_FreeSurface(inputSurface);
            SDL_DestroyTexture(inputTexture);
          }
        }
        // Flash save menu block line
        blinkCursor(renderer, font, saveInputBuffer, startX, startY + lineSpace, 0, static_cast<int>(saveInputBuffer.size()), 0);
      }

      if (!isSaving) {
        // Call your cursor blink routine using absolute rendering offsets
        blinkCursor(renderer, font, totalText[cursorLine], startX, startY, cursorLine - scrollLine, cursorCol, 0);
      }

      // Fix: Always push visual buffers to display hardware every loop cycle
      SDL_RenderPresent(renderer);

  }




    // Destroy resources and quit
    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}




