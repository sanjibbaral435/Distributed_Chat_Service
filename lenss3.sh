#!/bin/bash
#lenss-comp1 128.194.143.156
#128.194.143.215 lenss-comp3
#128.194.143.213 lenss-comp4
sleep 5
x-terminal-emulator -e ./fbsd -p 128.194.143.215:7011 -r 7
x-terminal-emulator -e ./fbsd -p 128.194.143.215:7012 -r 8
x-terminal-emulator -e ./fbsd -p 128.194.143.215:7013 -r 9
