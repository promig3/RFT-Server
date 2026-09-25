//
// Created by Phillip Romig on 7/15/24.
//
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <chrono>
#include <thread>

#include "networkLayer.h"
#include "logging.h"


std::ofstream open_output_file(const std::string &filename) {
    //if (std::filesystem::exists(filename)) {
    //    throw std::runtime_error("File already exists: " + filename);
    //}

    std::ofstream outputFile(filename, std::ios::trunc | std::ios::out | std::ios::binary);
    if (!outputFile.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }
    return outputFile;
}

void deliver_data(std::ofstream &outputFile, char *data, int length) {
    DEBUG << "Writing " << length << " bytes to output file." << ENDL;
    outputFile.write(data, length);
    if (!outputFile) {
        throw std::runtime_error("Error writing to output file.");
    }
    TRACE << "Data written to output file." << ENDL;
}


int main(int argc, char* argv[]) {

    // Defaults
    uint16_t portNum(12345);  // On Isengard only ports 12,000 - 13,000 are open.
    std::string outputFileName("");
    float lossRate(0.0);
    float corruptionRate(0.0);

    int opt;
    try {
        while ((opt = getopt(argc, argv, "f:p:d:l:c:")) != -1) {
            switch (opt) {
                case 'l':
                    lossRate = std::stof(optarg);
                    break;
                case 'c':
                    corruptionRate = std::stof(optarg);
                    break;
                case 'p':
                    portNum = std::stoi(optarg);
                    break;
                case 'd':
                    LOG_LEVEL = std::stoi(optarg);
                    break;
                case 'f':
                    outputFileName = optarg;
                    break;
                case '?':
                default:
                    std::cout << "Usage: " << argv[0] << " -f filename [-p port] [-d debug_level] [-l lossRate] [-c corruptionRate] " << std::endl;
                    break;
            }
        }
    } catch (std::exception &e) {
        FATAL << "Invalid command line arguments: " << e.what() << ENDL;
        std::cout << "Usage: " << argv[0] << " -f filename [-p port] [-d debug_level] [-l lossRate] [-c corruptionRate] " << std::endl;
        return(1);
    }

    if (outputFileName == "") {
       FATAL << "Invalid command line arguments: output filename is required."  << ENDL;
        std::cout << "Usage: " << argv[0] << " -f filename [-p port] [-d debug_level] [-l lossRate] [-c corruptionRate] " << std::endl;
        return(1);
    }
   
    INFO << "Command line arguments parsed." << ENDL;
    INFO << "\tOutput file name: " << outputFileName << ENDL;
    INFO << "\tPort number: " << portNum << ENDL;
    INFO << "\tDebug Level: " << LOG_LEVEL << ENDL;
    INFO << "\tLoss Rate: " << lossRate << ENDL;
    INFO << "\tCorruption Rate: " << corruptionRate << ENDL;
  

    //
    // Open the output file
    //
    std::ofstream outputFile;
    try {
        TRACE << "Opening output file: " << outputFileName << ENDL;
        outputFile = open_output_file(outputFileName);
        TRACE "Output file opened." << ENDL;
    } catch (std::exception &e) {
        FATAL << "Error opening output file: " << e.what() << ENDL;
        return(1);
    }

    //
    // Create the ACK packet we will send back each time. Only the ACK number will change.
    //
    uint16_t expectedSeqNum = 1;
    auto *sndpkt = new datagramS;
    sndpkt->seqNum = 0;
    sndpkt->ackNum = 0;
    sndpkt->payloadLength = 0;
    sndpkt->checksum = computeChecksum(sndpkt);


    try {
        auto *network = new networkLayerC(portNum, lossRate, corruptionRate);
        auto *datagram = new datagramS;
        bool notFinished = true;

        while (notFinished) {
            TRACE << "Main is calling udt_receive()" << ENDL;
            network->udt_receive(datagram);
            DEBUG << "Received: " << toString(datagram) << ENDL;

            DEBUG << "Validating checksum" << ENDL;
            if (!validateChecksum(datagram)) {
                WARNING << "Invalid checksum." << ENDL;
            } else {
                if (datagram->seqNum == expectedSeqNum) {

                    //
                    // Datagram is valid.  We need to deliver the data, and update SEQ and ACK numbers.
                    //
                    DEBUG << "SeqNum matches expectedSeqNum (both are " << expectedSeqNum << ")" << ENDL;

                    if (datagram->payloadLength == 0) {
                        INFO << "Payload length is zero, indicating we have received all the data packet." << ENDL;
                        outputFile.close();
                        notFinished = false;
                    } else {
                        DEBUG << "Main is calling deliver_data(rcvpkt->data)" << ENDL;
                        deliver_data(outputFile, datagram->data, datagram->payloadLength);
                    }

                    sndpkt->ackNum = expectedSeqNum++;
                    sndpkt->checksum = computeChecksum(sndpkt);
                } else {
                    WARNING << "SeqNum does not match expectedSeqNum (expected " << expectedSeqNum << ", got " << datagram->seqNum << ")" << ENDL;
                }
            }


            TRACE << "Main is calling udt_send(sndpkt)" << ENDL;
            network->udt_send(sndpkt,!notFinished);
            TRACE << "Sent ACK back: " << toString(sndpkt) << ENDL;
        }

        delete datagram;
        delete network;
    } catch (std::exception &e) {
        FATAL<< "Error: " << e.what() << ENDL;
        return(1);
    }

    outputFile.close();
    return 0;
}
