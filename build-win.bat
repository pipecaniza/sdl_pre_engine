mkdir "build/win"
gcc main.c -IC:\sdl2\include\SDL2 -LC:\sdl2\lib -g -ggdb -lmingw32 -lSDL2main -lSDL2 -o build/win/main
gcc test_engine.c -shared -g -ggdb -fPIC -w -o build/win/engine.dll