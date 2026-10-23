# Labirinto Giratório – M2

Equipe: Arthur Ferraz de Aquino & Vicenzo Roman Radin
Prof. Tiago Felski – Algoritmos e Programação II (UNIVALI)

## (a) Versão
Linux (usa `termios` no `getch`, igual à M1).

    g++ -Wall -o labirinto labirinto.cpp
    ./labirinto        # executar na pasta onde estão os mapaN.csv / mapaN.sol

## Arquivos
`labirinto.cpp`, `mapa1..5.csv`, `mapa1..5.sol`. O jogo cria `jogo_salvo.csv` e `recordes.csv`.

## (b) Formato do jogo_salvo.csv
Uma informação por linha: número do mapa, n, orientação (0 a 3), movimentos, rotações,
código da célula sob o jogador (ex.: 4 = alavanca), título, e depois as n linhas do cenário
(valores separados por vírgula; o jogador é o 2).

A posição do jogador não é gravada: é recalculada procurando o código 2 no cenário.
Arquivo ausente, vazio ou corrompido = "sem jogo salvo".

## (c) Antes/depois (M1 → M2)

| Função | M1 | M2 | Motivo |
|---|---|---|---|
| MoveJogador | `MoveJogador(int mapa[TAM][TAM], int &px, int &py, int &celulaSobJogador, char tecla, int orientacao, bool &venceu, bool &moveu)` | `int MoveJogador(Jogo &j, char tecla)` | o estado vive na struct; o resultado vai no retorno |
| ProcessaRotacao | `ProcessaRotacao(mapa, orientacao, px, py, celulaSobJogador, tecla, girou)` e imprimia mensagem | `int ProcessaRotacao(Jogo &j, char tecla)` | regra não imprime; devolve `RES_SEM_ALAVANCA` e a tela mostra a mensagem |
| AplicaGravidade | `AplicaGravidade(int mapa[TAM][TAM], int orientacao)` | `bool AplicaGravidade(Jogo &j)` | usa `j.n` (n não é mais 11) e devolve se esmagou o jogador |
| CarregaMapa | `CarregaMapa(int mapa[TAM][TAM], int numeroMapa)` com mapas no código | `bool CarregaMapa(Jogo &j, int numero, string &erro)` | lê `mapaN.csv`, valida e devolve o motivo |
| JogaFase | `JogaFase(mapa, numeroMapa, orientacao, px, py, celulaSobJogador, movimentos, rotacoes, jogoEmAndamento)` | `void JogaFase(Jogo &j)` | 9 parâmetros viraram 1 struct |
| LocalizaJogador | `LocalizaJogador(mapa, int &linha, int &coluna)` | `Posicao LocalizaJogador(const Jogo &j)` | devolve uma struct Posicao |

## (d) Defeitos da M1 corrigidos
(conferir com a lista da correção do professor e ajustar se precisar)
- `ProcessaRotacao` imprimia na tela e esperava tecla dentro da regra do jogo.
- `rand()` sem `srand()`: o mapa "aleatório" era sempre o mesmo.
- O movimento não checava os limites da matriz (mapa sem parede na borda travava).
- `CelulaSustentaBloco` tinha um parâmetro que não era usado.
- Os mapas 2 e 3 começavam com bloco no ar (instáveis): foi colocada uma parede embaixo do bloco.
- Não existia derrota por esmagamento: agora bloco que cai em cima do jogador é derrota.

## (e) Decisões de projeto
- **Struct por referência** nas funções que alteram o jogo (`ProcessaTecla`, `AplicaGravidade`, `JogaFase`). **Por valor** só em `MapaEstavel`, porque ali a cópia é o objetivo: a gravidade roda na cópia e o original fica intacto. Copiar a struct custa copiar a matriz 21x21 inteira, por isso não se faz isso à toa.
- **Posição do jogador**: fica guardada em `j.jogador` por rapidez, mas é recalculada depois de cada giro, porque a matriz gira e o campo ficaria errado. No jogo salvo não é gravada.
- **Quando salvar**: só ao sair com ESC/X. Gravar a cada movimento seria mais seguro, mas desnecessário aqui.
- **Mesmas regras no teclado e na solução**: as duas chamam `ProcessaTecla`; nada de regra escrito duas vezes.
- **Desempate no ranking**: menos movimentos; empate, menos rotações. Empate total mantém a ordem de chegada.
- **Mapas novos**: `mapa4` (15x15, bloco tampando a passagem + porta vertical) e `mapa5` (três portas em sequência, exige 3 rotações). Submeter ao professor antes.

## (f) Desafios extras
Nenhum.
