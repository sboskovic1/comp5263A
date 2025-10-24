#!/bin/bash
clear
gcc *.c ./yacsim.o -lm -o main
./main --evictPolicy PLRU --numIterations 128 --trace 0  --cachesizeBits 10 --waysBits 0