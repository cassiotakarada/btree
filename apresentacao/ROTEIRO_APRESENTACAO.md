# Roteiro de Apresentação — Árvore B em Memória Secundária
**Duração alvo: 40 a 60 minutos** · 36 slides · ~1 a 1:30 por slide · AED-PG-2026

> Como usar: cada slide tem a **mensagem-chave** (o que a banca tem que levar) e a
> **fala** (texto natural, pode adaptar). As frases em _itálico_ são transições. No
> fim há um bloco de **perguntas prováveis**.
>
> **Público:** suponha que parte da plateia não conhece Árvore B. Não precisamos
> *ensinar* a estrutura, mas a linguagem deve ser acessível — fale em termos de
> "arquivo no disco" e "menos acessos = mais rápido".
>
> **Ritmo:** 36 slides. A ~1 min/slide → ~40 min; a ~1:30/slide → ~54 min. Os slides
> marcados com (~) são candidatos a encurtar se o tempo apertar; os com (*) merecem
> mais tempo.

| # | Slide | Alvo |
|---|---|---|
| 1 | Capa | 1:00 |
| 2 | Roteiro da apresentação (~) | 1:00 |
| 3 | O problema e os objetivos | 1:30 |
| 4 | Contexto: memória × disco | 1:30 |
| 5 | Contexto: índice e métrica honesta | 1:30 |
| 6 | Anatomia de um nó (*) | 1:30 |
| 7 | Layout do arquivo no disco (*) | 1:30 |
| 8 | Decisões de implementação | 1:30 |
| 9 | O DiskManager (1 nó por I/O) | 1:30 |
| 10 | Reaproveitamento: lista de livres | 1:30 |
| 11 | Métodos: visão geral (~) | 1:00 |
| 12 | Busca m-way | 1:30 |
| 13 | Inserção e split | 1:30 |
| 14 | Remoção | 1:30 |
| 15 | Resumo do que o código faz (~) | 1:00 |
| 16 | Demonstração (GIF) | 1:30 |
| 17 | Grafos gerados | 1:00 |
| 18 | Os experimentos | 1:30 |
| 19 | Metodologia e ambiente | 1:30 |
| 20 | Métricas | 1:00 |
| 21 | Impacto de m: I/O por busca (*) | 2:00 |
| 22 | Impacto de m: altura | 1:30 |
| 23 | Resultados por operação | 1:30 |
| 24 | Escala do conjunto (N) | 1:30 |
| 25 | Reaproveitamento: resultado | 1:30 |
| 26 | Dois sistemas: tempo (wall) | 1:30 |
| 27 | Dois sistemas: CPU e determinismo (*) | 2:00 |
| 28 | Dois sistemas: validação (resumo) (~) | 1:00 |
| 29 | Dois sistemas: escala | 1:00 |
| 30 | E se usássemos union? (*) | 2:00 |
| 31 | Dificuldades técnicas | 1:30 |
| 32 | Vantagens × desvantagens | 1:30 |
| 33 | Aplicações | 1:00 |
| 34 | Ferramentas utilizadas | 1:30 |
| 35 | Conclusão | 1:30 |
| 36 | Referências / encerramento | 0:30 |

---

## Parte 1 — A estrutura

**1. Capa.** _Mensagem:_ o que é o trabalho em uma frase.
"Nosso trabalho é uma **Árvore B de ordem m que opera estritamente em memória
secundária** — a árvore vive num arquivo no disco e toda operação lê ou escreve **um
nó por vez**, nunca a árvore inteira." _Apresentem-se e citem o foco na avaliação
experimental._

**2. Roteiro.** _Mensagem:_ mapa da fala. "Primeiro a estrutura — contexto, como o nó
e o arquivo são, os métodos e uma demo. Depois a avaliação — experimentos, impacto de
m, comparação entre duas máquinas e a discussão final." _Passe rápido._

**3. Problema e objetivos.** _Mensagem:_ o que o enunciado pede. "Implementar a Árvore
B residindo em **disco**, lida em blocos, **nunca inteira na RAM**; com busca,
inserção e remoção; contando acessos ao disco. Acrescentamos uma avaliação empírica e
validação em mais de uma máquina." _"Por que disco importa tanto?"_

**4. Contexto: memória × disco.** _Mensagem:_ disco é o gargalo. "RAM é rápida, mas
pequena e volátil; disco é enorme e permanente, porém **milhares de vezes mais
lento**. Logo, o segredo da velocidade é fazer o **menor número de acessos ao
disco**." _"E como evitamos ler o arquivo todo?"_

**5. Contexto: índice e métrica.** _Mensagem:_ índice + métrica honesta. "Um **índice**
diz onde cada registro está, evitando ler tudo — é o que a Árvore B faz em bancos de
dados. E a métrica honesta **não é o relógio**, é o **número de acessos ao disco**,
que independe do hardware." _"Vamos ver como um nó é por dentro."_

**6. Anatomia de um nó. (*)** _Mensagem:_ o que há dentro de um nó. "Cada nó é um
registro de **tamanho fixo**: o campo **n** (quantas chaves), o vetor de **ponteiros
A[ ]** (os RRN dos filhos) e o vetor de **chaves K[ ]**. Um nó de ordem m guarda até
m-1 chaves. Ler um nó = **um acesso ao disco**." _"E como esses nós ficam no arquivo?"_

**7. Layout do arquivo. (*)** _Mensagem:_ o arquivo é um vetor de registros. "O arquivo
é um **vetor de registros** numerados por RRN. O registro 0 é o **header** (raiz,
total, free_head). Achamos qualquer nó por **cálculo direto**: offset = RRN ×
PAGE_SIZE. E nós removidos entram numa **lista de livres** para reuso." _"Com isso, as
decisões de projeto."_

**8. Decisões de implementação.** _Mensagem:_ as 4 decisões. "Ordem m como constante de
compilação; raiz no header; **um nó por I/O**; e reaproveitamento de nós via lista de
livres. À direita, uma árvore de ordem 3 real, exportada pelo código." _"Toda a E/S
passa por uma classe."_

**9. O DiskManager.** _Mensagem:_ onde mora o '1 nó por I/O'. "readNode e writeNode leem
e escrevem **um** registro — e é aqui que fica o **contador de acessos**, a métrica
central. A lógica da árvore só enxerga o disco pelo DiskManager, então nada vai
inteiro para a RAM." _"E quando um nó é liberado?"_

**10. Reaproveitamento.** _Mensagem:_ como a free list funciona. "Ao remover, o nó vira
**livre** e entra numa pilha encadeada no próprio arquivo (cabeça no header). A
próxima alocação **reusa** um livre antes de crescer o arquivo. É ligável — usamos
isso para medir o ganho de espaço." _"Agora os métodos."_

**11. Métodos: visão geral. (~)** _Mensagem:_ as três operações + ferramentas. "Busca,
inserção e remoção, todas em disco, mais as ferramentas de monitoramento. Vou detalhar
uma a uma." _Não leia item a item._

**12. Busca m-way.** _Mensagem:_ desce por faixas. "Em cada nó, as chaves dividem o
universo em **faixas**; descemos pela faixa certa até a folha. O custo é ~**a altura**
em acessos — pouquíssimos. No grafo, buscar 70 custa 2 acessos." _"Inserir é parecido,
até estourar."_

**13. Inserção e split.** _Mensagem:_ split mantém o balanço. "Acha a folha e insere; se
o nó **estoura**, dividimos em dois e a **mediana sobe** para o pai. Isso pode
propagar e criar uma nova raiz — é o que mantém a árvore **sempre balanceada**."
_"Remover é o caso mais delicado."_

**14. Remoção.** _Mensagem:_ sucessor, redistribuição, fusão. "Se a chave está num nó
interno, trocamos pelo **sucessor**. Se uma folha fica abaixo do mínimo, reparamos:
**redistribuição** (empréstimo de um irmão) ou **fusão** (junta com o irmão). A fusão
pode propagar para cima e baixar a árvore." _"Num slide, tudo que o código faz."_

**15. Resumo do código. (~)** _Mensagem:_ núcleo + extras. "À esquerda o núcleo da
árvore em disco; à direita os extras de avaliação (contador, free list, grafos,
benchmark). O núcleo é a estrutura; os extras existem para **medi-la**." _"Vamos ver
rodando."_

**16. Demonstração (GIF).** _Mensagem:_ é real. "Execução de verdade: do `make M=3` por
**cada item do menu** — inserir, buscar, remover, acessos, imprimir, exportar grafo —
e depois com m=5, onde a árvore fica mais rasa." _"E o próprio programa desenha o
grafo."_

**17. Grafos gerados.** _Mensagem:_ m maior = mais raso. "Exportados pelo programa via
Graphviz. Cada caixa é um nó no disco (RRN). m=3 precisa de mais níveis; m=5 fica mais
raso — **menos acessos**." _"Como medimos isso de forma sistemática."_

## Parte 2 — A avaliação

**18. Os experimentos.** _Mensagem:_ a matriz cobre o enunciado. "Quatro experimentos:
impacto de m; escala de N (mil a um milhão); ocupação com/sem reuso; e decomposição do
tempo. Tudo aleatório e sequencial, em **duas máquinas**." _"Como exatamente
medimos."_

**19. Metodologia e ambiente.** _Mensagem:_ rigor. "Driver não interativo emite CSV;
cada caso é reconstruído do zero; medimos com getrusage (CPU usuário, CPU sistema,
espera de I/O); duas máquinas — Notebook e Titan/USP — com automação." _"As seis
métricas."_

**20. Métricas. (~)** _Mensagem:_ a honesta é o contador. "Acessos ao disco (a
principal), altura, ocupação e tempo decomposto. A altura já cai em degraus com m."
_"O resultado central: impacto de m."_

**21. Impacto de m: I/O por busca. (*)** _Mensagem:_ despenca e **satura**. "Com m=3,
13 acessos por busca e 14 níveis. Subindo m, despenca: m=512 → 2 acessos. Mas o ganho
**satura** entre m=64 e 128 — a árvore já é rasa demais. Existe um **m ótimo**, do
tamanho de um bloco de disco." _"O mesmo aparece na altura."_

**22. Impacto de m: altura.** _Mensagem:_ altura ~ acessos. "A altura cai em degraus de
14 (m=3) para 2 (m grande). Altura baixa é a razão de existir da Árvore B." _"E nas
três operações?"_

**23. Resultados por operação.** _Mensagem:_ o ganho vale para todas. "Busca é a mais
barata; inserção um pouco mais (split); remoção a mais cara (lê irmãos). As três caem
com m e **todas saturam**." _"E quando o conjunto cresce?"_

**24. Escala do conjunto.** _Mensagem:_ escalável. "Multiplicamos N por mil e o custo
por busca **mal muda** — é logarítmico na base m. A estrutura aguenta bases enormes."
_"O segundo eixo: espaço."_

**25. Reaproveitamento: resultado.** _Mensagem:_ ~30% de economia. "No churn (insere,
remove metade, reinsere), com reuso o arquivo **não cresce**; sem reuso, **incha**. A
free list economiza **27-30%**." _"Sobre o processo: validação em duas máquinas."_

**26. Dois sistemas: wall.** _Mensagem:_ a forma é igual. "Notebook × Titan: acessos a
disco **idênticos**; só o tempo muda. As curvas têm **a mesma forma**." _"Por que o
relógio engana — e a prova de determinismo."_

**27. Dois sistemas: CPU e determinismo. (*)** _Mensagem:_ idêntico nas duas. "À
esquerda: o tempo é quase todo **CPU de sistema** (I/O via cache), por isso o relógio
não é honesto. À direita: todos os pontos caem na reta **y=x** — acessos
**rigorosamente iguais**. Determinístico e portável." _"Resumindo a validação."_

**28. Validação: resumo. (~)** _Mensagem:_ portabilidade. "Métricas de acesso idênticas
byte a byte; só o tempo difere; comparação automatizada por script." _"E a escala nas
duas?"_

**29. Dois sistemas: escala.** _Mensagem:_ tendência igual. "De mil a um milhão nas duas
máquinas: mesma tendência, Titan em paralelo abaixo. Escala comprovada." _"Um ponto que
o professor levantou: o union."_

**30. E se usássemos union? (*)** _Mensagem:_ página única × portabilidade. "Hoje header
e nó são tipos separados, com serialização campo a campo. Com **union**, seriam uma
**página única** de tamanho fixo: ler/gravar de uma vez, menos código e bugs, free
list explícita. Custo: **portabilidade** (padding/endianness) e o UB de type-punning,
resolvível com memcpy/bit_cast. Mais limpo, exigindo cuidado com o formato binário."
_"Quais foram as dificuldades?"_

**31. Dificuldades técnicas.** _Mensagem:_ o que custou. "Indexação a partir do RRN 0; a
ordem do path na remoção; o `eofbit` que silenciava leituras; e garantir 1 nó por I/O.
A serialização verbosa foi o que motivou a ideia do union." _"E os trade-offs?"_

**32. Vantagens × desvantagens.** _Mensagem:_ equilíbrio consciente. "Vantagens: rasa,
balanceada, determinística, econômica. Custos: nós subocupados, remoção complexa, e m
grande troca I/O por CPU — daí o **m ótimo**." _"Onde isso é usado de verdade?"_

**33. Aplicações.** _Mensagem:_ está em tudo. "Índices de MySQL e PostgreSQL, sistemas
de arquivos (NTFS, ext4), key-value stores (SQLite, LMDB). Eu usaria como índice de um
banco em disco." _"Com que ferramentas construímos isso?"_

**34. Ferramentas utilizadas.** _Mensagem:_ stack + papel da IA. "Implementação: C++17,
Make, fstream, Graphviz. Avaliação: Python/matplotlib, Pillow (GIFs), python-pptx e
reportlab (slides); Git; duas máquinas. E sou transparente: usamos o Claude como
**apoio, revisão e incremento** — no menu, na exportação de grafos e na revisão. A
**lógica da Árvore B é nossa**; a IA acelerou ferramentas acessórias." _"Para fechar."_

**35. Conclusão.** _Mensagem:_ os três achados. "Um: I/O cai com m até **saturar** —
existe m ótimo. Dois: a free list economiza **~30%**. Três: resultados
**determinísticos** em duas máquinas. A Árvore B é rasa, balanceada e econômica — por
isso é a base de bancos e sistemas de arquivos." _"Referências e obrigado."_

**36. Referências.** Comer (1979); Knuth, TAOCP vol. 3; Folk & Zoellick; Bayer &
McCreight (1972); slides do Prof. Baranauskas. "Obrigado! Ficamos à disposição."

---

## Perguntas prováveis (preparação)

**"Árvore B não é binária?"** → "Não. Binária é a BST (2 filhos). A Árvore B é
**m-way** (até m filhos); o 'B' é de balanceada (Bayer). m grande deixa a árvore rasa.
Ordem 2 é degenerada e o código a proíbe; ordem 3 é só o menor caso testado."

**"Por que o tempo aparece como CPU, não como espera de I/O?"** → "Porque `writeNode`
faz `flush` para o **cache de páginas do SO**, não `fsync` no disco físico. O 'I/O'
vira CPU de sistema. Por isso a métrica honesta é o **contador de acessos**."

**"O que mudaria de fato com o union no header?"** → "Header e nó virariam **uma página
única** de tamanho fixo garantido em compilação: ler/gravar de uma vez, sem `memcpy`
campo a campo, e free list explícita. Custo: portabilidade (padding/endianness) e UB
de type-punning, resolvível com `memcpy`/`std::bit_cast`."

**"Onde fica a raiz?"** → "No header, registro 0 — uma leitura de metadado e já sabemos
a raiz."

**"Como funciona o reaproveitamento?"** → "Lista de livres encadeada no arquivo: cabeça
em `free_head`, cada livre guarda o próximo em `K[1]` com n=-1; `allocNode` puxa dela
antes de estender o arquivo."

**"Qual o m ideal na prática?"** → "Aquele em que o nó preenche um **bloco de disco**
(4-8 KB) — é onde a curva do slide 21 satura."

**"Por que a inserção sequencial gera arquivo maior?"** → "O split sempre cai à direita,
deixando nós ~50% cheios; no aleatório a ocupação fica ~69% (ln 2)."

**"A IA fez o trabalho?"** → "Não. A lógica da Árvore B é nossa; a IA apoiou ferramentas
acessórias (menu, grafos, scripts) e revisão. As decisões e a análise são nossas."

---

### Dicas de cronometragem
- **40 min (rápido):** ~1 min/slide; encurte os marcados (~) (2, 11, 15, 20, 28) lendo
  só os títulos.
- **50-60 min (completo):** detalhe os marcados (*) (6, 7, 21, 27, 30) e responda
  perguntas no fim de cada parte.
- **Não leia os slides palavra por palavra** — a fala acima é a narração; os slides são
  o apoio visual.
