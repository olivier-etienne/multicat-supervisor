#!/bin/sh
### BEGIN INIT INFO
# Provides:          multicat-eit
# Required-Start:    
# Required-Stop:     
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# X-Interactive:     true
# Short-Description: Start/stop multicat-eit
### END INIT INFO

#set -e

SCRIPTNAME="${0##*/}"
SCRIPTNAME="${SCRIPTNAME##[KS][0-9][0-9]}"

PATHSCRIPTBIN=`dirname $0`
BASEDIR=${PATHSCRIPTBIN%/*}
VIDEOPATH=$BASEDIR/videos

if [ ! -d $BASEDIR/var ]; then
	mkdir -p $BASEDIR/var
fi

if [ ! -d  $BASEDIR/logs ]; then
	mkdir -p $BASEDIR/logs
fi


pidof_multicat() {

	PID=`ps -ef | grep multicat-eit | grep "\-P $PROVIDER" | grep -v grep | grep -v sh |grep -v supervisor | awk '{print $2}'`
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
	
	ulimit -c unlimited
	$BASEDIR/bin/multicat-eit -l -P $PROVIDER -I $BASEDIR/conf/multicat.ini $VIDEOPATH/$VIDEO $ADDRESS:$PORT >> $BASEDIR/logs/multicat-eit.log 2>&1 &
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
	echo "[SUCCESS]"

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

usage()
{
    echo "usage: ${SCRIPTNAME} <-p|--provider provider number> <-v|--video file> <-a|--address adr> <-P|--port number> [-h]] start|stop|restart|status"
}
OPTS=`getopt -o hp:v:a:P: --long provider:,video:,address:,port:,help -n '${SCRIPTNAME}' -- "$@"`
 
if [ $? != 0 ] ; then echo "Failed parsing options." >&2 ; exit 1 ; fi
 
#echo "$OPTS"
eval set -- "$OPTS"
 
PROVIDER=
VIDEO=
ADDRESS=
PORT=
 
while true; do
case "$1" in
-p | --provider ) PROVIDER="$2"; shift; shift ;;
-v | --video ) VIDEO="$2"; shift; shift ;;
-a | --address ) ADDRESS="$2"; shift; shift ;;
-P | --port ) PORT="$2"; shift; shift ;;
-h | --help ) usage; exit 0;;
-- ) shift; break ;;
* ) break ;;
esac
done 

if [ -z "$PROVIDER" ]; then
	usage
	exit 1
fi

if [ "$1" = "start" ]; then
	if [ -z "$VIDEO" -o -z "$ADDRESS" -o -z "$PORT" ]; then
		usage
		exit 1
	fi
	VIDEO=`basename $VIDEO`
	
fi

PIDFILE=$BASEDIR/var/multicat-eit.$PROVIDER.pid
LOCKFILE=$BASEDIR/var/multicat-eit.$PROVIDER.lock
RET=0

case $1 in
	start)
	    echo ""
		echo -n "Starting $SCRIPTNAME ...... "       
		start
		RET=$?
	;;
	stop)
	    echo ""
		echo -n "Stopping $SCRIPTNAME ...... "       
		stop
		RET=$?
	;;
	restart)
	    echo ""
		echo -n "Stopping $SCRIPTNAME ...... "       
		stop
		sleep 5
	    echo ""
		echo -n "Starting $SCRIPTNAME ...... "       
		start
		RET=$?
	;;
	status)
	    echo ""
		echo -n "Status $SCRIPTNAME ...... "       
		if [ ! -e $LOCKFILE ]; then
			echo "[NOT STARTED]"
			exit 1
		fi

		if [ ! -e $PIDFILE ]; then
			echo "[NOT STARTED]"
			exit 1
		fi
	
		PID=`cat $PIDFILE`
		echo "[STARTED $PID]"
	;;
	*)
		usage
		exit 1
	;;
esac

exit $RET
