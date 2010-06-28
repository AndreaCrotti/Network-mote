#!/usr/bin/env zsh
# 
# usage: ./test_mit_delay.sh [router]

DELAYS=(0ms 1ms 10ms 100ms)

if [ ! $1 ]; then
	echo "** No router given, using localhost"
else
	echo "** Using $1 as router"
	ROUTER=$1
fi

if [ $ROUTER ]; then
	echo "Set local routes"
	./route_up.sh
fi

for delay in $DELAYS; do

	echo "Delay=$delay setzen"
	if [ $ROUTER ]; then
		ssh root@$ROUTER "tc qdisc del dev eth0 root netem"
		ssh root@$ROUTER "tc qdisc add dev eth0 root netem delay $delay"
	else
		tc qdisc del dev eth0 root netem
		tc qdisc add dev eth0 root netem delay $delay
	fi
	echo "** Starting ./alphatest.rb $delay"
	./alphatest.rb $delay
done


if [ $ROUTER ]; then
	echo "Bring down local routes"
	./route_down.sh
fi
