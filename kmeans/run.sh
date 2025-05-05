#!/bin/bash

export -f compile_gtk4
source ~/.bashrc
# Compile the file
compile_gtk4 kmeans.c

./kmeans data.txt

