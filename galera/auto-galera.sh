if [ $# -lt 2 ]
then
   echo "Usage: <pintool> <benchmark_name>"
   echo
   exit 0 
fi

galera start $1

galera sysbench $2 ${@:3}

galera stop

exit 1
