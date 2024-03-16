#!/bin/bash
mkdir -p build/linux
gcc main.c -g -ggdb -w -lm -lSDL2 -o build/linux/main
gcc test_engine.c -shared -g -ggdb -fPIC -w -o build/linux/engine.so