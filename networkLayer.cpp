//
// Created by Phillip Romig on 7/15/24.
//

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "networkLayer.h"
#include "logging.h"




// Constructor for the server
// hostname: the name of the server
// portNum: the port number to connect to
networkLayerC::networkLayerC(uint16_t portNum) {
    TRACE << "Creating a networkLayerC object with flavor server." << ENDL;
    // Create a UDP socketFd
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1) {
        FATAL << "Error creating socketFd, throwing error #" << errno << ENDL;
        throw(std::system_error(std::make_error_code(static_cast<std::errc>(errno)), strerror(errno)));
    }

    // Set up the server address
    memset(&serverAddr, 0, sizeof(struct sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum); // Port number
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on any interface

    // Bind the socketFd to the address
    while (bind(socketFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1) {
        if (errno == EADDRINUSE) {
            WARNING << "Port #" << portNum << " is in use. Trying next port." << ENDL;
            portNum++;
            serverAddr.sin_port = htons(portNum);
        } else {
            FATAL << "Error binding socketFd, throwing error #" << errno << ENDL;
            close(socketFd);
            throw(std::system_error(std::make_error_code(static_cast<std::errc>(errno)), strerror(errno)));
        }
    }
    TRACE << "Successfully bound to port#" << portNum << ENDL;


    server = true;
}




void networkLayerC::udt_send(const datagramS *data)  {

    struct sockaddr_in destinationAddr{};
    if (server) {
        if (clientAddr.sin_family == 0) {
            FATAL << "Server can't send before it has received a message (client addr not set)." << ENDL;
            throw(std::runtime_error("Server can't send before it has received a message (client add not set)."));
        }
        destinationAddr = clientAddr;
    } else {
        destinationAddr = serverAddr;
    }

    DEBUG << "Sending datagram to " << inet_ntoa(destinationAddr.sin_addr) << ":" << ntohs(destinationAddr.sin_port) << ENDL;
    TRACE << "Sending: " << toString(data) << ENDL;
    ssize_t bytesSent = sendto(socketFd, data, sizeof(datagramS), 0,
                                 (struct sockaddr*)&destinationAddr, sizeof(destinationAddr));
    if (bytesSent == -1) {
        std::cerr << "Error receiving datagram." << std::endl;
        close(socketFd);
        throw(std::runtime_error("Server can't send before it has received a message (client add not set)."));
    }
    DEBUG << "Successfully sent " << bytesSent << " bytes." << ENDL;
}

void networkLayerC::udt_receive(const datagramS *data)  {

    memset(&clientAddr, 0, sizeof(struct sockaddr_in));
    socklen_t addrLen = sizeof(clientAddr);
    DEBUG << "Calling recvfrom on socket with file descriptor #" << socketFd << ENDL;
    ssize_t bytesRead = recvfrom(socketFd, (void *) data, sizeof(datagramS), 0,
                              reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen);

    if (bytesRead == -1) {
        FATAL << "Error when calling recvfrom() throwing error #" << errno << ENDL;
        close(socketFd);
        throw(std::system_error(std::make_error_code(static_cast<std::errc>(errno)), strerror(errno)));
    }

    DEBUG << "Successfully received " << bytesRead << " bytes." << ENDL;
    TRACE << "Received: " << toString(data) << ENDL;
}

networkLayerC::~networkLayerC() {
    if (socketFd != 0) {
        close(socketFd);
    }

}

