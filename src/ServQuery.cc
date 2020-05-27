
#include <thrift/concurrency/ThreadManager.h>
//#include <thrift/concurrency/ThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/TToString.h>

#include "thrift/gen-cpp/CacheService.h"

#include "basic.h"
#include "ServQuery.h"
#include "StorageWrapper.h"

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

class CacheServiceHandler : virtual public CacheServiceIf {
public:
  CacheServiceHandler() {
    // Your initialization goes here
  }

  void Get(GetResponse& _return, const int32_t logid, const GetRequest& req) {
    // Your implementation goes here
    fprintf(stdout, "Get request, key=%s\n", req.key.c_str());
    auto* storage = StorageWrapper::GetInstance();
    string val;
    if (!storage->Get(req.key, val)) {
      fprintf(stderr, "Get fail, key=%s\n", req.key.c_str());
      return;
    }
    fprintf(stdout, "Get request, key=%s, val=%s\n", 
      req.key.c_str(), val.c_str());
    _return.val = val;
    return;
  }
};

void StartServQuery() {
  int port = 9091;
  boost::shared_ptr<CacheServiceHandler> handler(new CacheServiceHandler());
  boost::shared_ptr<TProcessor> processor(new CacheServiceProcessor(handler));
  boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();

  // Fail to compile on Mac
  /*
  // using thread pool with maximum 150 threads to handle incoming requests
  boost::shared_ptr<ThreadManager> threadManager
        = ThreadManager::newSimpleThreadManager(150);
  boost::shared_ptr<PosixThreadFactory> threadFactory(new PosixThreadFactory());
  threadManager->threadFactory(threadFactory);
  threadManager->start();

  TNonblockingServer server(processor, protocolFactory, port, threadManager);
  server.serve();
  */
}

