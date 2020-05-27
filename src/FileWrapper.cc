
#include "FileWrapper.h"

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