#!/bin/sh

set -xe

mkdir -p target
gcc -O3 -nostdlib -shared -o target/libcli.so cli.c
