i=1

sysbench --db-driver=mysql oltp_read_write prepare --table_size=10000 --distinct_ranges=10 --sum_ranges=10 --index_updates=10 --delete_inserts=10 --simple_ranges=10 --order_ranges=10 --point_selects=10 --non_index_updates=10 --tables=2 --non_index_updates=10 --simple_ranges=10 --order_ranges=10 --point_selects=10 --range_size=$i  

while [ $i -le 10000 ]
do
echo $i
sysbench --db-driver=mysql oltp_read_write run --table_size=10000 --distinct_ranges=10 --sum_ranges=10 --index_updates=10 --delete_inserts=10 --simple_ranges=10 --order_ranges=10 --point_selects=10 --non_index_updates=10 --tables=2 --non_index_updates=10 --simple_ranges=10 --order_ranges=10 --point_selects=10 --range_size=$i 

((i=i+99))
done

sysbench --db-driver=mysql oltp_read_write cleanup --table_size=10000 --distinct_ranges=10 --sum_ranges=10 --index_updates=10 --delete_inserts=10 --simple_ranges=10 --order_ranges=10 --point_selects=10 --non_index_updates=10 --tables=2 --non_index_updates=10 --simple_ranges=10 --order_ranges=10 --point_selects=10 --range_size=$i 
