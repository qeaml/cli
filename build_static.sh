#!/bin/sh

set -xe

mkdir -p target
gcc -O3 -nostdlib -std=c99 -c -o target/cli.o cli.c
ar rcs target/libcli.a target/cli.o
