# Trabalho AED-PG-2026 — Árvore B em Armazenamento Secundário

Implementação completa de uma Árvore B (B-Tree) persistida inteiramente em disco,
conforme especificação do Prof. AED-PG-2026 — Universidade do Estado de São Paulo.

---

## 1. Estrutura de dados e algoritmos implementados

A **Árvore B de ordem m** é uma árvore de busca balanceada com as seguintes propriedades:

| Invariante | Valor |
|---|---|
| Máximo de chaves por nó | m − 1 |
| Mínimo de chaves por nó (não-raiz) | ⌈m/2⌉ − 1 |
| Máximo de filhos por nó | m |
| Todas as folhas na mesma profundidade | sim |

### Operações

- **`mSearch(T, x)`** — busca m-way (slide 22): percorre do nó `T` em direção às folhas comparando `x` contra as chaves do nó corrente; retorna `(p, i, achou)` onde `p` é o RRN do nó, `i` o índice da posição, e `achou` é `true` se a chave foi encontrada.
- **`insertB(T, x)`** — inserção bottom-up (slide 84): usa `mSearch` para localizar a folha, insere `x`, e propaga splits rumo à raiz conforme a Equação 1.
- **`deleteB(T, x)`** — remoção (slides 117–118): para nó não-folha, substitui a chave pelo sucessor imediato; repara underflow por redistribuição (borrow de irmão) ou fusão (merge), propagando rumo à raiz se necessário.

### Equação 1 (split com m = 5, ⌈m/2⌉ = 3)

```
Nó p (cheio, n = m):
  K₁ K₂ K₃ K₄ K₅
       ↓ split
p mantém:  K₁ K₂         (⌈m/2⌉ − 1 = 2 chaves)
promovido: K₃             (1 chave vai para o pai)
q recebe:  K₄ K₅         (m − ⌈m/2⌉ = 2 chaves)
```

---

## 2. Formato físico do arquivo binário

O arquivo `.bin` é dividido em registros de tamanho fixo chamados **páginas**:

```
PAGE_SIZE = sizeof(int) × (1 + m + m)  =  (2m + 1) × 4  bytes
```

| Registro (RRN) | Conteúdo |
|---|---|
| 0 | **Cabeçalho**: `root` (RRN da raiz), `total` (nós alocados), `free_head` (cabeça da free list) |
| 1, 2, … | **Nós da Árvore B**: `n`, `A[0..m−1]`, `K[0..m−1]` |

Offset de acesso direto: `offset(rrn) = rrn × PAGE_SIZE`.

**Invariante de I/O (Item 5 do enunciado — condição de zeragem):** a árvore **nunca** existe inteira em memória. Cada operação lê ou escreve exatamente um nó por vez, via `readNode(rrn)` / `writeNode(rrn, node)` usando `fstream` em modo binário com `seekg`/`seekp`.

---

## 3. Reaproveitamento de nós (free list)

Nós removidos durante fusões (`deleteB`) são empurrados para uma lista encadeada de nós livres persistida no próprio arquivo. O campo `free_head` no cabeçalho aponta para o primeiro nó livre; cada nó livre armazena o ponteiro para o próximo em `K[1]` (com `n = −1` como marcador).

`allocNode()` retira da lista antes de alocar uma nova página, garantindo que o arquivo não cresça desnecessariamente quando há nós reciclados disponíveis.

---

## 4. Contador de acessos ao disco

Todo `readNode` e `writeNode` incrementa o contador `disk_accesses_` em `DiskManager`. O menu interativo exibe o contador a qualquer momento (opção 5) e permite zerá-lo (opção 6). O experimento automático mede acessos médios por operação separadamente para inserção, busca e remoção.

---

## 5. Compilação e uso

### Requisitos

- GCC ≥ 7 com suporte a C++17 (`-std=c++17`)
- Make

### Compilar

```bash
# Ordem padrão m = 5
make

# Ordem personalizada (ex.: m = 7)
make M=7

# Mudar m exige recompilação completa (automática via stamp file)
make M=3
```

### Executar

```bash
./btree [arquivo.bin]      # arquivo padrão: btree.bin
```

### Menu interativo

```
1. Inserir chave
2. Buscar chave
3. Remover chave
4. Imprimir árvore (hierárquico com RRNs)
5. Exibir acessos ao disco
6. Zerar contador
7. Experimento automático (varia N e modo random/sequential)
0. Sair
```

### Experimento automatizado

O menu 7 solicita `N` (quantidade de chaves) e modo (0 = aleatório, 1 = sequencial), executa inserção de todos os N elementos, busca de todos, e remoção de todos (em ordem embaralhada), reportando:

- Total e média de acessos por operação
- Tempo de CPU (ms) para cada fase

Exemplo de saída com m = 5, N = 1 000, aleatório:

```
--- Resultados (ordem=5, N=1000, modo=rand) ---
Inserção : 7530 acessos  (7.53 médio/op)  14.75 ms
Busca    : 4636 acessos  (4.636 médio/op)   6.97 ms
Remoção  : 6710 acessos  (6.71 médio/op)  12.69 ms
```

---

## 6. Organização do código-fonte

```
trabalho/
├── src/
│   ├── types.hpp         — constantes (ORDER, MID, PAGE_SIZE), structs BNode e Header
│   ├── disk_manager.hpp  — interface DiskManager (I/O binário + contador de acessos)
│   ├── disk_manager.cpp  — implementação: readNode, writeNode, allocNode, freeNode
│   ├── btree.hpp         — interface BTree
│   └── btree.cpp         — mSearch, insertB, deleteB, printTree e auxiliares
├── main.cpp              — menu interativo e driver experimental
├── Makefile              — compilação com variável M configurável
└── README.md             — este arquivo
```

---

## 7. Uso de ferramentas de IA

Este trabalho foi implementado com apoio do assistente **Claude Sonnet 4.6** (Anthropic) via Claude Code CLI, utilizado exclusivamente como ferramenta de auxílio ao desenvolvimento.

Contribuições da IA:
- Discussão e síntese dos pseudocódigos dos slides do professor.
- Identificação e correção de dois bugs de implementação:
  1. *Stale-header bug*: `insertB` capturava o cabeçalho antes de `allocNode()` atualizar `total`, causando colisão de RRNs e auto-referência de ponteiros.
  2. *fstream eofbit bug*: escrita além do fim do arquivo setava o bit `eofbit` do `fstream`, fazendo as leituras subsequentes falharem silenciosamente; corrigido com `file_.clear()` antes de cada `seekg`/`seekp`.
- Geração inicial do esqueleto de código (refatorado e verificado manualmente).

Todo o código foi revisado e validado manualmente pelo autor, incluindo verificação contra os pseudocódigos dos slides e testes funcionais.

**O autor é responsável pelo entendimento e pela correção final de todo o código entregue.**
