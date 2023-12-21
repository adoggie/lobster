
import os
import zmq
import time 
import fire

def send_csv_files(data_dir, zmq_address ,suffix='', sleep=0, bind=True,skip_first = True , allows=[]):
    print(data_dir, zmq_address ,suffix, sleep, bind,skip_first , allows)
     

    context = zmq.Context()
    socket = context.socket(zmq.PUB)
    if bind:
        socket.bind(zmq_address)
    else:
        socket.connect(zmq_address)

    for filename in os.listdir(data_dir):
        if filename.endswith(".csv"):
            
            if allows and filename not in allows:
                continue
            
            file_path = os.path.join(data_dir, filename)
            print(file_path)
            with open(file_path, "r") as file:
                lines = file.readlines()
                from_ = skip_first and 1 or 0
                for line in lines[from_:]:
                    if sleep > 0:
                        time.sleep(sleep)
                    line = line.strip() if suffix == '' else line.strip() + suffix
                    print(">>", line)
                    socket.send_string( line)

    socket.close()
    context.term()

# Example usage
data_dir = "/home/zhiyuan/20231129"
zmq_address = "tcp://127.0.0.1:5555"

if __name__ == '__main__':
    fire.Fire()
    
# send_csv_files(data_dir, zmq_address,skip_first=False,allows=["688299.csv"])
# python ./mdl_csv_send.py send_csv_files --data_dir=/home/zhiyuan/20231129 --sleep=0.5 --zmq_address=tcp://127.0.0.1:5555 --suffix=",mdl_4_24" --skip_first=False --allows=["688299.csv"] --bind=True
