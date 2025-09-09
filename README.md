# TUIcGame
WIP programming game that I'm working on, inspired by [BitBurner](https://github.com/bitburner-official/bitburner-src) but much worse because it uses C and has no sandboxing, just a few restrictions in the code. Currently there is no game (not a reference, simply true), just a small internal command-line, integrated C compiler, kind of broken chat room, empty map window and all-important exit button.

Uses Ncurses for the display, using a slightly modified version of a "window display" I've made before, I need to make it more generic I know. Pretty much everything is in C++, older things I've made before starting this project are in C.

The server IP is currently hard-coded to be 127.0.0.1 and uses port 12345. Said server is for the chat channel, which has many issues and does not have any kind of authentication. Named IRC in-game and in the code, even though it is *NOT* actually an IRC channel, client or server. This might be removed in a later version, I don't know if I like it.

# Compiling:
Currently *only* compiles and runs on Linux and I do not reccomend that you do anyway, since this is not meant to be more than a personal project.

Run compile.sh, it sets up ninja, uses it to compile the main executable then compiles the server executable with g++.

Both executables are placed in the root folder (same directory as the README in this repo). Both have the .x86_64 extention, cGame.x86_64 is the main game, server.x86_64 is the server.

## Requires:
- cmake
- ninja (but could easily be changed to not require it)
- g++
- gcc
- glibc
- Ncurses
- tcc (Tiny C Compiler)
- Optionally tree (used to show user directory on game startup, purely 'cosmetic')

## Controls and small tutorial if you for some reason get this compiled and working for you:
There is no built-in text editor to create and edit a c file for the game, go into save -> username (inputted in-game) -> root -> wherever you want to put the file.

Use "help" in the cmd for a list of commands

Tab: cycle windows

Arrow keys up and down: cycle through options in the sidebar

Most of the rest of the keyboard: typing