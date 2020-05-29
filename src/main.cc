
#include "basic.h"

#include "Ingester.h"
#include "ServQuery.h"
#include "CacheReader.h"

//#define handle_error(msg) \
  //         do { printf(msg); exit(EXIT_FAILURE); } while (0)




const string kDBPath = "./data/db";
const string kInputPath = "./data/input.txt";

bool ingest() {
    return Ingester::CreateIndex(kInputPath, kDBPath);
}

bool serv() {
    CacheReader* storage = CacheReader::GetInstance();
    if (storage->StartServ(kDBPath, kInputPath)) {
        StartServQuery();
    }
    return true;
}

int main() {
    cout << "welcome onboard" << endl;
    if (!ingest()) {
        cout << "fail to ingest" << endl;
        return -1;
    }
    cout << "create index done\n";
    cout << "start serv query..." << endl;
    serv();
    cout << "done" << endl;
    return 0;
}