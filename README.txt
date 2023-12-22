
# Lobster 
limit order book
实时订单簿 c++ implementation

exchange --> lobster -->   strategy-app 
         zmq        fanout
         mdl-data 

settings.json 
"cmake.configureSettings": {
        "CMAKE_TOOLCHAIN_FILE": "/home/zhiyuan/vcpkg/scripts/buildsystems/vcpkg.cmake",
    }


cpplint 
https://cloud.tencent.com/developer/article/1494003

vcpkg install 
protobuf cppzmq jsoncpp
vcpkg install log4cplus

https://github.com/log4cplus/log4cplus
https://github.com/log4cplus/log4cplus/wiki/Code-Examples

protobuf compile 
protoc -I ./ --cpp_out ./ lob.proto

https://protobuf.dev/
    protobuf:x64-linux                                3.21.12 

https://github.com/protocolbuffers/protobuf/releases/download/v21.0/protoc-21.0-linux-x86_64.zip
protoc --version 
    libprotoc 3.21.0