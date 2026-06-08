#include "btree.hpp"
#include <iostream>
#include <fstream>
#include <climits>
#include <string>
#include <stdexcept>

BTree::BTree(DiskManager& dm) : dm_(dm) {}

// =============================================================================
// BUSCA (mSearch / mSearchPath) — busca m-way na árvore B
// =============================================================================
// Ideia: em cada nó, as chaves ordenadas K[1..n] particionam o universo em faixas.
// O índice i indica a faixa onde x cai: K[i] <= x < K[i+1] (com K[0] = -∞ e K[n+1] = +∞
// tratados via INT_MIN/INT_MAX). O filho A[i] é a raiz da subárvore dessa faixa.
// Desce-se repetindo: lê o nó, acha i, se K[i]==x retornou; senão p = A[i] até folha.
// path guarda, a cada nível, o RRN do *pai* do nó atual (q antes de atualizar p),
// para insertB/deleteB subirem depois pela árvore sem nova busca.
// =============================================================================
std::tuple<int,int,bool> BTree::mSearchPath(int x, std::vector<int>& path) {
    Header hdr = dm_.readHeader();
    int p = hdr.root;
    int q = 0;
    int i = 0;

    while (p != 0) {
        path.push_back(q);          // pai do nó p neste nível (0 = sem pai, ex.: acima da raiz)
        BNode node = dm_.readNode(p);

        // Determina i: posição da "gaveta" onde x se encaixa entre chaves consecutivas
        i = node.n;
        for (int j = 0; j <= node.n; ++j) {
            int kj  = (j == 0)         ? INT_MIN : node.K[j];
            int kj1 = (j == node.n)    ? INT_MAX : node.K[j + 1];
            if (kj <= x && x < kj1) { i = j; break; }
        }

        // Chave encontrada neste nó (só faz sentido comparar K[i] quando i >= 1)
        if (i > 0 && node.K[i] == x)
            return {p, i, true};

        q = p;
        p = node.A[i];              // desce na subárvore da faixa i
    }
    // Árvore vazia (p nunca entrou no while) ou x não existe: último pai q e slot i
    return {q, i, false};
}

std::tuple<int,int,bool> BTree::mSearch(int x) {
    std::vector<int> dummy;
    return mSearchPath(x, dummy);
}

// Insere chave k com filho à direita a (A[i] aponta para subárvore com chaves > K[i]).
// Desloca chaves/ponteiros para manter K[1..n] ordenado (inserção local no vetor do nó).
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

// SPLIT (Equação 1 do enunciado): o nó ficou com n = m chaves após insertInNode (overflow).
// Parte em dois nós irmãos: p fica com as ⌈m/2⌉−1 menores, q com as maiores,
// e K[MID] é a mediana promovida ao pai (retornada em mediana). A[0] de q recebe
// o filho que ficava à direita da mediana em p.
int BTree::splitNode(int pRRN, BNode& p, int& mediana) {
    mediana = p.K[MID];

    // Após insertInNode no nó cheio, n == ORDER (m chaves temporárias até o split)
    BNode q{};
    q.A[0] = p.A[MID];
    int qi = 0;
    for (int j = MID + 1; j <= ORDER; ++j) {
        ++qi;
        q.K[qi] = p.K[j];
        q.A[qi] = p.A[j];
    }
    q.n = qi;

    // Trunca p: fica só com chaves à esquerda da mediana
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

// =============================================================================
// INSERÇÃO (insertB) — bottom-up com propagação de split
// =============================================================================
// 1) mSearchPath localiza onde x entraria: se já existe, sai; senão p é a folha
//    (ou p==0 se árvore vazia) e path[] lista os pais do caminho (path[0]=0).
// 2) Insere (K=x, A=0) na folha. Se nó não estourar o máximo de chaves, grava e fim.
// 3) Se estourar: splitNode divide o nó; a mediana vira nova chave K a subir e A aponta
//    para o irmão direito novo. Sobe para o pai (path[parentIdx]) e repete.
// 4) Se subir além da raiz (p==0), cria nova raiz com uma chave e dois filhos
//    (árvore cresce em altura). allocNode antes de reler o cabeçalho evita RRN/total obsoletos.
// =============================================================================
void BTree::insertB(int x) {
    std::vector<int> path;
    auto [p, i, achou] = mSearchPath(x, path);

    if (achou) return;   // duplicata: árvore B típica não reinsere a mesma chave

    int K = x;
    int A = 0;

    // parentIdx: índice em path do pai imediato do nó p que estamos tratando
    int parentIdx = static_cast<int>(path.size()) - 1;

    while (p != 0) {
        BNode node = dm_.readNode(p);
        insertInNode(node, K, A);

        if (node.n <= MAX_KEYS) {
            dm_.writeNode(p, node);
            return;
        }

        // Overflow: divide e prepara (K,A) para o nível acima
        int mediana;
        int q = splitNode(p, node, mediana);
        K = mediana;
        A = q;
        p = (parentIdx >= 0) ? path[parentIdx--] : 0;
    }

    // Novo nível na raiz
    int T = dm_.readHeader().root;

    int r = dm_.allocNode();  // atualiza total no disco

    BNode no_r{};
    no_r.n    = 1;
    no_r.A[0] = T;
    no_r.K[1] = K;
    no_r.A[1] = A;
    dm_.writeNode(r, no_r);

    Header hdr = dm_.readHeader();  // cabeçalho fresco após allocNode
    hdr.root = r;
    dm_.writeHeader(hdr);
}

// Compacta K e A após remover a posição i (remove K[i] e o ponteiro A[i] à sua direita).
void BTree::removeFromNode(BNode& p, int i) {
    for (int j = i; j < p.n; ++j) {
        p.K[j] = p.K[j + 1];
        p.A[j] = p.A[j + 1];
    }
    p.K[p.n] = 0;
    p.A[p.n] = 0;
    p.n -= 1;
}

// Sucessor in-order na subárvore A[i]: menor chave à direita = folha mais à esquerda.
// path acumula os nós internos visitados para depois remendar underflow subindo.
int BTree::findSuccessorLeaf(int childRRN, std::vector<int>& path) {
    int q = childRRN;
    while (true) {
        BNode node = dm_.readNode(q);
        if (node.A[0] == 0) return q;
        path.push_back(q);
        q = node.A[0];
    }
}

// =============================================================================
// REMOÇÃO (deleteB)
// =============================================================================
// Fase A — localizar: mSearchPath; se não achou, retorna.
// Fase B — se o nó não é folha (A[0] != 0): não dá para "apagar buraco" no meio
//    da árvore sem quebrar ordem; troca-se K[i] pelo sucessor in-order (menor chave
//    na subárvore direita A[i]), que sempre está numa folha. A remoção efetiva passa
//    a ser nessa folha (sempre removemos K[1] após copiar o sucessor).
// Fase C — removeFromNode na folha; se n >= minKeys (⌈m/2⌉−1), balanceado: grava e sai.
// Fase D — underflow (n < minKeys): enquanto não for raiz, tenta emprestar chave do
//    irmão (redistribuição) ou funde com irmão + chave separadora do pai (merge).
//    Isso pode reduzir o pai; sobe com p = pai e repete. Na raiz: se n==0 e não é
//    folha única, desce altura promovendo o único filho.
// =============================================================================
void BTree::deleteB(int x) {
    std::vector<int> path;
    auto [p, i, achou] = mSearchPath(x, path);

    if (!achou) return;

    Header hdr = dm_.readHeader();
    int raiz = hdr.root;

    BNode pNode = dm_.readNode(p);

    if (pNode.A[0] != 0) {
        // Nó interno: substitui K[i] pelo sucessor in-order (folha mais à esquerda
        // da subárvore A[i]) e remove o sucessor lá. Para o reparo de underflow
        // funcionar precisamos que `path` continue ordenado da raiz até a folha:
        //   1) p é o pai imediato de A[i] (nível abaixo) → empilhe primeiro;
        //   2) findSuccessorLeaf empilha os internos do caminho até a folha.
        // (Empilhar na ordem inversa fazia o loop de underflow olhar pro avô em
        //  vez do pai, e o `j` da busca por irmão acabava em -1, corrompendo o nó.)
        path.push_back(p);
        int q = findSuccessorLeaf(pNode.A[i], path);

        BNode q_no = dm_.readNode(q);
        pNode.K[i] = q_no.K[1];
        dm_.writeNode(p, pNode);

        p     = q;
        i     = 1;            // na folha do sucessor, a menor chave é K[1]
        pNode = q_no;
    }

    removeFromNode(pNode, i);
    int n = pNode.n;

    // Mínimo de chaves permitido em nó que não é raiz (invariante da B)
    int minKeys = MID - 1;

    // Repara underflow subindo pelo path até a raiz ou conseguir emprestar/merge estável
    while (n < minKeys && p != raiz) {
        int parentRRN = path.back();
        path.pop_back();
        BNode z = dm_.readNode(parentRRN);

        // j: posição em z tal que z.A[j] == p (p é o j-ésimo filho do pai)
        int j = -1;
        for (int k = 0; k <= z.n; ++k) {
            if (z.A[k] == p) { j = k; break; }
        }

        bool hasRight = (j < z.n);

        if (hasRight) {
            int yRRN  = z.A[j + 1];
            BNode y   = dm_.readNode(yRRN);

            if (y.n >= MID) {
                // Irmão direito tem chave "sobrando": rotação — chave do pai desce, do irmão sobe
                pNode.n += 1;
                pNode.K[pNode.n] = z.K[j + 1];
                pNode.A[pNode.n] = y.A[0];

                z.K[j + 1] = y.K[1];
                // Remove first key of y
                y.A[0] = y.A[1];
                removeFromNode(y, 1);

                dm_.writeNode(p,         pNode);
                dm_.writeNode(parentRRN, z);
                dm_.writeNode(yRRN,      y);
                return;
            }

            // Irmão direito também no mínimo: funde p, separador z.K[j+1] e y num só nó
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

            // Pai perde o separador entre p e y; pode ficar com underflow
            removeFromNode(z, j + 1);
            n = z.n;
            p = parentRRN;
            pNode = z;

        } else {
            // Caso simétrico: irmão à esquerda y = z.A[j-1]
            int yRRN  = z.A[j - 1];
            BNode y   = dm_.readNode(yRRN);

            if (y.n >= MID) {
                // Rotação pelo irmão esquerdo: empurra pNode para a direita e puxa de y
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

            // Funde y, separador z.K[j] e p em y; libera página de p
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

    // Nó reequilibrado ou underflow resolvido antes de chegar à raiz: grava e termina
    if (p != raiz) {
        dm_.writeNode(p, pNode);
        return;
    }

    // Raiz: não tem underflow "abaixo do mínimo" da mesma forma; só trata raiz vazia
    if (pNode.n == 0) {
        // Raiz sem chaves mas com um filho: árvore perde um nível (filho vira raiz)
        int newRoot = pNode.A[0];
        dm_.freeNode(p);
        hdr = dm_.readHeader();
        hdr.root = newRoot;
        dm_.writeHeader(hdr);
    } else {
        dm_.writeNode(p, pNode);
    }
}

// ---------------------------------------------------------------------------
// printTree — hierarchical indented output
// ---------------------------------------------------------------------------
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

// B-tree é balanceada por construção: todas as folhas estão no mesmo nível.
// Basta descer pelo A[0] contando níveis até chegar à folha.
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

// Recursivo: emite definição do nó (record com RRN no topo + chaves nas células,
// uma porta <ai> para cada ponteiro de filho) e depois as arestas pai->filho.
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
