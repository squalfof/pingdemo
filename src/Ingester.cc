
#include "rocksdb/options.h"

#include "Ingester.h"

// For simplicity, use 'string' to represent size, instead of uint64
// key_size:uint64, key:binary,
// val_size:uint64, val:binary
const char* key_sz_tag = "key_size:";
const char* key_tag    = "key:";
const char* val_sz_tag = "val_size:";
const char* val_tag    = "val:";

char* parseKey(char* addr, string& key) {
    // key len
    char* pch = strstr(addr, key_sz_tag);
    if (!pch) {
        return nullptr;
    }
    addr = pch + strlen(key_sz_tag);
    int key_sz = atoi(addr);
    // key
    pch = strstr(addr, key_tag);
    if (!pch) {
        return nullptr;
    }
    addr = pch + strlen(key_tag);
    key = string(addr, key_sz);
    return addr + key_sz;
}

char* parseVal(char* addr, string& val) {
    // value len
    char* pch = strstr(addr, val_sz_tag);
    if (!pch) {
        return nullptr;
    }
    addr = pch + strlen(val_sz_tag);
    int val_sz = atoi(addr);
    // key
    pch = strstr(addr, val_tag);
    if (!pch) {
        return nullptr;
    }
    addr = pch + strlen(val_tag);
    val = string(addr, val_sz);
    return addr + val_sz;
}


bool Ingester::CreateIndex(const string& fpath, const string& dbpath) {
    Ingester ingester;
    ingester.Prepare2Ingest(dbpath);
    
    // open file, use mmap
    int fd = open(fpath.c_str(), O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "open fail, input=%s\n", fpath.c_str());
        return false;
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1) { // To obtain file size
        fprintf(stderr, "stat fail\n");
        return false;
    }
    const int len = sb.st_size;
    char* addr = (char*)mmap(NULL, len, PROT_READ|PROT_WRITE,
                            MAP_SHARED, fd, 0);
    int enumber = errno;
    if ((void*)addr == (void*)-1) {
        fprintf(stderr, "mmap failed, errno=%s\n", strerror(enumber));
        return false;
    }

    char* origin = addr;
    int overwrite_pos = 0;
    while (true) {
        string key, val;
        if ((addr = parseKey(addr, key)) == nullptr) {
            break;
        }
        if ((addr = parseVal(addr, val)) == nullptr) {
            break;
        }
        ingester.Put(key, to_string(overwrite_pos));
        // TBD: bigendian
        // inc overwrite_offset
        int vsize = val.size();
        memmove(reinterpret_cast<void*>(origin+overwrite_pos), 
            reinterpret_cast<const void*>(&vsize), sizeof(int));
        overwrite_pos += sizeof(int);        
        memmove(reinterpret_cast<void*>(origin+overwrite_pos), 
            reinterpret_cast<const void*>(val.data()), val.size());
        overwrite_pos += val.size();
    }
    ingester.FinishIngest();
    
    munmap((void*)origin, sb.st_size);
    ftruncate(fd, overwrite_pos);
    close(fd);

    return true;
}

bool Ingester::Prepare2Ingest(const string& dbpath) {
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

    return true;
}

bool Ingester::Put(const string& key, const string& val) {
    //fprintf(stdout, "[Put] key=%s, val=%s\n", key.c_str(), val.c_str());
    batch_.Put(key, val);
    if (batch_.Count() > kThreshold) {
        db_->Write(woption_, &batch_);
        batch_.Clear();
    }
     return true;
 }

void Ingester::FinishIngest() {
    if (batch_.Count() > 0) {
        fprintf(stdout, "[FinishIngest] batch.size=%d\n", 
            batch_.Count());
        db_->Write(woption_, &batch_);
        batch_.Clear();
    }
    db_->Flush(rocksdb::FlushOptions());
    rocksdb::CompactRangeOptions compactionOption;
    compactionOption.change_level = true;
    compactionOption.target_level = 1;
    db_->CompactRange(compactionOption, nullptr, nullptr);

    delete db_;
    db_ = nullptr;
}