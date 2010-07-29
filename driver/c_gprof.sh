#!/bin/bash
# change those variables as you prefer
PROG=$1
PROF="gmon.out"
TYPE=svg
OUT=profiled.$TYPE

if uname | grep -i 'darwin'
then
    OPEN="open"
    GPROF2DOT="gprof2dot.py"
elif uname | grep -i 'linux'
then
    OPEN="evince"
    GPROF2DOT="gprof2dot"
fi

# check if gprof2dot is there

set -x

./$@
gprof --static-call-graph -m 0 $PROG | $GPROF2DOT -e0.01 -n0.01 -c bw | dot -T$TYPE -o $OUT
$OPEN $OUT
