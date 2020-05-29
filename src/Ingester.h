
#pragma once

#include "rocksdb/db.h"

#include "basic.h"

class Ingester {
public:
    static bool CreateIndex(const string& fpath, const string& dbpath);

private:
    const int kThreshold = 100;
    rocksdb::WriteOptions woption_;
    rocksdb::WriteBatch batch_;
    rocksdb::DB* db_ = nullptr;

private:
    Ingester() = default;

    bool Prepare2Ingest(const string& dbpath);
    bool Put(const string& key, const string& val);
    void FinishIngest();
};