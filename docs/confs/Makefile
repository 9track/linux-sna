# Makefile for etc-sna, install an automatic SNA start up system in a stadard
# Linux environemnt
#
# IMPORTANT: THIS SCRIPT INSTALL WILL OVERWRITE ANY EXISTING CONFIGURATION FILE
#            FOR SNA - THOUGH IT DOES TRY AND BACKUP EVERYTHING
#
# Files etc-sna owns:
# /etc/rc.d/init.d/snatch
# /etc/rc.d/init.d/snatchd.conf
# /etc/sysconfig/sna/
#
# Files Etc-sna modifies:
# /etc/rc.d/init.d/network
# /etc/sysconfig/network
#

NETWORKC="/etc/sysconfig/network"
NETWORKI="/etc/rc.d/init.d/network"
SNAC="/etc/sna.cfg"

all: backup net snaconfig

backup:
	if [ -f ${NETWORKC} ]; then \
		cp -f ${NETWORKC} ${NETWORKC}.bk; \
	fi
	if [ -f ${NETWORKI} ]; then \
		cp -f ${NETWORKI} ${NETWORKI}.bk; \
	fi
	if [ -f ${SNAC} ]; then \
		cp -f ${SNAC} ${SNAC}.bk; \
	fi

net:
	if ! grep SNA ${NETWORKC} > /dev/null; then \
		echo "SNA=yes" >> ${NETWORKC}; \
	fi
	cp -f network ${NETWORKI}

snaconfig:
	cp -f sna.cfg ${SNAC}
