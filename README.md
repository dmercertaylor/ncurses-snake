# ncurses-snake

A simple snake game for your console, written in C using ncurses. To compile, just run `cc -o snake snake.c -lncurses` (some systems may use `-lcurses` instead). I've only tested this in MacOS, but see no reason why it wouldn't work in any other Unix-y system.

The game takes two arguments at launch: screen width and height, e.g. `./snake 12 16`. If neither are specified, the game defaults to 16 x 16. If only one argument is given, such as `./snake 12`, that one argument will be used for both width and height.
