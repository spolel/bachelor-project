if [ $# -eq 0 ]
	then
    		echo "No arguments supplied"
		echo "Use galera help"
		exit 0 
fi

case "$1" in
   "help")
   echo 
   echo " Galera help page : "
   echo " this program will automate the process of doing benchmarks instrumented by pin "
   echo " the output file of the pintool is created in the directory you run it from"
   echo " the output of the benchmarks is created in the ~/out directory"
   echo
   echo " help					display help page"
   echo " start <pintool>			start the server with a pintool attached"
   echo " sysbench <benchmark_name> [options]	start a benchmark"
   echo "					( you can also set some permanent options in /etc/galera/sysbench-galera.cnf )"
   echo " sysbench help    			display sysbench help page"
   echo " sysbench <benchmark_name> help		display benchmark help page"
   echo " stop 					stop the server"
   echo " status					show status of the server"
   echo

   exit 1 
   ;;
   "start")
   if [ $# -eq 1 ]
   then
      echo "No pintool provided : start <pintool>" 
      exit 0
   fi
   echo starting server

   echo pw | sudo -S /home/spolel/pin-3.6/pin -t /home/spolel/pin-3.6/source/tools/MyPinTool/obj-intel64/$2.so -- /usr/sbin/mysqld --console & 

   while (echo $(mysql -u root -ppw -e "SHOW STATUS LIKE 'wsrep_cluster_size'" 2>&1 >/dev/null) | grep -q ERROR)
   do
      sleep 0.1
      echo -en "server starting .   \r"
      sleep 0.1
      echo -en "server starting ..  \r"
      sleep 0.1
      echo -en "server starting ... \r"
   done

   echo server is up
   echo $(mysql -u root -ppw -e "SHOW STATUS LIKE 'wsrep_cluster_size'")
   
   exit 1
   ;;
   "sysbench")
   if [ $# -eq 1 ]
   then
      echo "Usage :" 
      echo "sysbench <name> [options]		start benchmark"
      echo "					( you can also set some permanent options in /etc/galera/sysbench-galera.cnf )"
      echo "sysbench help			get sysbench options"
      echo "sysbench <benchmark_name> help	get benchmark options" 
      exit 0
   fi

   if [[ $2 = "help" ]]
   then
      sysbench --help
      exit 1
   fi 

   if [[ $# = 3 && $3 = "help" ]]
   then
      sysbench $2 help
      exit 1
   fi

   if (echo $(mysql -u root -ppw -e "SHOW STATUS LIKE 'wsrep_cluster_size'" 2>&1 >/dev/null) | grep -q ERROR)
   then
      echo Server is not up, cannot start benchmark.
      exit 0
   fi

   touch $HOME/out/$2-$(date +%Y-%m-%d-%H:%M:%S).out

   sysbench --mysql-user=root --mysql-password=pw --db-driver=mysql $(< /etc/galera/sysbench-galera.cnf) ${@:3} $2 prepare
   sysbench --mysql-user=root --mysql-password=pw --db-driver=mysql $(< /etc/galera/sysbench-galera.cnf) ${@:3} $2 run | tee $HOME/out/$2-$(date +%Y-%m-%d-%H:%M:%S).out
   sysbench --mysql-user=root --mysql-password=pw --db-driver=mysql $(< /etc/galera/sysbench-galera.cnf) ${@:3} $2 cleanup

   exit 1
   ;;
   "stop")

   if (echo $(mysql -u root -ppw -e "SHOW STATUS LIKE 'wsrep_cluster_size'" 2>&1 >/dev/null) | grep -q ERROR)
   then
      echo Server is already down.
      exit 0
   fi

   echo killing server
   echo pw | sudo -S kill $(pgrep mysql)

   while ( pgrep mysql > /dev/null )
   do
      sleep 0.1
      echo -en "server shutting down .   \r"
      sleep 0.1
      echo -en "server shutting down ..  \r"
      sleep 0.1
      echo -en "server shutting down ... \r"
   done

   #c++filt -n < proccount.out > pc.out

   exit 1
   ;;
   "status")
   if (echo $(mysql -u root -ppw -e "SHOW STATUS LIKE 'wsrep_cluster_size'" 2>&1 >/dev/null) | grep -q ERROR)
   then
      echo Server is down.
   else
      echo Server is up.
   fi
   exit 1
   ;;
esac

echo "Not recognised"
echo "Use galera help" 

exit 0
