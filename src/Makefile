

CXX=g++
CXXFLAGS=-Wall -g -std=c++11
INC_PATH=./include
LIB_PATH=./lib

#INCS=-I./ -I${INC_PATH}/rocksdb
#LIBS=-L./ -L${LIB_PATH}/rocksdb
THRIFT_PATH=./thrift

OBJS := ServQuery.o Ingester.o CacheReader.o Scheduler.o 
THRIFT_OBJS := $(THRIFT_PATH)/CacheService.o $(THRIFT_PATH)/cache_constants.o $(THRIFT_PATH)/cache_types.o

$(OBJS): %.o : %.cc
	${CXX} ${CXXFLAGS} -c ${INCS} $< -o $@

$(THRIFT_OBJS): $(THRIFT_PATH)/%.o : $(THRIFT_PATH)/gen-cpp/%.cpp
	${CXX} ${CXXFLAGS} -c ${INCS} $< -o $@

all: server

server: main.o $(OBJS) $(THRIFT_OBJS)
	$(CXX) $(CXXFLAGS) -o ping-server main.o $(OBJS) $(THRIFT_OBJS) $(LIBS) -lrocksdb -lthrift -lthriftz -lthriftc

clean:
	rm -rf ping-server main.o $(OBJS)
