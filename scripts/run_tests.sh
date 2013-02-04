#!/bin/sh

for TEST_CONFIG in $(find -name test.conf); do
    phantom run $TEST_CONFIG 2>&1
done
