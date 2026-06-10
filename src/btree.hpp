#pragma once
#include <string>
#include <tuple>
#include <vector>
#include "disk_manager.hpp"

class BTree {
public:
    explicit BTree(DiskManager& dm);

    std::tuple<int,int,bool> mSearch(int x);

    void insertB(int x);

    void deleteB(int x);

    void printTree();

    int  height();

    void exportDot(const std::string& path);

    int  diskAccesses() const { return dm_.diskAccesses(); }
    void resetCounter()       { dm_.resetCounter(); }

private:
    DiskManager& dm_;

    std::tuple<int,int,bool> mSearchPath(int x, std::vector<int>& path);

    void insertInNode(BNode& p, int k, int a);

    int splitNode(int pRRN, BNode& p, int& mediana);

    void removeFromNode(BNode& p, int i);

    int findSuccessorLeaf(int childRRN, std::vector<int>& path);

    void printNode(int rrn, int depth);

    void emitDotNode(int rrn, std::ostream& os);
};
