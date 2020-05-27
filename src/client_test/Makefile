

CXX=g++
CXXFLAGS=-Wall -g -std=c++11
INC_PATH=./include
LIB_PATH=./lib

#INCS=-I./ -I${INC_PATH}/rocksdb
#LIBS=-L./ -L${LIB_PATH}/rocksdb
THRIFT_PATH=../thrift

THRIFT_OBJS := $(THRIFT_PATH)/CacheService.o $(THRIFT_PATH)/cache_constants.o $(THRIFT_PATH)/cache_types.o

#$(THRIFT_OBJS): $(THRIFT_PATH)/%.o : $(THRIFT_PATH)/gen-cpp/%.cpp
#	${CXX} ${CXXFLAGS} -c ${INCS} $< -o $@

all: client

client: main.o $(THRIFT_OBJS)
	$(CXX) $(CXXFLAGS) -o client main.o $(THRIFT_OBJS) $(LIBS) -lthrift

clean:
	rm -rf client main.o
