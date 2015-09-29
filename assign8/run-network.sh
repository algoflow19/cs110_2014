#!/bin/bash

set -e

. utils.sh

trap "kill_jobs; exit" SIGINT SIGTERM

RESET=0
ALONE=0

while test $# -gt 0; do
    case "$1" in
        -h)
            echo "./run-network.sh - run network servers"
            echo "./run-network.sh -h"
            echo "./run-network.sh [-r] [alone]"
            echo " "
            echo "options:"
            echo "  -h      print this help message"
            echo "  -r      reset verified_txns before starting"
            echo "  alone   don't run other farmer servers.  only run master"
            exit 0
            ;;
        -r)
            shift
            RESET=1
            ;;
        alone)
            shift
            ALONE=1
            ;;
        *)
            break
            ;;
    esac
done


if [ $RESET -eq 1 ] || [ ! -d "servers" ]
then
    reset
fi

if [ $ALONE -eq 1 ]
then
    start_network alone
else
    start_network
fi

echo "sleeping forever"
sleep infinity
