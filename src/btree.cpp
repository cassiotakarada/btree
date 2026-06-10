#include "btree.hpp"
#include <iostream>
#include <fstream>
#include <climits>
#include <string>
#include <stdexcept>

BTree::BTree(DiskManager& dm) : dm_(dm) {}

std::tuple<int,int,bool> BTree::mSearchPath(int x, std::vector<int>& path) {
    Header hdr = dm_.readHeader();
    int p = hdr.root;
    int q = 0;
    int i = 0;

    while (p != 0) {
        path.push_back(q);
        BNode node = dm_.readNode(p);

        i = node.n;
        for (int j = 0; j <= node.n; ++j) {
            int kj  = (j == 0)         ? INT_MIN : node.K[j];
            int kj1 = (j == node.n)    ? INT_MAX : node.K[j + 1];
            if (kj <= x && x < kj1) { i = j; break; }
        }

        if (i > 0 && node.K[i] == x)
            return {p, i, true};

        q = p;
        p = node.A[i];
    }
    return {q, i, false};
}

std::tuple<int,int,bool> BTree::mSearch(int x) {
    std::vector<int> dummy;
    return mSearchPath(x, dummy);
}

void BTree::insertInNode(BNode& p, int k, int a) {
    int pos = p.n;
    while (pos >= 1 && p.K[pos] > k) {
        p.K[pos + 1] = p.K[pos];
        p.A[pos + 1] = p.A[pos];
        --pos;
    }
    p.K[pos + 1] = k;
    p.A[pos + 1] = a;
    p.n += 1;
}

int BTree::splitNode(int pRRN, BNode& p, int& mediana) {
    mediana = p.K[MID];

    BNode q{};
    q.A[0] = p.A[MID];
    int qi = 0;
    for (int j = MID + 1; j <= ORDER; ++j) {
        ++qi;
        q.K[qi] = p.K[j];
        q.A[qi] = p.A[j];
    }
    q.n = qi;

    p.n = MID - 1;
    for (int j = MID; j <= ORDER; ++j) {
        p.K[j] = 0;
        p.A[j] = 0;
    }

    int q_rrn = dm_.allocNode();
    dm_.writeNode(pRRN, p);
    dm_.writeNode(q_rrn, q);
    return q_rrn;
}

void BTree::insertB(int x) {
    std::vector<int> path;
    auto [p, i, achou] = mSearchPath(x, path);

    if (achou) return;

    int K = x;
    int A = 0;

    int parentIdx = static_cast<int>(path.size()) - 1;

    while (p != 0) {
        BNode node = dm_.readNode(p);
        insertInNode(node, K, A);

        if (node.n <= MAX_KEYS) {
            dm_.writeNode(p, node);
            return;
        }

        int mediana;
        int q = splitNode(p, node, mediana);
        K = mediana;
        A = q;
        p = (parentIdx >= 0) ? path[parentIdx--] : 0;
    }

    int T = dm_.readHeader().root;

    int r = dm_.allocNode();

    BNode no_r{};
    no_r.n    = 1;
    no_r.A[0] = T;
    no_r.K[1] = K;
    no_r.A[1] = A;
    dm_.writeNode(r, no_r);

    Header hdr = dm_.readHeader();
    hdr.root = r;
    dm_.writeHeader(hdr);
}

void BTree::removeFromNode(BNode& p, int i) {
    for (int j = i; j < p.n; ++j) {
        p.K[j] = p.K[j + 1];
        p.A[j] = p.A[j + 1];
    }
    p.K[p.n] = 0;
    p.A[p.n] = 0;
    p.n -= 1;
}

int BTree::findSuccessorLeaf(int childRRN, std::vector<int>& path) {
    int q = childRRN;
    while (true) {
        BNode node = dm_.readNode(q);
        if (node.A[0] == 0) return q;
        path.push_back(q);
        q = node.A[0];
    }
}

void BTree::deleteB(int x) {
    std::vector<int> path;
    auto [p, i, achou] = mSearchPath(x, path);

    if (!achou) return;

    Header hdr = dm_.readHeader();
    int raiz = hdr.root;

    BNode pNode = dm_.readNode(p);

    if (pNode.A[0] != 0) {
        path.push_back(p);
        int q = findSuccessorLeaf(pNode.A[i], path);

        BNode q_no = dm_.readNode(q);
        pNode.K[i] = q_no.K[1];
        dm_.writeNode(p, pNode);

        p     = q;
        i     = 1;
        pNode = q_no;
    }

    removeFromNode(pNode, i);
    int n = pNode.n;

    int minKeys = MID - 1;

    while (n < minKeys && p != raiz) {
        int parentRRN = path.back();
        path.pop_back();
        BNode z = dm_.readNode(parentRRN);

        int j = -1;
        for (int k = 0; k <= z.n; ++k) {
            if (z.A[k] == p) { j = k; break; }
        }

        bool hasRight = (j < z.n);

        if (hasRight) {
            int yRRN  = z.A[j + 1];
            BNode y   = dm_.readNode(yRRN);

            if (y.n >= MID) {
                pNode.n += 1;
                pNode.K[pNode.n] = z.K[j + 1];
                pNode.A[pNode.n] = y.A[0];

                z.K[j + 1] = y.K[1];
                y.A[0] = y.A[1];
                removeFromNode(y, 1);

                dm_.writeNode(p,         pNode);
                dm_.writeNode(parentRRN, z);
                dm_.writeNode(yRRN,      y);
                return;
            }

            pNode.n += 1;
            pNode.K[pNode.n] = z.K[j + 1];
            pNode.A[pNode.n] = y.A[0];
            for (int k = 1; k <= y.n; ++k) {
                pNode.n += 1;
                pNode.K[pNode.n] = y.K[k];
                pNode.A[pNode.n] = y.A[k];
            }
            dm_.writeNode(p, pNode);
            dm_.freeNode(yRRN);

            removeFromNode(z, j + 1);
            n = z.n;
            p = parentRRN;
            pNode = z;

        } else {
            int yRRN  = z.A[j - 1];
            BNode y   = dm_.readNode(yRRN);

            if (y.n >= MID) {
                for (int k = pNode.n; k >= 1; --k) {
                    pNode.K[k + 1] = pNode.K[k];
                    pNode.A[k + 1] = pNode.A[k];
                }
                pNode.A[1] = pNode.A[0];
                pNode.K[1] = z.K[j];
                pNode.A[0] = y.A[y.n];
                pNode.n += 1;

                z.K[j] = y.K[y.n];
                y.K[y.n] = 0;
                y.A[y.n] = 0;
                y.n -= 1;

                dm_.writeNode(p,         pNode);
                dm_.writeNode(parentRRN, z);
                dm_.writeNode(yRRN,      y);
                return;
            }

            y.n += 1;
            y.K[y.n] = z.K[j];
            y.A[y.n] = pNode.A[0];
            for (int k = 1; k <= pNode.n; ++k) {
                y.n += 1;
                y.K[y.n] = pNode.K[k];
                y.A[y.n] = pNode.A[k];
            }
            dm_.writeNode(yRRN, y);
            dm_.freeNode(p);

            removeFromNode(z, j);
            n = z.n;
            p = parentRRN;
            pNode = z;
        }
    }

    if (p != raiz) {
        dm_.writeNode(p, pNode);
        return;
    }

    if (pNode.n == 0) {
        int newRoot = pNode.A[0];
        dm_.freeNode(p);
        hdr = dm_.readHeader();
        hdr.root = newRoot;
        dm_.writeHeader(hdr);
    } else {
        dm_.writeNode(p, pNode);
    }
}

void BTree::printNode(int rrn, int depth) {
    if (rrn == 0) return;
    BNode node = dm_.readNode(rrn);
    if (node.n <= 0) return;

    std::string indent(static_cast<size_t>(depth) * 4, ' ');
    std::cout << indent << "[RRN " << rrn << "] (n=" << node.n << ") keys:";
    for (int i = 1; i <= node.n; ++i)
        std::cout << " " << node.K[i];
    std::cout << "\n";

    for (int i = 0; i <= node.n; ++i)
        printNode(node.A[i], depth + 1);
}

void BTree::printTree() {
    Header hdr = dm_.readHeader();
    if (hdr.root == 0) {
        std::cout << "(empty tree)\n";
        return;
    }
    printNode(hdr.root, 0);
}

int BTree::height() {
    Header hdr = dm_.readHeader();
    int p = hdr.root;
    int h = 0;
    while (p != 0) {
        ++h;
        BNode node = dm_.readNode(p);
        p = node.A[0];
    }
    return h;
}

void BTree::emitDotNode(int rrn, std::ostream& os) {
    if (rrn == 0) return;
    BNode node = dm_.readNode(rrn);
    if (node.n <= 0) return;

    os << "  n" << rrn << " [label=\"{ RRN " << rrn << " | { ";
    for (int i = 0; i <= node.n; ++i) {
        if (i > 0) os << " | ";
        os << "<a" << i << "> ";
        if (i >= 1) os << node.K[i];
    }
    os << " } }\"];\n";

    for (int i = 0; i <= node.n; ++i) {
        if (node.A[i] != 0) {
            os << "  n" << rrn << ":a" << i
               << " -> n" << node.A[i] << ";\n";
        }
    }

    for (int i = 0; i <= node.n; ++i)
        emitDotNode(node.A[i], os);
}

void BTree::exportDot(const std::string& path) {
    std::ofstream os(path);
    if (!os) throw std::runtime_error("Não consegui abrir arquivo: " + path);

    os << "digraph BTree {\n"
       << "  graph [rankdir=TB, splines=line, nodesep=0.25, ranksep=0.6];\n"
       << "  node  [shape=record, fontname=\"Helvetica\", fontsize=10];\n"
       << "  edge  [arrowsize=0.6];\n";

    Header hdr = dm_.readHeader();
    if (hdr.root == 0) {
        os << "  empty [label=\"(árvore vazia)\"];\n";
    } else {
        emitDotNode(hdr.root, os);
    }
    os << "}\n";
}
