#!/usr/bin/bash

case "$1" in
    "wan")
	sudo tc qdisc add dev lo root netem delay 100ms rate 100Mbit
	;;
    "lan")
	sudo tc qdisc add dev lo root netem delay 1ms rate 100Mbit
	;;
    "normal")
	sudo tc qdisc del dev lo root
	;;    
    "custom")
	sudo tc qdisc add dev lo root netem delay ${2}ms rate ${3}Mbit
	;;
    *)
	echo "OPTIONS: use 'wan' or 'normal'."
	exit 1
	;;
esac
