#!/usr/bin/env bash

# Script to run your translator.

SH_DIR=$(dirname "$BASH_SOURCE")

# it prints out a header file named {timestamp}.h at the cwd
$SH_DIR/translator 2> $(date +"%s").h