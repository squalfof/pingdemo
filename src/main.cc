
#include "basic.h"
#include "ServQuery.h"
#include "StorageWrapper.h"

#define handle_error(msg) \
           do { printf(msg); exit(EXIT_FAILURE); } while (0)

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


const string kDBPath = "./data/db";
const string kInputPath = "./data/input.txt";

bool createIndex(const string& fpath, const string& dbpath) {
    StorageWrapper* storage = StorageWrapper::GetInstance();
    storage->Prepare2Ingest(kDBPath);
    
    // open file, use mmap
    int fd = open(fpath.c_str(), O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "open fail, input=%s\n", kInputPath.c_str());
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
        storage->Put(key, to_string(overwrite_pos));
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
    storage->FinishIngest();
    
    munmap((void*)origin, sb.st_size);
    ftruncate(fd, overwrite_pos);
    close(fd);

    return true;
}

int serv() {
    StorageWrapper* storage = StorageWrapper::GetInstance();
    if (storage->StartServ(kDBPath, kInputPath)) {
        StartServQuery();
    }
    return 0;
}

int main() {
    cout << "welcome onboard" << endl;
    createIndex(kInputPath, kDBPath);
    cout << "create index done\n";
    cout << "start serv query..." << endl;
    serv();
    cout << "done" << endl;
    return 0;
}