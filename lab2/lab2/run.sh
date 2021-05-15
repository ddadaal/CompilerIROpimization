#!/usr/bin/env bash

# A script that invokes your compiler.

SH_DIR=$(dirname "$BASH_SOURCE")
# $SH_DIR/lab2 -opt=scp,dse -backend=rep < ../examples/gcd.3addr
$SH_DIR/lab2 $@