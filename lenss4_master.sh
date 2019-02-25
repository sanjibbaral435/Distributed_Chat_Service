#!/bin/bash
x-terminal-emulator -e ./master -p 5254 -r 4
x-terminal-emulator -e ./master -p 5255 -r 5
x-terminal-emulator -e ./master -p 5256 -r 6
sleep 15
x-terminal-emulator -e ./fbsd -p 128.194.143.213:7001 -r 5
x-terminal-emulator -e ./fbsd -p 128.194.143.213:7002 -r 6

#128.194.143.156 lenss-comp1
#128.194.143.215 lenss-comp3
#128.194.143.213 lenss-comp4
