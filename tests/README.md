
## sh mdl 行情回放

> python ./mdl_csv_send.py send_csv_files --data_dir=/home/zhiyuan/20231129 --sleep=0.5 --zmq_address=tcp://127.0.0.1:5555 --suffix=",mdl_4_24" --skip_first=False --allows=["688299.csv"] --bind=True

播放指定目录下的sh 指定股票的 逐笔委托和成交记录

> python ./mdl_csv_send.py send_csv_files --data_dir=/home/zhiyuan/projects/lobster --sleep=2 --zmq_address=tcp://127.0.0.1:5555 --suffix=",mdl_4_24" --skip_first=False --allows=["688299.csv"] --bind=True
>
cat ../688299.csv |  awk 'NR >= 1000 && NR < 1000 + 1000'


lob <-  mdl_zmq  <- mdl_file(sh/sz)
        tlive <- 

lob ->  zmq-pub  :      tcp://127.0.0.1:5556 (msgpack)
        file-write :     store_dir/6000234.lob
        redis-hmset     127.0.0.1:6379  ( redis hmgetall 6000234 )
