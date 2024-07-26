cmake_minimum_required(VERSION 3.28)
project(RFT)

set(CMAKE_CXX_STANDARD 23)

add_executable(server
        networkLayer.cpp
        networkLayer.h
        main.cpp
        logging.h
        datagram.cpp
        datagram.h
)
