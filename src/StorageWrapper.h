
#pragma once

#include "rocksdb/db.h"

#include "basic.h"
#include "FileWrapper.h"

class Scheduler;
class StorageWrapper {
private:
    enum State {
        Init      = 0,
        Ingesting = 1,
        ServQuery = 2
    };
    State state_ = Init;
    Scheduler* scheduler_ = nullptr;
    rocksdb::DB* db_ = nullptr;
    FileReader* freader_ = nullptr;
    static StorageWrapper* instance;

    const int kThreshold = 100;
    rocksdb::ReadOptions roption_;
    rocksdb::WriteOptions woption_;
    rocksdb::WriteBatch batch_;
    
public:
    static StorageWrapper* GetInstance();
    ~StorageWrapper();

    bool Prepare2Ingest(const string& dbpath);
    void FinishIngest();
    bool StartServ(const string& dbpath, const string& fpath);

    bool Put(const string& key, const string& val);
    bool Get(const string& key, string& val);

private:
    StorageWrapper() = default;
};