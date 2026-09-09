#!/bin/bash

START_DIR=$PWD
LIBDAISY_DIR=$PWD/lib/libDaisy
DAISYSP_DIR=$PWD/lib/DaisySP

echo "building libDaisy . . ."
cd "$LIBDAISY_DIR" ; make -s clean ; make -j -s
if [ $? -ne 0 ]
then
    echo "Failed to compile libDaisy"
    exit 1
fi
echo "done."

echo "building DaisySP . . ."
# DaisySP compiles with -Werror, and newer GCC versions (16.2 at the time of
# writing) flag two unused variables in Source/PhysicalModeling/drip.cpp.
# Keep that warning non-fatal. OPT is DaisySP's optimisation level (-O3 in
# its Makefiles); overriding it is the only way to add a flag without
# replacing the rest of its CFLAGS.
cd "$DAISYSP_DIR" ; make -s clean ; make -j -s OPT='-O3 -Wno-error=unused-but-set-variable'
if [ $? -ne 0 ]
then
    echo "Failed to compile DaisySP"
    exit 1
fi
echo "done."

