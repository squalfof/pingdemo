
#pragma once

#include "basic.h"

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



