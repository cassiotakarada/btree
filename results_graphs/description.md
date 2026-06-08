# Grafos — imagens da Árvore B (início e fim do processo)

Imagens geradas via Graphviz a partir do export `.dot` da própria árvore
(`BTree::exportDot`). Cada nó é um *record* mostrando seu RRN (número do registro
em disco) e as chaves; as arestas saem das "portas" A[i] do pai para os filhos.

São árvores **pequenas e legíveis** (N=40 chaves, mesmo conjunto e seed para
todas as ordens) — propositalmente, já que renderizar uma árvore de 10⁶ chaves
seria ilegível. O objetivo é ilustrar a estrutura e a **redução de altura**
conforme a ordem *m* cresce.

## Arquivos
- `m3_ins6.png` — **início do processo**: árvore de ordem 3 após as 6 primeiras
  inserções.
- `m3_final.png` — **fim do processo**: árvore de ordem 3 após as 40 inserções.
- `m5_final.png` — mesma carga (40 chaves) em ordem 5.
- `m10_final.png` — mesma carga (40 chaves) em ordem 10.

Comparar `m3_final` × `m5_final` × `m10_final` mostra, com o **mesmo conjunto de
chaves**, como aumentar *m* achata a árvore (menos níveis ⇒ menos acessos a disco
por busca). Os `.dot` correspondentes ficam ao lado dos `.png` para reprodução.
