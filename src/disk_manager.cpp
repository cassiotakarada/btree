#include "disk_manager.hpp"
#include <stdexcept>
#include <cstring>

// Record 0 = Header (padded to PAGE_SIZE).
// Record r >= 1 = BNode at offset r * PAGE_SIZE.

DiskManager::DiskManager(const std::string& path) {
    // Try opening existing file first.
    file_.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        // Create new file.
        file_.open(path, std::ios::in | std::ios::out |
                         std::ios::binary | std::ios::trunc);
        if (!file_.is_open())
            throw std::runtime_error("Cannot open B-tree file: " + path);

        // Write blank header as record 0.
        Header h{0, 0, -1};
        writeHeader(h);
    }
}

DiskManager::~DiskManager() {
    if (file_.is_open()) file_.close();
}

std::streamoff DiskManager::nodeOffset(int rrn) const {
    return static_cast<std::streamoff>(rrn) * PAGE_SIZE;
}

Header DiskManager::readHeader() {
    char buf[PAGE_SIZE];
    std::memset(buf, 0, PAGE_SIZE);
    file_.clear();
    file_.seekg(nodeOffset(0));
    file_.read(buf, PAGE_SIZE);

    Header h{};
    std::memcpy(&h.root,      buf + 0 * sizeof(int), sizeof(int));
    std::memcpy(&h.total,     buf + 1 * sizeof(int), sizeof(int));
    std::memcpy(&h.free_head, buf + 2 * sizeof(int), sizeof(int));
    return h;
}

void DiskManager::writeHeader(const Header& h) {
    char buf[PAGE_SIZE];
    std::memset(buf, 0, PAGE_SIZE);
    std::memcpy(buf + 0 * sizeof(int), &h.root,      sizeof(int));
    std::memcpy(buf + 1 * sizeof(int), &h.total,     sizeof(int));
    std::memcpy(buf + 2 * sizeof(int), &h.free_head, sizeof(int));

    file_.clear();
    file_.seekp(nodeOffset(0));
    file_.write(buf, PAGE_SIZE);
    file_.flush();
}

BNode DiskManager::readNode(int rrn) {
    ++accesses_;
    char buf[PAGE_SIZE];
    std::memset(buf, 0, PAGE_SIZE);
    file_.clear();
    file_.seekg(nodeOffset(rrn));
    file_.read(buf, PAGE_SIZE);

    BNode node{};
    int off = 0;
    std::memcpy(&node.n, buf + off, sizeof(int)); off += sizeof(int);
    std::memcpy(node.A,  buf + off, sizeof(int) * M); off += sizeof(int) * M;
    std::memcpy(node.K,  buf + off, sizeof(int) * M);
    return node;
}

void DiskManager::writeNode(int rrn, const BNode& node) {
    ++accesses_;
    char buf[PAGE_SIZE];
    std::memset(buf, 0, PAGE_SIZE);

    int off = 0;
    std::memcpy(buf + off, &node.n, sizeof(int)); off += sizeof(int);
    std::memcpy(buf + off,  node.A, sizeof(int) * M); off += sizeof(int) * M;
    std::memcpy(buf + off,  node.K, sizeof(int) * M);

    file_.clear();
    file_.seekp(nodeOffset(rrn));
    file_.write(buf, PAGE_SIZE);
    file_.flush();
}

int DiskManager::allocNode() {
    Header h = readHeader();
    int rrn;
    if (h.free_head != -1) {
        rrn = h.free_head;
        // Read the recycled node to follow the free-list chain.
        // K[1] stores the next pointer (n == -1 marks free nodes).
        BNode freed = readNode(rrn);
        h.free_head = freed.K[1];  // K[1] = next free RRN (-1 if end)
    } else {
        h.total += 1;
        rrn = h.total;  // RRN 0 = header, so first node = 1
    }
    writeHeader(h);
    return rrn;
}

void DiskManager::freeNode(int rrn) {
    Header h = readHeader();
    // Mark node as free: n = -1, K[1] = old free_head.
    BNode freed{};
    freed.n    = -1;
    freed.K[1] = h.free_head;
    writeNode(rrn, freed);
    h.free_head = rrn;
    writeHeader(h);
}
