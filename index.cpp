// =====================================================================
// LABIRINTO GIRATORIO - M2
// Equipe: Arthur Ferraz de Aquino & Vicenzo Roman Radin
// Prof. Tiago Felski | Algoritmos e Programacao II | Versao: Linux
// "// M2:" marca o que foi trocado ou criado em relacao a M1.
// Compilar: g++ -Wall -o labirinto labirinto.cpp   |   Rodar: ./labirinto
// =====================================================================

#include <iostream>
#include <fstream>   // M2
#include <sstream>   // M2
#include <string>    // M2
#include <cctype>    // M2 (toupper)
#include <cstdlib>
#include <cstdio>
#include <ctime>     // M2 (srand)
#include <termios.h>
#include <unistd.h>
using namespace std;

const int TAM = 21;           // M2: era #define TAM 11 (agora e o tamanho maximo)
const int MAX_RECORDES = 200; // M2

const int VAZIO   = 0;
const int PAREDE  = 1;
const int JOGADOR = 2;
const int BLOCO   = 3;
const int ALAVANCA = 4;
const int SAIDA   = 5;
const int PORTA_H = 6; // porta horizontal: fica fechada com o mapa deitado (orientacao 0 ou 2)
const int PORTA_V = 7; // porta vertical: fica fechada com o mapa em pe (orientacao 1 ou 3)

// M2: o que aconteceu depois de uma tecla
const int RES_OK = 0;
const int RES_VENCEU = 1;
const int RES_DERROTA = 2;
const int RES_SEM_ALAVANCA = 3;
const int RES_INVALIDA = 4;

// M2: structs (na M1 eram proibidas)
struct Posicao {
    int lin, col;
};

struct Jogo {
    int cenario[TAM][TAM]; // so as n x n primeiras posicoes sao usadas
    int n;                 // dimensao do mapa (de 11 ate TAM)
    int orientacao;        // 0, 1, 2 ou 3 (multiplos de 90 graus)
    Posicao jogador;
    int movimentos, rotacoes;
    int numeroMapa;
    string titulo;
    int sobJogador;        // era a variavel solta celulaSobJogador da M1
};

struct Recorde {
    string nome;
    int mapa, movimentos, rotacoes;
};

// Le uma tecla do teclado sem precisar enter
int getch(void) {
    int ch;
    termios oldt;
    termios newt;
    tcgetattr(STDIN_FILENO, &oldt);// guarda as configuracoes antigas
    newt = oldt;// copia as configuracoes antigas para as novas
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); // aplica as novas configuracoes
    ch = getchar();// pega a tecla
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // restaura as configuracoes antigas
    return ch;
}

// Para a tela ate o jogador apertar uma tecla
void AguardaTecla() {
    cout << "(pressione uma tecla para continuar) ";
    getch();
}

// Limpa a tela do terminal /clear
void LimpaTela() {
    system("clear");
}

// M2: troca dois valores de qualquer tipo (usada para ordenar os recordes)
template <typename T>
void Trocar(T &a, T &b) {
    T aux = a;
    a = b;
    b = aux;
}

// M2: texto -> numero. Devolve -1 se nao for numero (assim arquivo estragado nao trava o jogo)
int ParaInt(const string &s) {
    if (s.empty() || s.size() > 9) return -1;
    for (unsigned int i = 0; i < s.size(); i++)
        if (s[i] < '0' || s[i] > '9') return -1;
    return stoi(s);
}

// M2: le uma linha do arquivo tirando o \r (arquivo feito no Windows)
bool LeLinha(ifstream &arq, string &linha) {
    if (!getline(arq, linha)) return false;
    if (!linha.empty() && linha[linha.size() - 1] == '\r') linha.pop_back();
    return true;
}

// ---------------------------------------------------------------------
// REGRAS (alteram o Jogo, nao imprimem, nao acessam arquivo)
// ---------------------------------------------------------------------

// Diz se uma porta (6 ou 7) esta fechada para a orientacao atual do mapa
bool PortaFechada(int valor, int orientacao) {
    if (valor == PORTA_H) return (orientacao == 0 || orientacao == 2); // M2: antes 0 e 180
    if (valor == PORTA_V) return (orientacao == 1 || orientacao == 3); // M2: antes 90 e 270
    return false;
}

// Diz se o jogador pode andar para a casa
// M2: recebe so o valor da celula (antes recebia a matriz inteira)
bool CelulaAtravessavel(int valor, int orientacao) {
    if (valor == PAREDE || valor == BLOCO) return false;
    if (valor == PORTA_H || valor == PORTA_V) return !PortaFechada(valor, orientacao);
    return true; //atravessa o resto
}

// M2: antes recebia matriz, linha, coluna e orientacao (que nem era usada)
bool CelulaSustentaBloco(int valor) {
    if (valor == PAREDE || valor == JOGADOR || valor == BLOCO) return true; //segura
    if (valor == SAIDA || valor == PORTA_H || valor == PORTA_V) return true; //nao apaga
    return false;
}

// Procura o jogador dentro do mapa e guarda onde ele esta
// M2: antes recebia matriz + linha + coluna; agora grava em j.jogador
void LocalizaJogador(Jogo &j) {
    for (int i = 0; i < j.n; i++)            // M2: j.n em vez de TAM
        for (int k = 0; k < j.n; k++)
            if (j.cenario[i][k] == JOGADOR) { j.jogador.lin = i; j.jogador.col = k; }
} //sem break

// M2: antes tinha 8 parametros (mapa, px, py, celulaSobJogador, tecla, orientacao, venceu, moveu)
int MoveJogador(Jogo &j, char tecla) {
    int novaX = j.jogador.lin;
    int novaY = j.jogador.col;
    if (tecla == 'W') novaX--;
    if (tecla == 'S') novaX++;
    if (tecla == 'A') novaY--;
    if (tecla == 'D') novaY++;

    // M2: protecao de limite (mapa de arquivo pode nao ter parede na borda)
    if (novaX < 0 || novaX >= j.n || novaY < 0 || novaY >= j.n) return RES_OK;
    if (!CelulaAtravessavel(j.cenario[novaX][novaY], j.orientacao)) return RES_OK;

    int destinoAntigo = j.cenario[novaX][novaY];
    j.cenario[j.jogador.lin][j.jogador.col] = j.sobJogador;
    j.sobJogador = destinoAntigo;
    j.cenario[novaX][novaY] = JOGADOR;
    j.jogador.lin = novaX;
    j.jogador.col = novaY;
    j.movimentos++;                          // M2: contador agora fica dentro do Jogo

    //identifica se o jogador esta em cima da saida o que daria a vitoria
    if (destinoAntigo == SAIDA) return RES_VENCEU;
    return RES_OK;
}

// M2: gira o cenario n x n (antes eram duas matrizes TAM x TAM de parametro)
void GiraDireita(Jogo &j) {
    int temp[TAM][TAM];
    for (int i = 0; i < j.n; i++)
        for (int k = 0; k < j.n; k++)
            temp[k][j.n - 1 - i] = j.cenario[i][k]; // primeira coluna da matriz original vira a primeira linha da matriz girada
    for (int i = 0; i < j.n; i++)
        for (int k = 0; k < j.n; k++)
            j.cenario[i][k] = temp[i][k];
}

void GiraEsquerda(Jogo &j) {
    int temp[TAM][TAM];
    for (int i = 0; i < j.n; i++)
        for (int k = 0; k < j.n; k++)
            temp[j.n - 1 - k][i] = j.cenario[i][k];
    for (int i = 0; i < j.n; i++)
        for (int k = 0; k < j.n; k++)
            j.cenario[i][k] = temp[i][k];
}

// M2: agora devolve true se um bloco caiu em cima do jogador (esmagou)
bool AplicaGravidade(Jogo &j) {
    bool esmagou = false;
    bool mudou = true;
    while (mudou) { // ve se o bloco se mexeu
        mudou = false;
        for (int i = j.n - 2; i >= 0; i--) { // 1 bloco por vez  (M2: j.n em vez de TAM)
            for (int k = 0; k < j.n; k++) {
                if (j.cenario[i][k] == BLOCO && !CelulaSustentaBloco(j.cenario[i + 1][k])) {
                    j.cenario[i + 1][k] = BLOCO; // se nao segura ele desce mais uma
                    j.cenario[i][k] = VAZIO;
                    mudou = true;
                    if (i + 2 < j.n && j.cenario[i + 2][k] == JOGADOR) esmagou = true; // M2
                }
            }
        }
    }
    return esmagou;
}

// M2: antes tinha 7 parametros e imprimia a mensagem aqui dentro.
// Agora so aplica a regra; quem imprime e a tela (JogaFase)
int ProcessaRotacao(Jogo &j, char tecla) {
    if (j.sobJogador != ALAVANCA) return RES_SEM_ALAVANCA;

    if (tecla == 'E') {
        GiraDireita(j);
        j.orientacao = (j.orientacao + 1) % 4; // M2: antes (orientacao + 90) % 360
    } else {
        GiraEsquerda(j);
        j.orientacao = (j.orientacao + 3) % 4; // M2: antes (orientacao + 270) % 360
    }
    LocalizaJogador(j);
    j.rotacoes++;
    if (AplicaGravidade(j)) return RES_DERROTA; // M2
    return RES_OK;
}

// M2: UMA funcao so para tratar tecla (teclado e reproducao de solucao usam esta mesma)
int ProcessaTecla(Jogo &j, char tecla) {
    tecla = (char) toupper(tecla);
    if (tecla == 'W' || tecla == 'A' || tecla == 'S' || tecla == 'D') return MoveJogador(j, tecla);
    if (tecla == 'Q' || tecla == 'E') return ProcessaRotacao(j, tecla);
    return RES_INVALIDA;
}

// M2: mapa estavel = a gravidade nao mexe em nada. O Jogo vem POR VALOR de proposito:
// a gravidade roda na copia e o original fica intacto
bool MapaEstavel(Jogo j) {
    Jogo copia = j;
    AplicaGravidade(copia);
    for (int i = 0; i < j.n; i++)
        for (int k = 0; k < j.n; k++)
            if (copia.cenario[i][k] != j.cenario[i][k]) return false;
    return true;
}

// M2: executa a solucao sem desenhar. pos = posicao da tecla onde parou
int ExecutaSolucao(Jogo &j, const string &teclas, int &pos) {
    for (pos = 1; pos <= (int) teclas.size(); pos++) {
        int r = ProcessaTecla(j, teclas[pos - 1]);
        if (r == RES_VENCEU || r == RES_DERROTA || r == RES_INVALIDA) return r;
    }
    return RES_OK; // as teclas acabaram
}

// M2: texto do resultado de uma solucao
string DescreveResultado(int res, int pos, const Jogo &j) {
    if (res == RES_VENCEU)
        return "solucao valida (" + to_string(pos) + " teclas, " + to_string(j.movimentos) +
               " movimentos, " + to_string(j.rotacoes) + " rotacoes)";
    if (res == RES_DERROTA) return "derrota na tecla " + to_string(pos);
    if (res == RES_INVALIDA) return "tecla invalida na posicao " + to_string(pos);
    return "as teclas acabaram e o jogador nao chegou a saida";
}

// ---------------------------------------------------------------------
// ARQUIVOS (so leem e gravam) - tudo M2
// ---------------------------------------------------------------------

string NomeArquivo(int numero, string extensao) {
    return "mapa" + to_string(numero) + extensao; // mapa3.csv, mapa3.sol...
}

// le as n linhas do cenario (n valores de 0 a 7 separados por virgula)
bool LeCenario(ifstream &arq, Jogo &j, string &erro) {
    for (int i = 0; i < j.n; i++) {
        string linha, campo;
        if (!LeLinha(arq, linha)) {
            erro = "faltam linhas do cenario";
            return false;
        }
        stringstream ss(linha);
        int col = 0;
        while (getline(ss, campo, ',')) {
            int valor = ParaInt(campo);
            if (valor < 0 || valor > 7) {
                erro = "valor nao numerico ou fora de 0 a 7";
                return false;
            }
            if (col >= j.n) {
                erro = "linha com colunas demais";
                return false;
            }
            j.cenario[i][col] = valor;
            col++;
        }
        if (col != j.n) {
            erro = "linha com quantidade de colunas diferente de n";
            return false;
        }
    }
    return true;
}

// exatamente 1 jogador e pelo menos 1 saida
bool ValidaMapa(const Jogo &j, string &erro) {
    int jogadores = 0, saidas = 0;
    for (int i = 0; i < j.n; i++)
        for (int k = 0; k < j.n; k++) {
            if (j.cenario[i][k] == JOGADOR) jogadores++;
            if (j.cenario[i][k] == SAIDA) saidas++;
        }
    if (jogadores != 1) { erro = "quantidade de jogadores diferente de 1"; return false; }
    if (saidas < 1) { erro = "mapa sem saida"; return false; }
    return true;
}

// M2: antes copiava matrizes escritas no codigo; agora le mapaN.csv, valida e devolve o motivo
bool CarregaMapa(Jogo &j, int numero, string &erro) {
    ifstream arq(NomeArquivo(numero, ".csv"));
    if (!arq.is_open()) { erro = "arquivo nao encontrado"; return false; }

    string linha;
    if (!LeLinha(arq, linha)) { erro = "arquivo vazio"; return false; }
    j.n = ParaInt(linha);
    if (j.n < 0) { erro = "dimensao ausente ou nao numerica"; return false; }
    if (j.n < 11 || j.n > TAM) { erro = "dimensao fora do intervalo de 11 ate " + to_string(TAM); return false; }

    if (!LeLinha(arq, j.titulo) || j.titulo == "") { erro = "titulo ausente"; return false; }
    if (!LeCenario(arq, j, erro)) return false;
    if (!ValidaMapa(j, erro)) return false;

    LocalizaJogador(j);
    j.sobJogador = VAZIO;
    j.orientacao = 0;
    j.movimentos = 0;
    j.rotacoes = 0;
    j.numeroMapa = numero;
    if (!MapaEstavel(j)) { erro = "mapa instavel (bloco no ar)"; return false; }
    return true;
}

// descobre quantos mapas existem: tenta abrir mapa1.csv, mapa2.csv... ate faltar um
int ContaMapas() {
    int total = 0;
    while (true) {
        ifstream arq(NomeArquivo(total + 1, ".csv"));
        if (!arq.is_open()) return total;
        total++;
    }
}

// carrega o mapa e a linha de teclas do .sol
bool PreparaSolucao(int numero, Jogo &j, string &teclas, string &erro) {
    if (!CarregaMapa(j, numero, erro)) {
        erro = "mapa invalido (" + erro + ")";
        return false;
    }
    ifstream arq(NomeArquivo(numero, ".sol"));
    if (!arq.is_open()) { erro = "arquivo .sol inexistente"; return false; }
    if (!LeLinha(arq, teclas) || teclas == "") { erro = "arquivo .sol vazio"; return false; }
    return true;
}

// formato do jogo_salvo.csv (uma informacao por linha):
// numero do mapa, n, orientacao, movimentos, rotacoes, celula sob o jogador, titulo, e as n linhas do cenario
bool SalvaJogo(const Jogo &j) {
    ofstream arq("jogo_salvo.csv");
    if (!arq.is_open()) return false;
    arq << j.numeroMapa << "\n" << j.n << "\n" << j.orientacao << "\n";
    arq << j.movimentos << "\n" << j.rotacoes << "\n" << j.sobJogador << "\n" << j.titulo << "\n";
    for (int i = 0; i < j.n; i++) {
        for (int k = 0; k < j.n; k++) {
            arq << j.cenario[i][k];
            if (k < j.n - 1) arq << ",";
        }
        arq << "\n";
    }
    return true;
}

// arquivo ausente, vazio ou corrompido = false (sem travar)
bool CarregaJogoSalvo(Jogo &j) {
    ifstream arq("jogo_salvo.csv");
    if (!arq.is_open()) return false;
    Jogo t;
    string linha, erro;
    int v[6];
    for (int i = 0; i < 6; i++) {
        if (!LeLinha(arq, linha)) return false;
        v[i] = ParaInt(linha);
        if (v[i] < 0) return false;
    }
    t.numeroMapa = v[0];
    t.n = v[1];
    t.orientacao = v[2];
    t.movimentos = v[3];
    t.rotacoes = v[4];
    t.sobJogador = v[5];
    if (t.n < 11 || t.n > TAM || t.orientacao > 3) return false;
    if (!LeLinha(arq, t.titulo)) return false;
    if (!LeCenario(arq, t, erro) || !ValidaMapa(t, erro)) return false;
    LocalizaJogador(t);
    j = t;
    return true;
}

bool ExisteJogoSalvo() {
    Jogo t;
    return CarregaJogoSalvo(t);
}

// acrescenta uma linha no fim de recordes.csv (o arquivo nasce na primeira vitoria)
bool RegistraRecorde(const Recorde &r) {
    ofstream arq("recordes.csv", ios_base::app);
    if (!arq.is_open()) return false;
    arq << r.nome << "," << r.mapa << "," << r.movimentos << "," << r.rotacoes << "\n";
    return true;
}

// le recordes.csv (ate max linhas); linha estragada e ignorada. Devolve quantos leu
int CarregaRecordes(Recorde v[], int max) {
    ifstream arq("recordes.csv");
    int qtd = 0;
    string linha, mapa, mov, rot;
    while (qtd < max && LeLinha(arq, linha)) {
        stringstream ss(linha);
        Recorde r;
        if (getline(ss, r.nome, ',') && getline(ss, mapa, ',') && getline(ss, mov, ',') && getline(ss, rot, ',')) {
            r.mapa = ParaInt(mapa);
            r.movimentos = ParaInt(mov);
            r.rotacoes = ParaInt(rot);
            if (r.mapa >= 0 && r.movimentos >= 0 && r.rotacoes >= 0) {
                v[qtd] = r;
                qtd++;
            }
        }
    }
    return qtd;
}

// menos movimentos primeiro; empate: menos rotacoes (bubble sort com o template Trocar)
void OrdenaRecordes(Recorde v[], int qtd) {
    for (int i = 0; i < qtd - 1; i++)
        for (int k = 0; k < qtd - 1 - i; k++)
            if (v[k].movimentos > v[k + 1].movimentos ||
                (v[k].movimentos == v[k + 1].movimentos && v[k].rotacoes > v[k + 1].rotacoes))
                Trocar(v[k], v[k + 1]);
}

// ---------------------------------------------------------------------
// TELA (so imprime)
// ---------------------------------------------------------------------

// M2: recebe o Jogo (antes: matriz e orientacao soltas)
void DesenhaCenario(const Jogo &j) {
    for (int i = 0; i < j.n; i++) {          // M2: j.n em vez de TAM
        for (int k = 0; k < j.n; k++) {
            switch (j.cenario[i][k]) {
                case VAZIO:   cout << "  "; break;
                case PAREDE:  cout << "# "; break;
                case JOGADOR: cout << "@ "; break;
                case BLOCO:   cout << "0 "; break;
                case ALAVANCA:cout << "A "; break;
                case SAIDA:   cout << "S "; break;
                case PORTA_H: cout << (PortaFechada(PORTA_H, j.orientacao) ? "= " : ": "); break;
                case PORTA_V: cout << (PortaFechada(PORTA_V, j.orientacao) ? "| " : "; "); break;
            } // op ternario ? true fechada | : false aberta
        }
        cout << endl;
    }
}

// M2: recebe o Jogo (antes: 4 parametros soltos)
void ExibeStatus(const Jogo &j) {
    cout << "Mapa: " << j.numeroMapa << " | Orientacao: " << j.orientacao * 90   // M2: orientacao agora e 0 a 3
         << " graus | Movimentos: " << j.movimentos << " | Rotacoes: " << j.rotacoes << endl;
}

// M2: menu novo (6 opcoes)
void ExibeMenu(bool temSalvo) {
    LimpaTela();
    cout << "========= MENU =========" << endl;
    cout << "1- Novo jogo" << endl;
    cout << "2- Continuar o jogo" << (temSalvo ? "" : " (indisponivel)") << endl;
    cout << "3- Ver solucao" << endl;
    cout << "4- Recordes" << endl;
    cout << "5- Sobre" << endl;
    cout << "6- Fim" << endl;
    cout << "========================" << endl;
    cout << "Opcao: ";
}

// M2: texto novo (mes/ano, novas opcoes, ESC)
void ExibeSobre() {
    LimpaTela();
    cout << "Equipe: Arthur Ferraz de Aquino & Vicenzo Roman Radin" << endl;
    cout << "Outubro/2026 | Prof. Tiago Felski | Algoritmos e Programacao II" << endl;
    cout << "OBJETIVO: leve o jogador (@) ate a saida (S)." << endl;
    cout << "REGRAS: o mapa so gira com o jogador em cima de uma alavanca (A)." << endl;
    cout << "  Ao girar, os blocos (0) caem; bloco que cai em cima do jogador esmaga (derrota)." << endl;
    cout << "  Portas fechadas: = e |   Portas abertas: : e ;" << endl;
    cout << "TECLAS: W A S D mover | Q/E girar (so na alavanca) | R reiniciar | ESC volta ao menu e salva" << endl;
    cout << "NOVAS OPCOES: Continuar (jogo salvo em arquivo) | Ver solucao (executa a solucao do mapa" << endl;
    cout << "  ou verifica todos) | Recordes (5 melhores de cada mapa)" << endl;
}

// M2: mostra os mapas; os invalidos aparecem como indisponiveis, com o motivo
void ExibeListaMapas(int total) {
    for (int i = 1; i <= total; i++) {
        Jogo j;
        string erro;
        if (CarregaMapa(j, i, erro)) cout << i << "- " << j.titulo << endl;
        else cout << i << "- INDISPONIVEL: " << erro << endl;
    }
}

// M2: 5 melhores do mapa (a lista ja vem ordenada)
void ExibeRanking(const Recorde v[], int qtd, int mapa) {
    cout << "===== RECORDES DO MAPA " << mapa << " =====" << endl;
    int mostrados = 0;
    for (int i = 0; i < qtd && mostrados < 5; i++)
        if (v[i].mapa == mapa) {
            mostrados++;
            cout << mostrados << ". " << v[i].nome << " - " << v[i].movimentos << " movimentos, "
                 << v[i].rotacoes << " rotacoes" << endl;
        }
    if (mostrados == 0) cout << "Ainda nao ha recordes para este mapa. Seja o primeiro a vencer!" << endl;
}

// M2: reproduz a solucao desenhando a cada tecla (usa a mesma ProcessaTecla do teclado)
int ReproduzSolucao(Jogo &j, const string &teclas, int &pos) {
    for (pos = 1; pos <= (int) teclas.size(); pos++) {
        LimpaTela();
        ExibeStatus(j);
        DesenhaCenario(j);
        cout << "Tecla " << pos << " de " << teclas.size() << ": " << teclas[pos - 1] << endl;
        usleep(400000); // pausa de 0,4 segundo
        int r = ProcessaTecla(j, teclas[pos - 1]);
        if (r == RES_VENCEU || r == RES_DERROTA || r == RES_INVALIDA) return r;
    }
    return RES_OK;
}

// M2: executa (sem desenhar) a solucao de cada mapa e lista o resultado
void VerificaTodos(int total) {
    for (int i = 1; i <= total; i++) {
        Jogo j;
        string teclas, erro;
        bool ok = PreparaSolucao(i, j, teclas, erro);
        cout << "Mapa " << i << " - " << (ok ? j.titulo : "") << ": ";
        if (!ok) cout << erro << endl;
        else {
            int pos;
            int res = ExecutaSolucao(j, teclas, pos);
            cout << DescreveResultado(res, pos, j) << endl;
        }
    }
}

// ---------------------------------------------------------------------
// ENTRADA
// ---------------------------------------------------------------------

int LeDigito() {
    char tecla = (char) getch();
    cout << tecla << endl; // mostra o que foi digitado
    return tecla - '0'; //ASCII 3 = 51, 0 = 48
}

// M2: le numero com varios digitos (escolha de mapa). Invalido = -1
int LeNumero() {
    string linha;
    getline(cin, linha);
    return ParaInt(linha);
}

// M2: nome do recordista (ate 15 caracteres, sem virgula)
string PedeNome() {
    string nome, limpo = "";
    cout << "Digite seu nome (ate 15 letras, sem virgula): ";
    getline(cin, nome);
    for (unsigned int i = 0; i < nome.size() && limpo.size() < 15; i++)
        if (nome[i] != ',') limpo += nome[i];
    if (limpo == "") limpo = "Anonimo";
    return limpo;
}

// ---------------------------------------------------------------------
// FASE E OPCOES DO MENU
// ---------------------------------------------------------------------

// Loop principal de uma fase, mostra o mapa, le uma tecla do jogador e decide o que fazer com ela
// M2: recebe so o Jogo (antes eram 9 parametros)
void JogaFase(Jogo &j) {
    while (true) {
        LimpaTela();
        ExibeStatus(j);
        DesenhaCenario(j);
        cout << "Comando: ";
        char tecla = (char) getch();   // le a tecla na hora
        cout << tecla << endl;

        if (tecla == 27 || tecla == 'X' || tecla == 'x') { // M2: ESC (27) tambem volta ao menu
            if (SalvaJogo(j)) cout << "Jogo salvo. Escolha 'Continuar o jogo' no menu para retomar." << endl; // M2
            else cout << "Nao foi possivel salvar o jogo." << endl;
            AguardaTecla();
            break;
        }

        if (tecla == 'R' || tecla == 'r') {
            Jogo novo;            // M2: reinicia lendo o mapa do arquivo de novo
            string erro;
            if (CarregaMapa(novo, j.numeroMapa, erro)) j = novo;
            continue;
        }

        int r = ProcessaTecla(j, tecla); // M2: antes eram if separados para girar e mover

        if (r == RES_SEM_ALAVANCA) {
            // M2: a mensagem saiu da regra e veio para a tela
            cout << "Voce precisa estar em cima da alavanca (A) para girar o mapa." << endl;
            AguardaTecla();
        } else if (r == RES_VENCEU) {
            ExibeStatus(j);
            DesenhaCenario(j);
            cout << "PARABENS, VOCE VENCEU!" << endl;
            remove("jogo_salvo.csv"); // M2: o jogo salvo dessa partida deixa de valer
            Recorde rec;              // M2: registra o recorde
            rec.nome = PedeNome();
            rec.mapa = j.numeroMapa;
            rec.movimentos = j.movimentos;
            rec.rotacoes = j.rotacoes;
            RegistraRecorde(rec);
            AguardaTecla();
            break;
        } else if (r == RES_DERROTA) { // M2
            ExibeStatus(j);
            DesenhaCenario(j);
            cout << "VOCE FOI ESMAGADO POR UM BLOCO! Fim de jogo." << endl;
            AguardaTecla();
            break;
        }
    }
}

// M2: opcao 1
void OpcaoNovoJogo(Jogo &jogo) {
    int total = ContaMapas();
    LimpaTela();
    ExibeListaMapas(total);
    cout << "Mapa (numero) ou 0 aleatorio: ";
    int numero = LeNumero();

    Jogo novo;
    string erro;
    bool ok = false;
    if (numero == 0 && total > 0) {
        for (int t = 0; t < 100 && !ok; t++) ok = CarregaMapa(novo, rand() % total + 1, erro); // sorteia entre os validos
    } else {
        ok = CarregaMapa(novo, numero, erro);
    }
    if (!ok) {
        cout << "Nao foi possivel carregar o mapa: " << erro << endl;
        AguardaTecla();
        return;
    }

    if (ExisteJogoSalvo()) {
        cout << "Ja existe um jogo salvo. Substituir? (s/n): ";
        char c = (char) getch();
        cout << c << endl;
        if (c != 's' && c != 'S') return;
        remove("jogo_salvo.csv");
    }
    jogo = novo;
    JogaFase(jogo);
}

// M2: opcao 2 (antes dependia de uma variavel na memoria)
void OpcaoContinuar(Jogo &jogo) {
    if (!CarregaJogoSalvo(jogo)) {
        cout << "Nenhum jogo salvo valido." << endl;
        AguardaTecla();
        return;
    }
    JogaFase(jogo);
}

// M2: opcao 3
void OpcaoVerSolucao() {
    int total = ContaMapas();
    LimpaTela();
    ExibeListaMapas(total);
    cout << "Mapa (numero) ou 0 para verificar todos: ";
    int numero = LeNumero();

    if (numero == 0) {
        VerificaTodos(total);
    } else {
        Jogo j;
        string teclas, erro;
        if (!PreparaSolucao(numero, j, teclas, erro)) {
            cout << "Nao foi possivel reproduzir: " << erro << endl;
        } else {
            int pos;
            int res = ReproduzSolucao(j, teclas, pos);
            LimpaTela();
            ExibeStatus(j);
            DesenhaCenario(j);
            cout << "Resultado: " << DescreveResultado(res, pos, j) << endl;
        }
    }
    AguardaTecla();
}

// M2: opcao 4
void OpcaoRecordes() {
    LimpaTela();
    cout << "Recordes de qual mapa? ";
    int mapa = LeNumero();
    Recorde v[MAX_RECORDES];
    int qtd = CarregaRecordes(v, MAX_RECORDES);
    OrdenaRecordes(v, qtd);
    ExibeRanking(v, qtd, mapa);
    AguardaTecla();
}

// M2: main curta, so o menu (a logica foi para as Opcao...)
int main() {
    srand(time(0)); // M2: a M1 nao chamava srand, entao o "aleatorio" repetia sempre o mesmo
    Jogo jogo;
    int escolha;
    do {
        ExibeMenu(ExisteJogoSalvo());
        escolha = LeDigito();
        switch (escolha) {
            case 1: OpcaoNovoJogo(jogo); break;
            case 2: OpcaoContinuar(jogo); break;
            case 3: OpcaoVerSolucao(); break;
            case 4: OpcaoRecordes(); break;
            case 5: ExibeSobre(); AguardaTecla(); break;
            case 6: cout << "Fim de jogo" << endl; break;
        }
    } while (escolha != 6);
    return 0;
}
