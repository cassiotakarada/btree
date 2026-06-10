#pragma once
#include <fstream>
#include <string>
#include "types.hpp"

class DiskManager {
public:
    explicit DiskManager(const std::string& path);
    ~DiskManager();

    BNode  readNode (int rrn);
    void   writeNode(int rrn, const BNode& node);

    int    allocNode();
    void   freeNode (int rrn);

    Header readHeader ();
    void   writeHeader(const Header& h);

    int  diskAccesses() const { return accesses_; }
    void resetCounter()       { accesses_ = 0; }

    void setReuse(bool on) { reuse_ = on; }
    bool reuse() const     { return reuse_; }

    std::streamoff fileSizeBytes();

    int totalNodes();
    int freeNodes();

private:
    std::fstream file_;
    int          accesses_{0};
    bool         reuse_{true};

    std::streamoff nodeOffset(int rrn) const;
};
