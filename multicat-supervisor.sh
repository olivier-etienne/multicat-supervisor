#!/bin/sh
### BEGIN INIT INFO
# Provides:          multicat-supervisor
# Required-Start:    
# Required-Stop:     
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# X-Interactive:     true
# Short-Description: Start/stop multicat-supervisor
### END INIT INFO

#set -e

SCRIPTNAME="${0##*/}"
SCRIPTNAME="${SCRIPTNAME##[KS][0-9][0-9]}"

PATHSCRIPTBIN=`dirname $0`
BASEDIR=${PATHSCRIPTBIN%/*}

if [ ! -d $BASEDIR/var ]; then
	mkdir -p $BASEDIR/var
fi

if [ ! -d  $BASEDIR/logs ]; then
	mkdir -p $BASEDIR/logs
fi

PIDFILE=$BASEDIR/var/multicat-supervisor.pid
LOCKFILE=$BASEDIR/var/multicat-supervisor.lock

pidof_multicat() {

	PID=`ps -ef | grep multicat-supervisor | grep -v grep | grep -v sh | awk '{print $2}'`
	if [ -z "$PID" ]; then
		return 1
	fi
	
	echo $PID > $PIDFILE
	if [ $? -ne 0 ]; then
		return 1
	fi
	return 0
}

start() {

	if [ -e $LOCKFILE ]; then
		echo "[ERROR] Program already started..."
		return 1
	fi
	
	killall -eq multicat-eit > /dev/null 2>&1
	sleep 2
	
	$BASEDIR/bin/multicat-supervisor -I $BASEDIR/conf/multicat.ini >> $BASEDIR/logs/multicat-supervisor.log 2>&1 &
	if [ $? -ne 0 ]; then
		echo "[ERROR] launch failure"
		return 1
	fi
	
	sleep 5
	
	pidof_multicat
	if [ $? -ne 0 ]; then
		echo "[ERROR] pid not found"	
		return 1
	fi
	
	touch $LOCKFILE
	echo "  SUCCESS"

	return 0
}	

stop() {

	if [ ! -e $LOCKFILE ]; then
		echo "[ERROR] Program not started..."
		return 1
	fi

	if [ ! -e $PIDFILE ]; then
		echo "[ERROR] missing pid file..."
		return 1
	fi
	
	PID=`cat $PIDFILE`
	kill -TERM $PID
	if [ $? -ne 0 ]; then
		echo "[ERROR] kill programm failed"
		return 1
	fi

	rm -f $PIDFILE $LOCKFILE 
	echo "	SUCCESS"
}


case $1 in
	start)
		echo -n "[START]	"
		start
	;;
	stop)
		echo -n "[STOP]	"	
		stop
	;;
	restart)
		echo -n "[STOP]	"	
		stop
		sleep 5
		echo -n "[START]	"
		start
	;;
	status)
		echo -n "[STATUS]	"
		if [ ! -e $LOCKFILE ]; then
			echo "Program not started..."
			exit 1
		fi

		if [ ! -e $PIDFILE ]; then
			echo "Program not started..."
			exit 1
		fi
	
		PID=`cat $PIDFILE`
		echo "Program started PID[$PID] ..."
	;;
	*)
		echo "Usage: /etc/init.d/multicat-supervisor.sh {start|stop|restart|status}"
		exit 1
	;;
esac

exit 0
