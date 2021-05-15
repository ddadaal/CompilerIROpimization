#!/usr/bin/env bash

# A script that invokes your compiler.

SH_DIR=$(dirname "$BASH_SOURCE")
# $SH_DIR/lab3 -opt=scp -backend=c < ../examples/gcd.3addr
$SH_DIR/lab3 $@