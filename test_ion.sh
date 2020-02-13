#!/bin/bash

SELF=$(realpath $(dirname $0))
TEST_PROG=mpp_buffer_test
TEST_PATH=/data/local/tmp

case $1 in
  android)
    adb push $SELF/build-x86_64/mpp/base/test/$TEST_PROG $TEST_PATH
    adb push $SELF/build-x86_64/mpp/libmpp.so $TEST_PATH
    # clear old logs
    adb logcat -c
    adb shell "LD_LIBRARY_PATH=$TEST_PATH $TEST_PATH/$TEST_PROG"
    # dump mpp logs and quit
    adb logcat -d | grep mpp
    ;;
  *)
    sudo chmod a+rw /dev/ion
    $SELF/build-linux/mpp/base/test/$TEST_PROG
    ;;
esac

echo ""
echo "--- done ---"
