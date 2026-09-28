#
# Makefile
# Computer Networking Programing Assignments
#
#  Created by Phillip Romig on 4/3/12.
#  Copyright 2012 Colorado School of Mines. All rights reserved.
#
CXX = g++
LD = g++
CXXFLAGS = -std=c++17 -g
LDFLAGS = 

#
# You should be able to add object files here without changing anything else
#
TARGET = rft-server
OBJ_FILES = datagram.o networkLayer.o rft-server.o
INC_FILES = datagram.h logging.h networkLayer.h

#
# Any libraries we might need.
#
LIBRARYS = 

${TARGET}: ${OBJ_FILES}
	${LD} ${LDFLAGS} ${OBJ_FILES} -o $@ ${LIBRARYS}

%.o : %.cpp ${INC_FILES}
	${CXX} -c ${CXXFLAGS} -o $@ $<

#
# Please remember not to submit objects or binarys.
#
clean:
	rm -f core ${TARGET} ${OBJ_FILES}
