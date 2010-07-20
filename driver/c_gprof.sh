#!/bin/bash
# change those variables as you prefer
PROG=$1
PROF="gmon.out"
STATS=$1.stats
TYPE=pdf
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
gprof $PROG | $GPROF2DOT | dot -T$TYPE -o $OUT
$OPEN $OUT