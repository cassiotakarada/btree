# Fala da Apresentação — Árvore B em Memória Secundária
**Versão "teleprompter"** · 38 slides · ~45 min · AED-PG-2026

> Esta é a fala em si, em primeira pessoa, como se você estivesse apresentando.
> Pode ler quase direto. Ajuste o "nós/eu" conforme for em dupla ou sozinho.

---

## Parte 1 — A estrutura

**1. Capa**
"Bom dia a todos. Nosso trabalho é uma Árvore B de ordem m que opera estritamente em
memória secundária. Na prática, isso quer dizer que a árvore inteira vive num arquivo
no disco, e toda operação lê ou escreve um nó por vez — nunca a árvore inteira na
memória. Além de implementar, a gente focou bastante em avaliar o desempenho dela."

**2. Roteiro**
"A apresentação tem duas partes. Primeiro a estrutura: por que disco, como é um nó e o
arquivo, os métodos e uma demonstração do programa rodando. Depois a avaliação: os
experimentos, o impacto da ordem m, uma comparação com um método mais simples, e a
discussão final."

**3. Motivação: memória × disco**
"Por que tanto trabalho para colocar a árvore no disco? Porque a memória RAM é rápida,
mas é pequena e volátil — some quando desliga. O disco é o contrário: gigante e
permanente, só que é cerca de um milhão de vezes mais lento que a RAM. Então, num banco
de dados de verdade, o que custa caro não é a conta na CPU; é o número de vezes que a
gente acessa o disco. A chave para ser rápido é fazer o menor número possível de
acessos. E é exatamente isso que a Árvore B otimiza."

**4. Árvore B: parâmetros (anatomia do nó)**
"Aqui está como é um nó por dentro. Cada nó é um registro de tamanho fixo, que a gente
lê de uma vez só do disco. Ele tem três coisas: o campo n, que diz quantas chaves estão
ali dentro; o vetor A, que são os ponteiros para os filhos; e o vetor K, que são as
chaves em si. A ordem m define o tamanho do nó: cada nó guarda até m menos 1 chaves e
tem até m filhos. E a regra que vale para tudo no trabalho: ler um nó é igual a um
acesso ao disco."

**5. Árvores geradas pelo Graphviz**
"Estas figuras não são desenho nosso — foram exportadas pelo próprio programa. Cada
caixa é um nó no disco, e o número dela é a posição no arquivo. Repara que com ordem 3 a
árvore precisa de mais níveis; já com ordem 4, como cabem mais chaves por nó, ela fica
mais baixa. E árvore mais baixa significa menos acessos ao disco para chegar num dado."

**6. Layout do arquivo no disco**
"Agora, como esses nós ficam organizados no arquivo. O arquivo é basicamente um vetor de
páginas de tamanho fixo, numeradas por posição. A página de número zero é o cabeçalho:
ela guarda onde está a raiz, quantos nós existem, e a cabeça da lista de nós livres. Com
um único acesso a esse cabeçalho, a gente já sabe por onde começar. E achar qualquer nó
é conta direta: posição vezes o tamanho da página. Quando um nó é removido, ele não vira
buraco — entra numa lista de livres para ser reaproveitado depois."

**7. Problema e objetivos**
"Esse é o enunciado. A gente tinha que implementar uma classe Árvore B de ordem m que
ficasse em memória secundária, ou seja, num arquivo em disco. A restrição central, que é
o coração do trabalho, é que a árvore nunca pode ser carregada inteira na RAM: cada
operação acessa um nó por vez. As operações pedidas são busca, inserção e remoção. E o
objetivo que a gente assumiu foi avaliar o desempenho variando os parâmetros."

**8. O índice e a métrica mais fiel**
"Esse jeito de organizar o arquivo faz a Árvore B funcionar como um índice. Em vez de
varrer o arquivo todo procurando um dado, ela segue só um caminho, da raiz até a folha,
começando pelo cabeçalho. E isso leva à pergunta: como a gente mede se está rápido? A
métrica mais honesta não é o relógio, porque o tempo depende da máquina, do sistema, de
mil coisas. A métrica fiel é o número de acessos ao disco — esse número é o mesmo em
qualquer computador."

**9. Decisões de implementação**
"Quatro decisões de projeto guiaram tudo. A ordem m é uma constante de compilação. A
raiz fica registrada no cabeçalho. Um nó por acesso de disco, sempre — nunca a árvore
toda. E o reaproveitamento de nós por uma lista de livres. Aqui do lado tem uma árvore
de ordem 3 real, com 40 chaves, exportada pelo nosso código."

**10. A classe DiskManager — um nó por I/O**
"Toda leitura e escrita no disco passa por uma única classe, o DiskManager. Quando a
gente precisa olhar um nó, ele faz uma leitura; se o nó muda, ele faz uma escrita — e
sempre de um registro inteiro, do tamanho da página. O mais importante: é dentro dessa
classe que mora o contador de acessos, que é a nossa métrica principal. Como a lógica da
árvore só enxerga o disco através dela, a gente tem garantia de que nada vai inteiro
para a memória."

**11. Métodos: visão geral**
"Resumindo os métodos: busca em m vias, inserção de baixo para cima com divisão de nó, e
remoção com sucessor, redistribuição e fusão. Mais as ferramentas de monitoramento, como
imprimir a árvore e exportar o grafo. Vou detalhar as três operações principais agora."

**12. Busca m-way**
"A busca funciona assim: em cada nó, as chaves dividem o universo de valores em faixas. A
gente compara o que procura e desce pela faixa certa, até chegar na folha. O custo da
busca é mais ou menos a altura da árvore em acessos — ou seja, pouquíssimos. No grafo,
buscar uma chave custa só dois acessos."

**13. Inserção e split**
"A inserção é de baixo para cima. A gente acha a folha certa e insere a chave em ordem.
Se o nó estourar, quer dizer, passar do limite de chaves, a gente faz um split: divide o
nó em dois e a chave do meio sobe para o pai. Isso pode propagar para cima e, no limite,
criar uma nova raiz. É exatamente esse mecanismo que mantém a árvore sempre balanceada,
sem a gente fazer nada a mais."

**14. Remoção**
"A remoção é o caso mais delicado. Se a chave está num nó interno, a gente troca ela pelo
sucessor, que é a menor chave maior que ela. Quando uma folha fica com chaves de menos,
a gente conserta de dois jeitos: redistribuição, que é pegar emprestada uma chave de um
irmão; ou fusão, que é juntar dois nós com a chave do pai no meio. E a fusão também pode
propagar para cima e diminuir a altura da árvore."

**15. Reaproveitamento — free list**
"E o que acontece com o nó que sobra quando há uma fusão? Ele não é desperdiçado. Ele
vira um nó livre e entra numa lista encadeada dentro do próprio arquivo, com a cabeça no
cabeçalho. Na próxima vez que a gente precisa de um nó novo, primeiro a gente reaproveita
um livre, antes de aumentar o arquivo. Isso é ligável e desligável, e a gente usou esse
botão para medir quanto de espaço o reaproveitamento economiza."

**16. Resumo do que o código faz**
"Resumindo o código em um slide. Do lado esquerdo, o núcleo: a Árvore B cem por cento em
disco, com ordem configurável, as três operações com split e fusão, e a persistência por
posição no arquivo. Do lado direito, os extras de avaliação: o contador de acessos, a
lista de livres ligável, e as ferramentas de visualização. O núcleo é a estrutura; os
extras existem para medir a estrutura."

**17. Ferramentas utilizadas**
"As ferramentas. A implementação é em C++ 17, compilado com g++ e otimização, usando
Make e a biblioteca de arquivos binários com registros fixos. Para a avaliação e o
material, a gente usou o Graphviz para os grafos, Python com matplotlib para os
gráficos, e Python também para gerar os slides. Todo o projeto é versionado em Git, e os
experimentos são reprodutíveis: cada teste roda por scripts e gera os dados em planilha."

**18. Demonstração — o programa rodando**
"Chega de slides, vamos ver rodando. Isto é uma captura real: a gente compila com ordem
3, abre o menu interativo, e faz inserção, busca, remoção, vê os acessos e exporta o
grafo. Depois mostro com ordem 5, onde dá para ver a árvore ficar mais rasa."

## Parte 2 — A avaliação

**19. Os experimentos (matriz)**
"Para avaliar, a gente montou quatro experimentos. Um: o impacto da ordem m. Dois: a
escala, indo de mil até um milhão de chaves. Três: a ocupação de espaço, com e sem o
reaproveitamento. E quatro: a decomposição do tempo, separando CPU e disco. Tudo isso
nos modos aleatório e sequencial, e validado em duas máquinas diferentes."

**20. Metodologia e ambiente**
"Sobre o rigor da medição: a gente fez um programa separado, não interativo, que roda os
testes e cospe os resultados em planilha. Cada configuração é reconstruída do zero, para
um teste não contaminar o outro. A gente mede CPU de usuário, CPU de sistema e espera de
disco. E rodou em duas máquinas: um notebook e o servidor Titan, da USP."

**21. Métricas utilizadas**
"As métricas que a gente acompanhou. A principal é o número de acessos ao disco. Além
dela: a altura da árvore, que cresce com o logaritmo de N; a ocupação, em número de nós
e bytes do arquivo; e o tempo, decomposto em relógio, CPU e espera de disco."

**22. Impacto de m: I/O por busca**
"Esse é o resultado central. Com ordem 3, uma busca custa em média 13 acessos ao disco,
porque a árvore tem 14 níveis. Conforme a gente aumenta o m, esse número despenca: com
ordem 512, uma busca custa só 2 acessos. Mas reparem numa coisa importante: o ganho
satura por volta de m igual a 64, 128. Depois disso, aumentar o m quase não ajuda mais,
porque a árvore já está rasa. Ou seja, existe um m ótimo, que na prática é o tamanho de
um bloco de disco."

**23. Impacto de m: altura**
"E aqui dá para ver por quê. A altura da árvore é praticamente igual ao número de
acessos por busca. Ela cai em degraus: de 14 níveis com ordem 3, até 2 níveis com ordem
grande. Essa altura baixa é literalmente a razão de existir da Árvore B."

**24. Resultados por operação**
"O ganho não é só na busca. A busca é a operação mais barata, porque só desce da raiz à
folha. A inserção custa um pouco mais, porque o split reescreve nós. E a remoção é a mais
cara, porque às vezes precisa olhar os irmãos para redistribuir ou fundir. Mas as três
caem com o aumento do m e saturam juntas."

**25. Escala do conjunto**
"E quando o volume de dados cresce? A gente foi de mil a um milhão de chaves, mil vezes
mais dados. E o custo por busca quase não mudou. Isso porque o custo é logarítmico na
base m. Na prática: o dado cresce, o custo quase não cresce. É o que torna a estrutura
escalável."

**26. Comparação: Árvore B × Array ordenado**
"Para deixar esse ganho bem concreto, a gente comparou a Árvore B com um método mais
simples: um array ordenado em disco, usando o mesmo contador de acessos, para a
comparação ser justa. O resultado é interessante. Na busca, os dois quase empatam,
porque ambos fazem busca binária, que é logarítmica. O problema do array aparece na
escrita: para inserir ou remover mantendo a ordem, ele precisa deslocar metade do
arquivo. Com dez mil chaves, isso dá cerca de cinco mil acessos por inserção, contra uns
nove da Árvore B — até quinhentas vezes mais acessos ao disco. A lição é essa: ordenar é
fácil; difícil é manter ordenado quando você insere e remove o tempo todo. E é
exatamente esse problema que a Árvore B resolve, com divisões e fusões locais."

**27. Reaproveitamento de nós (churn)**
"Voltando ao espaço. A gente fez um teste de vai-e-vem: insere tudo, remove metade, e
insere de novo. Com o reaproveitamento ligado, o arquivo praticamente não cresce, porque
reusa os nós livres. Com ele desligado, o arquivo incha. Na prática, a lista de livres
economizou em torno de 27 a 30 por cento do tamanho do arquivo nesse cenário."

**28. As duas máquinas da avaliação**
"Antes de comparar o desempenho nas duas máquinas, vale mostrar quais são elas. À
esquerda, o notebook, nossa referência: um Intel de 8 núcleos, 16 GB de RAM, rodando
Ubuntu dentro do WSL2, no Windows. À direita, o servidor Titan, da USP: um AMD
Threadripper de 32 núcleos, 125 GB de RAM, Ubuntu bare-metal, com SSD NVMe. São máquinas
bem diferentes — uma é um laptop virtualizado, a outra é uma workstation parruda rodando
direto no hardware. Mas reparem: o software é o mesmo, mesmo compilador e mesmas versões.
Guardem essa diferença de hardware, porque no próximo slide vem a surpresa: apesar de
tudo isso, o número de acessos ao disco é idêntico nas duas. Só o tempo muda."

**29. Dois sistemas: tempo (wall)**
"Agora a validação em duas máquinas. Rodando o mesmo programa no notebook e no Titan, os
acessos ao disco foram idênticos; só o tempo de relógio mudou. E reparem que as duas
curvas têm exatamente a mesma forma."

**30. Dois sistemas: CPU e determinismo**
"Esse slide explica por que o relógio engana. Do lado esquerdo: o tempo é quase todo CPU
de sistema, e quase nada de espera de disco. Isso porque a escrita vai para o cache do
sistema operacional, não direto para o disco físico. Por isso o relógio não é uma métrica
honesta. Do lado direito está a prova do determinismo: cada ponto é um teste nas duas
máquinas, e todos caem em cima da reta — ou seja, os acessos foram rigorosamente iguais.
A estrutura é determinística e portável."

**31. Dois sistemas: validação cruzada**
"Em resumo: notebook contra Titan, os acessos ao disco são idênticos, só o tempo varia, e
a comparação foi feita de forma automatizada por um script."

**32. Dois sistemas: escala**
"E a escala também se confirma nas duas máquinas. Indo até um milhão de chaves, com ordem
100, o tempo cresce bem devagar, de forma logarítmica, nas duas. Mesma tendência, com o
Titan rodando mais rápido. É a prova prática da escalabilidade."

**33. E se usássemos union no header?**
"Um ponto que o professor levantou: e se a gente usasse um union no cabeçalho? Hoje o
cabeçalho e o nó são tipos separados, e a gravação é feita campo por campo. Com um union,
os dois seriam uma página única, do mesmo tamanho fixo: dava para ler e gravar de uma vez
só, com menos código e menos chance de bug. O custo seria a portabilidade — questões de
alinhamento de bytes entre máquinas — mas isso é contornável. No geral, ficaria mais
limpo, exigindo cuidado com o formato binário."

**34. Dificuldades técnicas**
"As dificuldades que a gente teve. A indexação começando do zero, com o cabeçalho na
posição zero. Na remoção, manter o caminho de volta para consertar os nós. Um detalhe
chato do C++, o eofbit, que silenciava leituras e a gente resolveu com um clear. E a
gravação campo por campo, que é trabalhosa e fácil de errar — foi ela, aliás, que
motivou a ideia do union."

**35. Vantagens × desvantagens**
"Fazendo o balanço. Vantagens: a árvore é baixa, então são poucos acessos; está sempre
balanceada; é ideal para disco e para índices de banco; e é determinística e econômica
com o reuso. Desvantagens: os nós podem ficar só metade cheios no caso sequencial; a
remoção é complexa de implementar; e um m grande demais troca acesso a disco por mais
trabalho de CPU. Por isso existe aquele m ótimo, do tamanho do bloco."

**36. Aplicações práticas**
"Isso não é só teoria — está em todo lugar. Os bancos de dados usam, como MySQL,
PostgreSQL e Oracle. Os sistemas de arquivos usam, como NTFS, ext4 e Btrfs. E bancos
chave-valor também, como SQLite e LMDB. O uso mais natural é exatamente esse: ser o
índice de um banco de dados em disco."

**37. Conclusão**
"Para concluir, os principais achados. Primeiro: a Árvore B funciona cem por cento em
disco, com um nó por acesso. Segundo: o número de acessos por operação cai com o m até
saturar, por volta de 64 a 128 — existe um m ótimo. Terceiro: a lista de livres economiza
em torno de 30 por cento de espaço. E quarto: a estrutura é determinística, com acessos
idênticos nas duas máquinas. Rasa, balanceada e econômica — por isso ela é a base de
bancos de dados e sistemas de arquivos até hoje."

**38. Referências**
"Essas são as referências que a gente usou — o artigo clássico do Comer, o Knuth, o
Folk e Zoellick, o trabalho original do Bayer e McCreight, e os slides do professor.
Muito obrigado, e ficamos à disposição para perguntas."
