cmake_minimum_required(VERSION 3.14)
set(CMAKE_TOOLCHAIN_FILE "/home/zhiyuan/vcpkg/scripts/buildsystems/vcpkg.cmake")
# set(CMAKE_PREFIX_PATH "/home/zhiyuan/vcpkg/packages")
project(lobster LANGUAGES CXX)


# -DCMAKE_TOOLCHAIN_FILE=/home/zhiyuan/vcpkg/scripts/buildsystems/vcpkg.cmake
# add_compile_definitions(USE_VCPKG)
# set(CMAKE_PREFIX_PATH "/home/zhiyuan/vcpkg/installed/x64-linux/share/qt5")
add_compile_definitions(USE_VCPKG)

# link_directories(/home/zhiyuan/vcpkg/installed/x64-linux/lib)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include_directories(/home/zhiyuan/projects/include)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
message(STATUS "Qt${QT_VERSION_MAJOR}::Core")
# find_package(cppzmq)
find_package(jsoncpp  CONFIG REQUIRED)
find_package(cppzmq  CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
if(PROTOBUF_FOUND)
  message(STATUS "protobuf_FOUND")
  message(STATUS "protobuf_VERSION ${protobuf_VERSION}")
  message(STATUS "protobuf_Libs ${protobuf_LIBRARIES}")

endif()

find_package(log4cplus CONFIG REQUIRED)
find_package(msgpack-cxx CONFIG REQUIRED)

add_executable(lobster
  main.cpp service.cpp feed_zmq.cpp feed_zmq.h lob.cpp worker.cpp worker.h fanout.cpp fanout.h
  feed_csv.h feed_csv.cpp
  proto/lob.pb.h 
  # proto/lob.pb.cc
)
target_link_libraries(lobster Qt${QT_VERSION_MAJOR}::Core JsonCpp::JsonCpp cppzmq protobuf log4cplus::log4cplus msgpack-cxx )
# target_link_libraries(lobster PRIVATE )
# target_link_options(lobster PRIVATE -lprotobuf -lpthread)

include(GNUInstallDirs )
install(TARGETS lobster
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
