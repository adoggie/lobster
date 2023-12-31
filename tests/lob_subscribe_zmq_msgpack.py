
# subscribe.py

# pip install msgpack

import zmq
import msgpack

# 创建ZMQ套接字并订阅消息
context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect("tcp://localhost:5556")
socket.setsockopt(zmq.SUBSCRIBE, b'')

# 接收并解码消息
while True:
    message = socket.recv()
    data = msgpack.unpackb(message, raw=False)

    # 访问解码后的数据
    source = data['source']
    symbolid = data['symbolid']
    asks = data['asks']
    bids = data['bids']
