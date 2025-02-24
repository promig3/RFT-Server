cmake_minimum_required(VERSION 3.16)
project(RFT-Server)

set(CMAKE_CXX_STANDARD 20)

add_executable(rft-server
        networkLayer.cpp
        networkLayer.h
        main.cpp
        logging.h
        datagram.cpp
        datagram.h
)
