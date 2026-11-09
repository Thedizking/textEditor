# textEditor
Small simple Text editor made using C++ and SDL

Currently only runs on linux

All Code is listed in branch lol

Current build is functionally complete however a few more features are intended to be added.  
i.e. Code Coloring, Copy Paste.

Build instructions below.

## Required Dependencies

- SDL2 

- SDL_TTF

### Compilation command 

```

g++ main.cpp -o textEditor `pkg-config --cflags --libs sdl2 SDL2_ttf` 

```

#### Usage

```

./textEditor <filename>

```


To save the File press "CTRL + S" 

To exit without saving press "ESCAPE"
