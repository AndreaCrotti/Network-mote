#!/bin/bash
cd $TOSROOT/apps/IPBaseStation
echo "programming the mote in port USB0"
make micaz blip install /dev/ttyUSB0
cd $OLDPWD