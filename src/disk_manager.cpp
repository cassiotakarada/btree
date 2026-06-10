#include "disk_manager.hpp"
#include <stdexcept>
#include <cstring>

DiskManager::DiskManager(const std::string& path) {
    file_.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        file_.open(path, std::ios::in | std::ios::out |
                         std::ios::binary | std::ios::trunc);
        if (!file_.is_open())
            throw std::runtime_error("Cannot open B-tree file: " + path);

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
    if (reuse_ && h.free_head != -1) {
        rrn = h.free_head;
        BNode freed = readNode(rrn);
        h.free_head = freed.K[1];
    } else {
        h.total += 1;
        rrn = h.total;
    }
    writeHeader(h);
    return rrn;
}

void DiskManager::freeNode(int rrn) {
    Header h = readHeader();
    BNode freed{};
    freed.n    = -1;
    freed.K[1] = h.free_head;
    writeNode(rrn, freed);
    h.free_head = rrn;
    writeHeader(h);
}

std::streamoff DiskManager::fileSizeBytes() {
    file_.clear();
    file_.seekg(0, std::ios::end);
    return file_.tellg();
}

int DiskManager::totalNodes() {
    return readHeader().total;
}

int DiskManager::freeNodes() {
    int cur = readHeader().free_head;
    int count = 0;
    while (cur != -1) {
        char buf[PAGE_SIZE];
        std::memset(buf, 0, PAGE_SIZE);
        file_.clear();
        file_.seekg(nodeOffset(cur));
        file_.read(buf, PAGE_SIZE);
        int n, next;
        std::memcpy(&n,    buf, sizeof(int));
        std::memcpy(&next, buf + sizeof(int) * (1 + M + 1), sizeof(int));
        if (n != -1) break;
        cur = next;
        if (++count > readHeader().total + 1) break;
    }
    return count;
}
