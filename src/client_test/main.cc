#include <iostream>
#include <thread>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "../thrift/gen-cpp/CacheService.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;


void DoGetTask(int idx) {
  
  boost::shared_ptr<TTransport> socket(new TSocket("localhost", 9091));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  CacheServiceClient client(protocol);

  try {
    transport->open();

    vector<string> keys = {"ab", "vtable_lost", "testa"};
    for (auto& k : keys) {
      GetRequest req;
      req.key = k;
      GetResponse resp;
      client.Get(resp, 100, req);
      printf("thid=%d, req-key=%s, resp-val=%s\n", 
          idx, req.key.c_str(), resp.val.c_str());
    }
    
    transport->close();
  } catch (TException& tx) {
    cout << "ERROR: " << tx.what() << endl;
  }
}

int main() {
  vector<std::thread> ths;
  const int n = 5;
  for (int i = 0; i < n; i++) {
    ths.push_back(std::thread(DoGetTask, i));
  }
  for (int i = 0; i < n; i++) {
    ths[i].join();
  }

  cout << "done" << endl;
  return 0;
}
