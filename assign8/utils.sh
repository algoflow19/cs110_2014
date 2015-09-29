FARMER_SOLN=`pwd`/farmer_soln
MASTER_SOLN=`pwd`/master
COINS_SOLN=`pwd`/coins

stop_server () {
    PIDFILE=servers/"$1"/pid
    if [ -e "$PIDFILE" ]
    then
        PID="$(cat $PIDFILE)"
        if ps -p $PID > /dev/null
        then
            echo "stopping $1..."
            kill $PID &>/dev/null
        fi
        rm $PIDFILE
    fi
}

stop_network () {
    stop_server master
    stop_server alice
    stop_server bob
    stop_server sender
}

start_server () {
    pushd servers/"$1" > /dev/null
    echo "starting $1..."
    "$2" &>"$1.log" &
    PID=$!
    echo $PID > pid
    #disown $PID
    popd > /dev/null
}

start_network () {
    # start master

    start_server master $MASTER_SOLN
    sleep 1

    if [ $# -eq 0 ] || [ $1 != "alone" ]
    then
        # start farmers
        start_server alice $FARMER_SOLN
        sleep 1
        start_server bob $FARMER_SOLN
        sleep 1

        # start txn sender
        echo "starting sender..."
        python send.py &>servers/sender/send.log &
        SENDPID=$!
        echo $SENDPID > servers/sender/pid
        #disown $SENDPID
    fi
}

reset () {
    stop_network

    echo "resetting network..."

    # ensure directories exist
    mkdir -p servers/master
    mkdir -p servers/alice
    mkdir -p servers/bob
    mkdir -p servers/sender

    # remove directory contents
    echo "removing server directories..."
    rm -rf servers/master/*
    rm -rf servers/alice/*
    rm -rf servers/bob/*
    rm -rf servers/sender/*

    # generate keys
    echo "generating keys..."
    openssl genrsa -out servers/alice/privkey.pem 1024 &>/dev/null
    openssl rsa -in servers/alice/privkey.pem -pubout > servers/alice/pubkey.pem 2> /dev/null
    openssl genrsa -out servers/bob/privkey.pem 1024 &>/dev/null
    openssl rsa -in servers/bob/privkey.pem -pubout > servers/bob/pubkey.pem 2> /dev/null

    # generate user file
    rm -f localusers
    echo "alice" >> localusers
    cat servers/alice/pubkey.pem >> localusers
    echo "bob" >> localusers
    cat servers/bob/pubkey.pem >> localusers
    echo `whoami` >> localusers
    cat pubkey.pem >> localusers

    # remove our verified_txns file
    rm -f verified_txns

    # link user file
    ln -s ../../localusers servers/alice/
    ln -s ../../localusers servers/bob/
    ln -s ../../localusers servers/master/
}


kill_jobs () {
    stop_network
    jobs -p | xargs kill &>/dev/null
}
