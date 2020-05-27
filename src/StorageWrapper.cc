

#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"
#include "rocksdb/memtablerep.h"

#include "StorageWrapper.h"
#include "Scheduler.h"

StorageWrapper* StorageWrapper::instance;
StorageWrapper* StorageWrapper::GetInstance() {
    if (!instance) {
        instance = new StorageWrapper();
    }
    return instance;
}

StorageWrapper::~StorageWrapper() {
    if (scheduler_) {
        delete scheduler_;
        scheduler_ = nullptr;
    }
    if (db_) {
        delete db_;
        db_ = nullptr;
    }
    if (freader_) {
        delete freader_;
        freader_ = nullptr;
    }
}

bool StorageWrapper::Prepare2Ingest(const string& dbpath) {
    if (state_ != Init) {
        return false;
    }
    // open rocksdb
    rocksdb::Options options;
    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
    options.IncreaseParallelism(8);
    options.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options.create_if_missing = true;
    options.disable_auto_compactions = true;
    options.num_levels = 1;
    options.write_buffer_size = 256L * 1024L * 1024L;
    options.max_bytes_for_level_base = 1000L * 1000L * 1000L * 1000L;
    // open DB
    rocksdb::Status s = rocksdb::DB::Open(options, dbpath, &db_);
    if (!s.ok()) {
        fprintf(stderr, "[PrepareIngest] open db failed, reason=%s\n",
            s.ToString().c_str());
        return false;
    }
    woption_.disableWAL = true;

    state_ = Ingesting;
    return true;
}

void StorageWrapper::FinishIngest() {
    if (batch_.Count() > 0) {
        fprintf(stdout, "[FinishIngest] batch.size=%d\n", 
            batch_.Count());
        db_->Write(woption_, &batch_);
        batch_.Clear();
    }
    //db_->EnableAutoCompaction({db_->DefaultColumnFamily()});
    db_->Flush(rocksdb::FlushOptions());
    rocksdb::CompactRangeOptions compactionOption;
    compactionOption.change_level = true;
    compactionOption.target_level = 1;
    db_->CompactRange(compactionOption, nullptr, nullptr);

    delete db_;
    db_ = nullptr;
    state_ = Init;
}

bool StorageWrapper::StartServ(const string& dbpath, const string& fpath) {
    assert(state_ == Init);

    // open rocksdb
    rocksdb::Options options;
    options.disable_auto_compactions = true;
    size_t cap = 1024L * 1024L * 1024L;
    int num_shard_bits = 16;
    std::shared_ptr<rocksdb::Cache> cache = rocksdb::NewLRUCache(cap, num_shard_bits);
    rocksdb::BlockBasedTableOptions table_options;
    table_options.block_cache = cache;
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    // TBD: OpenForReadOnly is preferrable; however, it may cause mmap failed...
    rocksdb::Status s = rocksdb::DB::Open(options, dbpath, &db_);
    if (!s.ok()) {
        fprintf(stderr, "[StartServ] Open db failed, err=%s\n", s.ToString().c_str());
        return false;
    }
    
    // open file reader
    freader_ = new FileReader();
    if (!freader_->Open(fpath)) {
        fprintf(stderr, "[StartServ] Open file failed\n");
        return false;
    }
    fprintf(stdout, "[StartServ] succ\n");

    // start scheduler
    scheduler_ = new Scheduler();
    scheduler_->Start();

    // done
    state_ = ServQuery;
    return true;
}

bool StorageWrapper::Put(const string& key, const string& val) {
    if (state_ != Ingesting) {
        return false;
    }
    //fprintf(stdout, "[Put] key=%s, val=%s\n", key.c_str(), val.c_str());
    batch_.Put(key, val);
    if (batch_.Count() > kThreshold) {
        db_->Write(woption_, &batch_);
        batch_.Clear();
    }
    return true;
}

bool StorageWrapper::Get(const string& key, string& val) {
    if (state_ != ServQuery) {
        return false;
    }
    // Get offset from db
    //fprintf(stdout, "[Get] key=%s\n", key.c_str());
    string soffset;
    rocksdb::Status s = db_->Get(roption_, key, &soffset);
    if (!s.ok()) {
        fprintf(stderr, "get failed, errno=%s\n", s.ToString().c_str());
        return false;
    }
    long offset = std::stol(soffset);
    if (!scheduler_->GetTick(offset)) {
        fprintf(stdout, "[Get] ratelimit exceed\n");
        return false;
    }
    // Get value from file
    if (!freader_->Read(offset, val)) {
        return false;
    }
    return true;
}

