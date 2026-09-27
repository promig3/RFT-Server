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

void printInstructions() {
  std::cout << "Avalable falgs:" << std::endl;
  std::cout << "\t-f output-filename\t// Reqquired: Will be created or overwritten." << std::endl;
  std::cout << "\t-p port-number\t// UDP port to use. On Isengard must be between 12,000 and 13,000. Defaults to 12345." << std::endl;
  std::cout << "\t-l loss-rate\t// Probibility (0.0 - 1.0) a packet will be lost." << std::endl;
  std::cout << "\t-c corruption-rate\t// Probibility(0.0 - 1.0) a packet will be corrupted." << std::endl;
  std::cout << "\t-t delay\t// Delay between packets milliseconds. Can be used to slow down network to aid in debugging." << std::endl;
  std::cout << "\t-d vebosity\t// How verbose (0-6) logging messages should be.\n" << std::endl;
  return;
}

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
    unsigned int delay(100);

    int opt;
    try {
        while ((opt = getopt(argc, argv, "f:p:d:l:c:t:")) != -1) {
            switch (opt) {
                case 't':
                    delay = std::stoi(optarg);
                    break;
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
		  printInstructions();
                    break;
            }
        }
    } catch (std::exception &e) {
        FATAL << "Invalid command line arguments: " << e.what() << ENDL;
	printInstructions();
        return(1);
    }

    if (outputFileName == "") {
       FATAL << "Invalid command line arguments: output filename is required."  << ENDL;
       printInstructions();
        return(1);
    }
   
    INFO << "Command line arguments parsed." << ENDL;
    INFO << "\tOutput file name: " << outputFileName << ENDL;
    INFO << "\tPort number: " << portNum << ENDL;
    INFO << "\tDebug Level: " << LOG_LEVEL << ENDL;
    INFO << "\tLoss Rate: " << lossRate << ENDL;
    INFO << "\tCorruption Rate: " << corruptionRate << ENDL;
    INFO << "\tPacket delay (milliseconds)" << delay << ENDL;
  

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
      auto *network = new networkLayerC(portNum, lossRate, corruptionRate,delay);
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

        // Wait to see if we get a duplicate last datagram, which would indicate that the last
        // ACK was lost by the network. The file is closed, so we send the data to the application, 
        // rather we just kee sending the lask ACK until nothing is recieved for 1 second.
        // Code written with the help of Claude AI
        while (network->dataAvalable(1)) {
            network->udt_receive(datagram);
            if (validateChecksum(datagram) && (datagram->payloadLength == 0))
                network->udt_send(sndpkt,true);
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
