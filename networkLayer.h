//
// Created by Phillip Romig on 7/15/24.
//

#ifndef RFT_NETWORKLAYER_H
#define RFT_NETWORKLAYER_H

#include <netinet/in.h>
#include <cstring>
#include <random>
#include "datagram.h"

class networkLayerC {
private:
    bool server;
    int socketFd;
    std::mt19937 gen;
    std::bernoulli_distribution corruption;
    std::bernoulli_distribution loss;
    struct sockaddr_in serverAddr{};
    struct sockaddr_in clientAddr{};
public:
   // networkLayerC() : server(false), socketFd(0), lossRate_v(0.0), corruptionRate_v(0.0), gen(), corruption(), loss()  {};
   explicit networkLayerC(uint16_t portNum, float lossRate, float corruptionRate);
   ~networkLayerC();
   void udt_send( datagramS *data);
   void udt_receive(datagramS *data) ;

};

#endif //RFT_NETWORKLAYER_H
