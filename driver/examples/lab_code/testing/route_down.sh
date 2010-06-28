#!/bin/sh

echo "Lokale Routen loeschen"
route del silver-2
route del silver-1
route add -net 137.226.154.0 netmask 255.255.255.0 dev eth0
route add default gw 137.226.154.1

