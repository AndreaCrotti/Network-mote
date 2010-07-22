#!/bin/bash
set -x
for x in $(grep -l STANDALONE $x *.c)
do 
    TEST_FILE=tests/test_${x%.c}
    $@ -o $TEST_FILE $x
done

for t in tests/test_*
do
    echo "runnning $t"
    ./$t
done