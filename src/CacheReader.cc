

#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"
#include "rocksdb/memtablerep.h"

#include "CacheReader.h"
#include "Scheduler.h"

class FileReader {
private:
    char* origin_ = nullptr;
    long filesz_ = 0;

public:
    FileReader() = default;
    ~FileReader() {
        if (origin_) {
            munmap((void*)origin_, filesz_);
        }
        origin_ = nullptr;
    }

    bool Open(const string& fpath);
    bool Read(long offset, string& val);
};

bool FileReader::Open(const string& fpath) {
    fprintf(stdout, "[Reader] fpath=%s\n", fpath.c_str());
    int fd = open(fpath.c_str(), O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "[Open] open fail, input=%s\n", fpath.c_str());
        return false;
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1) { /* To obtain file size */
        fprintf(stderr, "[Open] stat fail\n");
        return false;
    }
    filesz_ = sb.st_size;
    char* addr = (char*)mmap(NULL, sb.st_size, PROT_READ,
                            MAP_SHARED, fd, 0);
    int enumber = errno;
    if (addr == (void*)-1 || enumber != 0) {
        fprintf(stderr, "[FileReader] mmap failed, errno=%d, reason=%s\n", 
            enumber, strerror(enumber));
        return false;
    }
    origin_ = addr;
    close(fd);
    return true;
}

bool FileReader::Read(long offset, string& val) {
    assert(origin_);
    assert(offset+4 < filesz_);

    char* addr = origin_ + offset;
    int val_len = *reinterpret_cast<int*>(addr);
    fprintf(stdout, "[Read] val-len=%d\n", val_len);
    if (offset + 4 + val_len > filesz_) {
        return false;
    }
    addr += sizeof(int);
    val = string(reinterpret_cast<char*>(addr), val_len);
    return true;
}



CacheReader* CacheReader::instance;
CacheReader* CacheReader::GetInstance() {
    if (!instance) {
        instance = new CacheReader();
    }
    return instance;
}

CacheReader::~CacheReader() {
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

bool CacheReader::StartServ(const string& dbpath, const string& fpath) {
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

    return true;
}

bool CacheReader::Get(const string& key, string& val) {
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

