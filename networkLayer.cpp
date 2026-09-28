//
// Created by Phillip Romig on 7/15/24.
//

#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <random>
#include "networkLayer.h"
#include "logging.h"




// Constructor for the server
// hostname: the name of the server
// portNum: the port number to connect to
networkLayerC::networkLayerC(uint16_t portNum, float lossRate, float corruptionRate, unsigned int delay) : 
    inboundLossCount_v(0), inboundCorruptionCount_v(0), outboundLossCount_v(0), 
    outboundCorruptionCount_v(0), datagramsSent_v(0), datagramsRecieved_v(0),
    delay_v(delay), endOfFileRecieved_v(false)
{
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
            if (++portNum > 13000)
                portNum = 12000;
            serverAddr.sin_port = htons(portNum);
        } else {
            FATAL << "Error binding socketFd, throwing error #" << errno << ENDL;
            close(socketFd);
            throw(std::system_error(std::make_error_code(static_cast<std::errc>(errno)), strerror(errno)));
        }
    }
    std::cout << "Using port# " << portNum << std::endl;

    // Set up random number generator
    // The std::random_device gets a random number from the OS.
    std::random_device rd;

    // Since the entropy of the random device is limited, we use a Mersenne Twister to generate random numbers.
    gen.seed(rd());


    loss.param(std::bernoulli_distribution::param_type(lossRate));
    corruption.param(std::bernoulli_distribution::param_type(corruptionRate));
}




void networkLayerC::udt_send(datagramS *data)  {


    if (clientAddr.sin_family == 0) {
        FATAL << "Server can't send before it has received a message (client addr not set)." << ENDL;
        throw(std::runtime_error("Server can't send before it has received a message (client add not set)."));
    }
    

    if (!endOfFileRecieved_v) {
        if (loss(gen)) {
	        outboundLossCount_v++;
            WARNING << "Losing the outgoing datagram." << ENDL;
            return;
        }

        if (corruption(gen)) {
            WARNING << "Corrupting the outgoing datagram." << ENDL;
	        outboundCorruptionCount_v++;
            data->checksum++;
        }
    }


    // Slowing things down makes it eaiser to see what is happening and hence eaiser to debug.
    // It also makes performance more predictable for setting window in the client.
    if (delay_v>0) {
        TRACE << "Sleeping for " << delay_v << "ms." << ENDL;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_v));
    }

    DEBUG << "Sending datagram to " << inet_ntoa(clientAddr.sin_addr)
	      << ":" << ntohs(clientAddr.sin_port) << ENDL;
    TRACE << "Sending: " << toString(data) << ENDL;
    
    ssize_t bytesSent = sendto(socketFd, data, sizeof(datagramS), 0,
                                 (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    if (bytesSent == -1) {
        WARNING << "Error sending datagram." << ENDL;
        close(socketFd);
        socketFd = -1;
        throw(std::runtime_error("sendto() failed. Error #" + std::to_string(errno) + ": " + strerror(errno)));
    }
    datagramsSent_v++;
    DEBUG << "Successfully sent " << bytesSent << " bytes." << ENDL;
}

void networkLayerC::udt_receive(datagramS *data)   {

    bool notReceived = true;
    ssize_t bytesRead = 0;
    while (notReceived) {
        memset(&clientAddr, 0, sizeof(struct sockaddr_in));
        socklen_t addrLen = sizeof(clientAddr);
        DEBUG << "Calling recvfrom on socket with file descriptor #" << socketFd << ENDL;
        bzero(data,sizeof(datagramS));
        bytesRead = recvfrom(socketFd, (void *) data, sizeof(datagramS), 0,
                                     reinterpret_cast<struct sockaddr *>(&clientAddr), &addrLen);

        if (bytesRead == -1) {
            FATAL << "Error when calling recvfrom() throwing error #" << errno << ENDL;
            close(socketFd);
            socketFd = -1;
            throw (std::system_error(std::make_error_code(static_cast<std::errc>(errno)), strerror(errno)));
        }
	    datagramsRecieved_v++;
        notReceived = false;    
        
        if (!endOfFileRecieved_v) {

            if (loss(gen)) {
                WARNING << "Losing the incoming datagram." << ENDL;
                inboundLossCount_v++;
                notReceived = true;
            } else if (corruption(gen)) {
                inboundCorruptionCount_v++;
                WARNING << "Corrupting the incoming datagram." << ENDL;
                data->checksum++;
            }
        }
    }

    DEBUG << "Successfully received " << bytesRead << " bytes." << ENDL;
    TRACE << "Received: " << toString(data) << ENDL;
}

networkLayerC::~networkLayerC() {
    if (socketFd != -1) {
        close(socketFd);
    }
    DEBUG << "Input loss count " << inboundLossCount_v << ENDL;
    DEBUG << "Input corruption count " << inboundCorruptionCount_v << ENDL;
    DEBUG << "Output loss count " << outboundLossCount_v << ENDL;
    DEBUG << "Output corruption count " << outboundCorruptionCount_v << ENDL;
    DEBUG << "Datagrams recieved " << datagramsRecieved_v << ENDL;
    DEBUG << "Datagrams sendt " << datagramsSent_v << ENDL;

}

bool networkLayerC::dataAvalable(unsigned int timeToSleep) {
    fd_set rfds;
    int retval;
    struct timeval tv;


    FD_ZERO(&rfds);
    FD_SET(socketFd, &rfds);

    tv.tv_sec = timeToSleep;
    tv.tv_usec = 0;

   retval = select(socketFd+1, &rfds, NULL, NULL, &tv);
   if (retval == -1) {
        FATAL << "select failed, returning -1" << ENDL;
        throw (std::system_error(std::make_error_code(static_cast<std::errc>(errno)), strerror(errno)));
   }

   return retval;
}

void networkLayerC::endOfFileRecieved() {
    endOfFileRecieved_v = true;
}

