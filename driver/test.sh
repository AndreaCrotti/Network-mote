#!/bin/bash
set -x
for x in $(grep -l STANDALONE $x *.c)
do 
    TEST_FILE=tests/test_${x%.c}
    $@ -o $TEST_FILE $x
done

TESTS="true"
for t in tests/test_*
do
    TESTS="$TESTS && $t"
done

echo "running tests"
eval $TESTS