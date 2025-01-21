#!/bin/sh

set -xe

./build_static.sh
gcc -O3 -std=c99 -o target/test test.c target/libcli.a
