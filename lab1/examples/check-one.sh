#!/usr/bin/env bash

C_SUBSET_COMPILER=../src/csc
#TODO: set THREE_ADDR_TO_C_TRANSLATOR to your own converter
THREE_ADDR_TO_C_TRANSLATOR=../lab1/run.sh

[ $# -ne 1 ] && { echo "Usage $0 PROGRAM" >&2; exit 1; }

# set -v

PROGRAM=$1
BASENAME=`basename $PROGRAM .c`
echo $PROGRAM
${C_SUBSET_COMPILER} $PROGRAM > ${BASENAME}.3addr
gcc $PROGRAM -o ${BASENAME}.gcc.bin
${THREE_ADDR_TO_C_TRANSLATOR} < ${BASENAME}.3addr > ${BASENAME}.3addr.c
gcc ${BASENAME}.3addr.c -o ${BASENAME}.3addr.bin
./${BASENAME}.gcc.bin > ${BASENAME}.gcc.txt
./${BASENAME}.3addr.bin > ${BASENAME}.3addr.txt
md5sum ${BASENAME}.gcc.txt ${BASENAME}.3addr.txt
