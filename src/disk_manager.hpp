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

    // Allocates a new RRN (from free list or by appending).
    // Does NOT write to disk — caller must writeNode afterwards.
    int    allocNode();
    // Pushes rrn onto the free list and persists the header.
    void   freeNode (int rrn);

    Header readHeader ();
    void   writeHeader(const Header& h);

    int  diskAccesses() const { return accesses_; }
    void resetCounter()       { accesses_ = 0; }

private:
    std::fstream file_;
    int          accesses_{0};

    std::streamoff nodeOffset(int rrn) const;
};
