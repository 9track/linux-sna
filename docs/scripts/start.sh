#!/bin/sh

NETID="lnxsna"
NAME="ehead"
PNETID="lnxsna"
PNAME="ibm"

ifconfig eth0 allmulti
./snaconfig -d 20 $NETID.$NAME nodeid 012345678 appn start
./snaconfig -d 20 $NETID.$NAME dlc eth0 0x04 pri
./snaconfig -d 0 $NETID.$NAME dlc eth0 0x04 start
./snaconfig $NETID.$NAME cpic APING '#INTER' $PNETID.$PNAME aping
./snaconfig $NETID.$NAME link eth0 0x04 $PNETID.$PNAME 00A0CC570AF2 0x04
./snaconfig lnxsna.ehead mode '#INTERC' lnxsna.ibm '#INTERC'
