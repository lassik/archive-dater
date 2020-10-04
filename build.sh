#!/bin/sh
set -eu
cd "$(dirname "$0")"
echo "Entering directory '$PWD'"
set -x
cc \
    -Wall -Wextra -pedantic -std=gnu99 -fsanitize=address -Og -g \
    -o archive-dater archive-dater.c \
    $(pkg-config --libs --cflags libarchive)
