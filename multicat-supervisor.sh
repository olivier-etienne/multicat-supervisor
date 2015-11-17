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

BASEDIR=$(dirname $(dirname $(readlink -f "$0")))

if [ ! -d $BASEDIR/var ]; then
	mkdir -p $BASEDIR/var
fi

if [ ! -d  $BASEDIR/logs ]; then
	mkdir -p $BASEDIR/logs
fi

PID=""
PIDFILE=$BASEDIR/var/multicat-supervisor.pid
LOCKFILE=$BASEDIR/var/multicat-supervisor.lock

multicat_pid() {
	PID=$(ps -ef | grep multicat-supervisor | grep -v grep | grep -v service | grep -v sh | awk '{print $2}')
	if [ -z "$PID" ]; then
		return 1
	fi
	return 0
}

start() {

	multicat_pid
	if [ $? -eq 0 ]; then
		echo "[ERROR] multicat already started..."
		return 1
	fi	
	
	killall -eq multicat-eit > /dev/null 2>&1
	rm $BASEDIR/var/*.pid $BASEDIR/var/*.lock > /dev/null 2>&1
	sleep 2
	
	nohup $BASEDIR/bin/multicat-supervisor -I $BASEDIR/conf/multicat.ini >> $BASEDIR/logs/multicat-supervisor.log 2>&1 &
	if [ $? -ne 0 ]; then
		echo "[ERROR] multicat launch failure"
		return 1
	fi
	
	sleep 5	

	multicat_pid
	if [ $? -ne 0 ]; then
		echo "[ERROR] pid of multicat-supervisor not found"	
		return 1
	fi

	echo $PID > $PIDFILE
	touch $LOCKFILE

	echo "  SUCCESS"

	return 0
}	

stop() {

	multicat_pid
	if [ $? -ne 0 ]; then
		echo "[ERROR] multicat not running..."	
		return 1
	fi
	
	PID=$(cat $PIDFILE)
	kill -TERM $PID
	if [ $? -ne 0 ]; then
		echo "[ERROR] kill multicat failed"
		return 1
	fi

	rm $PIDFILE $LOCKFILE > /dev/null 2>&1
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
		multicat_pid
		if [ $? -ne 0 ]; then
			echo "multicat not running..."	
			return 1
		fi
	
		PID=$(cat $PIDFILE)
		echo "multicat running PID[$PID] ..."
	;;
	*)
		echo "Usage: /usr/local/multicat-tools/multicat-supervisor.sh {start|stop|restart|status}"
		exit 1
	;;
esac

exit 0
