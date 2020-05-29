
#pragma once

#include "rocksdb/db.h"

#include "basic.h"

class Scheduler;
class FileReader;
class CacheReader {
private:
    static CacheReader* instance;
    
    Scheduler* scheduler_ = nullptr;
    rocksdb::DB* db_ = nullptr;
    FileReader* freader_ = nullptr;
    rocksdb::ReadOptions roption_;
    
public:
    static CacheReader* GetInstance();
    ~CacheReader();

    bool StartServ(const string& dbpath, const string& fpath);
    bool Get(const string& key, string& val);

private:
    CacheReader() = default;
};