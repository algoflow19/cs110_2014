#!/bin/bash

. utils.sh

FARMER=""
TEST=0

while test $# -gt 0; do
    case "$1" in
        -h)
            echo "./test.sh - run tests"
            echo "./test.sh -h"
            echo "./test.sh [-t <test>] <farmer>"
            echo " "
            echo "options:"
            echo "  -h           print this help message"
            echo "  -t <test>    run only one of the five tests.  <test> must be in [1..5]."
            echo "  <farmer>     path to farmer"
            exit 0
            ;;
        -t)
            shift
            TEST=$1
            shift
            ;;
        *)
            FARMER=$1
            shift
            break
            ;;
    esac
done


if [ "$FARMER" == "" ]
then
    echo "Please provide a farmer program to run."
    exit
fi

run_farmer () {
    echo "running $FARMER for $1s..."
    $FARMER &
    FARMERPID=$!

    sleep $1

    ps -p $FARMERPID > /dev/null
    if [ $? -ne 0 ]
    then
        echo "farmer process has died...FAIL"
    fi

    stop_network

    echo "stopping $FARMER..."
    kill $FARMERPID &>/dev/null
}

result() {
    if [ $? -eq 0 ]
    then
        echo "SUCCESS"
    else
        echo "FAIL"
    fi
}

test_PING() {
    echo -n "Did master receive PING request?..."
    NMATCHES=`grep -c "received PING request" servers/master/master.log`
    if [ "$NMATCHES" -gt "$1" ]
    then
        echo "SUCCESS"
    else
        echo "FAIL"
    fi
}

test_NONEMPTY() {
    echo -n "verified_txns nonempty..."
    if [ -s verified_txns ]
    then
        echo "SUCCESS"
    else
        echo "FAIL"
    fi
}

test_MATCHES() {
    echo -n "verified_txns matches master..."
    diff verified_txns servers/master/verified_txns &>/dev/null
    result
}

trap "kill_jobs; exit" SIGINT SIGTERM

test1 () {
    echo ""
    echo "=== TEST 1: CONNECT ==="
    echo ""
    echo "We run the network until someone generates a transaction.  "
    echo "We then run your program to ensure that you're fetching this"
    echo "transaction with a CONNECT request."
    echo ""
    reset
    start_network
    while [ ! -s servers/master/verified_txns ]; do
        sleep 1
    done
    stop_server alice
    stop_server bob
    stop_server sender
    run_farmer 1
    stop_network
    test_MATCHES
}

test2 () {
    echo ""
    echo "=== TEST 2: PING ==="
    echo ""
    echo "We run your farmer alone and ensure that you're sending "
    echo "PING requests to master."
    echo ""
    reset
    start_network alone
    run_farmer 10
    stop_network
    test_PING 2
}

test3 () {
    echo ""
    echo "=== TEST 3: farming ==="
    echo ""
    echo "We run your farmer alone for 30s and ensure that you're "
    echo "sending valid BLOCK requests."
    echo ""
    reset
    start_network alone
    run_farmer 30
    stop_network
    echo -n "Did master receive a BLOCK request?..."
    grep "received BLOCK request" servers/master/master.log > /dev/null
    result
    echo -n "Was BLOCK valid?..."
    grep "valid block" servers/master/master.log > /dev/null
    result
}

test4 () {
    echo ""
    echo "=== TEST 4: TXN handling ==="
    echo ""
    echo "We run alice until she generates a block.  We then stop alice, start "
    echo "your farmer, and alice sends her coin to bob.  We ensure that you "
    echo "handle this transaction and send it in a valid BLOCK request to master."
    echo ""
    reset
    start_server master $MASTER_SOLN
    sleep 1
    start_server alice $FARMER_SOLN
    echo "waiting for alice to farm a block..."
    while [ ! -s servers/alice/verified_txns ]; do
        sleep 1
    done
    stop_server alice
    echo "running $FARMER for 30s..."
    $FARMER &
    FARMERPID=$!
    sleep 1
    pushd servers/alice/ > /dev/null
    echo "sending 1 coin from alice to bob..."
    $COINS_SOLN send bob 1 &>/dev/null
    popd > /dev/null
    sleep 30
    echo "stopping $FARMER..."
    kill $FARMERPID &>/dev/null
    echo -n "Did you send master a block with the new Txn?..."
    if [ `./coins show balances | grep -c "bob 1"` -eq 1 ]
    then
        echo "SUCCESS"
    else
        echo "FAIL"
    fi
    stop_network
}

test5 () {
    echo ""
    echo "=== TEST 5: BLOCK handling ==="
    echo ""
    echo "We run the full network for 30s.  We ensure that your "
    echo "verified_txns file matches master's."
    echo ""
    reset
    start_network
    run_farmer 30
    stop_network
    test_NONEMPTY
    test_MATCHES
}

case $TEST in
    0)
        test1;test2;test3;test4;test5;
        ;;
    1)
        test1
        ;;
    2)
        test2
        ;;
    3)
        test3
        ;;
    4)
        test4
        ;;
    5)
        test5
        ;;
esac
