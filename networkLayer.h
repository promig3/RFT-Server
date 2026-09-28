//
// Created by Phillip Romig on 7/15/24.
//

#ifndef RFT_NETWORKLAYER_H
#define RFT_NETWORKLAYER_H

#include <netinet/in.h>
#include <cstring>
#include <random>
#include <chrono>
#include <thread>
#include "datagram.h"

class networkLayerC {
private:
    bool endOfFileRecieved_v;
    int socketFd;
    int inboundLossCount_v;
    int inboundCorruptionCount_v;
    int outboundLossCount_v;
    int outboundCorruptionCount_v;
    int datagramsSent_v;
    int datagramsRecieved_v;
    unsigned int delay_v;
    std::mt19937 gen;
    std::bernoulli_distribution corruption;
    std::bernoulli_distribution loss;
    struct sockaddr_in serverAddr{};
    struct sockaddr_in clientAddr{};
public:
   explicit networkLayerC(uint16_t portNum, float lossRate, float corruptionRate,unsigned int delay);
   ~networkLayerC();
   void udt_send( datagramS *data);
   void udt_receive(datagramS *data) ;
   bool dataAvalable(unsigned int timeToSleep);
   void endOfFileRecieved();

};

#endif //RFT_NETWORKLAYER_H
