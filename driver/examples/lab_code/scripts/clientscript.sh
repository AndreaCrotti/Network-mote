#!/bin/sh
#do not put in spaces between the = sign

#middlebox
#put in IP addr or hostname as defined in .ssh/config
MIDDLEBOX="hutch"
#remote host
#this instance runs the alpha daemon and the iperf server
#put in IP addr or hostname as defined in .ssh/config
REMOTEHOST="ironside"

#inits netem (the emulator for delay)
#sudo qdisc add dev eth0 root netem delay 0ms

#loop for some delays
for i in 0 1 2 3 4 5 6 7 8 9 
do
	#set delay
	#tc qdisc change dev eth0 root netem delay {$i}ms
	#setup alpha and iperf on foregin box
	ssh $REMOTEHOST "./hostscript.sh" &
	#setup alpha on middlebox
	ssh $MIDDLEBOX "./middleboxscript.sh" &
	#setup local alpha
	./alpha &
	#do iperf
	mkdir "association_$i"
	#wait that host set up iperf server
	for j in 0 1 2 3 4 5 6 7 8 9
	do
		iperf -c $REMOTEHOST -M 1300 > "test$j.txt"
	done
	#kill the server
	ssh $REMOTEHOST "pkill iperf"
	#kill the alpha daemon
	ssh $REMOTEHOST "pkill alpha"
	#kill the middlebox
	ssh $MIDDLEBOX "pkill alpha"
	#kill self
	pkill alpha
	#restart
done
