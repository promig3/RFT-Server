//
// Created by Phillip Romig on 7/15/24.
//

#ifndef RFT_NETWORKLAYER_H
#define RFT_NETWORKLAYER_H

#include <netinet/in.h>
#include <string.h>
#include "datagram.h"

class networkLayerC {
private:
    bool server;
    int socketFd;
    struct sockaddr_in serverAddr{};
    struct sockaddr_in clientAddr{};
public:
   networkLayerC() : server(false), socketFd(0) {};
   explicit networkLayerC(uint16_t portNum);
   ~networkLayerC();
   void udt_send(const datagramS *data) ;
   void udt_receive(const datagramS *data) ;
};

#endif //RFT_NETWORKLAYER_H
