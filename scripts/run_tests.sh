#!/bin/sh

for TEST_CONFIG in $(find -name test.conf); do
    echo "running $TEST_CONFIG"
    phantom run $TEST_CONFIG 2>&1 #| grep 'Assertion failed'
done
