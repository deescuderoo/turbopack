#!/bin/bash

tc qdisc del dev lo root
tc qdisc add dev lo root netem delay 10ms rate 60Mbit

./gen_data.sh 5 5 21 37 53 69

tc qdisc del dev lo root
