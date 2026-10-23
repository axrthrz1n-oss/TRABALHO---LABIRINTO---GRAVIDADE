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

const int TAM = 21;         
const int MAX_RECORDES = 200; // M2

const string EXT_MAPA = ".txt";
const string EXT_SOL = "_sol.txt";
const string ARQ_SALVO = "jogo_salvo.txt";
const string ARQ_RECORDES = "recordes.txt";

const int VAZIO   = 0;
const int PAREDE  = 1;
const int JOGADOR = 2;
const int BLOCO   = 3;
const int ALAVANCA = 4;
const int SAIDA   = 5;
const int PORTA_H = 6; // fechada orientacao 0 ou 2
const int PORTA_V = 7; // fechada orientacao 1 ou 3

const int RES_OK = 0;
const int RES_VENCEU = 1;
const int RES_DERROTA = 2;
const int RES_SEM_ALAVANCA = 3;
const int RES_INVALIDA = 4;

struct Posicao {
    int lin, col;
};

struct Jogo {
    int cenario[TAM][TAM]; 
    int n;                 
    int orientacao;       
    Posicao jogador;
    int movimentos, rotacoes;
    int numeroMapa;
    string titulo;
    int sobJogador;     
};

struct Recorde {
    string nome;
    int mapa, movimentos, rotacoes;
};

int getch(void) {
    cout << flush;
    termios oldt;
    termios newt;
    if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
        int ch = getchar();
        while (ch == '\n' || ch == '\r') ch = getchar(); 
        if (ch == EOF) return EOF;
        int resto = ch;
        while (resto != '\n' && resto != EOF) resto = getchar();
        return ch;
    }
    newt = oldt;// copia as configuracoes antigas para as novas
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); // aplica
    int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // restaura
    return ch;
}

void AguardaTecla() {
    cout << "(pressione uma tecla para continuar) ";
    getch();
}

void LimpaTela() {
    system("clear");
}

// M2: troca recordes
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

bool PortaFechada(int valor, int orientacao) {
    if (valor == PORTA_H) return (orientacao == 0 || orientacao == 2); 
    if (valor == PORTA_V) return (orientacao == 1 || orientacao == 3); 
    return false;
}

bool CelulaAtravessavel(int valor, int orientacao) {
    if (valor == PAREDE || valor == BLOCO) return false;
    if (valor == PORTA_H || valor == PORTA_V) return !PortaFechada(valor, orientacao);
    return true; 
}


bool CelulaSustentaBloco(int valor) {
    if (valor == PAREDE || valor == JOGADOR || valor == BLOCO) return true; 
    if (valor == SAIDA || valor == PORTA_H || valor == PORTA_V) return true; 
    return false;
}

void LocalizaJogador(Jogo &j) {
    for (int i = 0; i < j.n; i++)      
        for (int k = 0; k < j.n; k++)
            if (j.cenario[i][k] == JOGADOR) { j.jogador.lin = i; j.jogador.col = k; }
} 

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
    j.movimentos++; 

    if (destinoAntigo == SAIDA) return RES_VENCEU;
    return RES_OK;
}

void GiraDireita(Jogo &j) {
    int temp[TAM][TAM];
    for (int i = 0; i < j.n; i++)
        for (int k = 0; k < j.n; k++)
            temp[k][j.n - 1 - i] = j.cenario[i][k];
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


bool AplicaGravidade(Jogo &j) {
    bool esmagou = false;
    bool mudou = true;
    while (mudou) { 
        mudou = false;
        for (int i = j.n - 2; i >= 0; i--) { 
            for (int k = 0; k < j.n; k++) {
                if (j.cenario[i][k] == BLOCO && !CelulaSustentaBloco(j.cenario[i + 1][k])) {
                    j.cenario[i + 1][k] = BLOCO; 
                    j.cenario[i][k] = VAZIO;
                    mudou = true;
                    if (i + 2 < j.n && j.cenario[i + 2][k] == JOGADOR) esmagou = true;
                }
            }
        }
    }
    return esmagou;
}

int ProcessaRotacao(Jogo &j, char tecla) {
    if (j.sobJogador != ALAVANCA) return RES_SEM_ALAVANCA;

    if (tecla == 'E') {
        GiraDireita(j);
        j.orientacao = (j.orientacao + 1) % 4; 
    } else {
        GiraEsquerda(j);
        j.orientacao = (j.orientacao + 3) % 4; 
    }
    LocalizaJogador(j);
    j.rotacoes++;
    if (AplicaGravidade(j)) return RES_DERROTA; 
    return RES_OK;
}

int ProcessaTecla(Jogo &j, char tecla) {
    tecla = (char) toupper(tecla);
    if (tecla == 'W' || tecla == 'A' || tecla == 'S' || tecla == 'D') return MoveJogador(j, tecla);
    if (tecla == 'Q' || tecla == 'E') return ProcessaRotacao(j, tecla);
    return RES_INVALIDA;
}

bool MapaEstavel(Jogo j) {
    Jogo copia = j;
    AplicaGravidade(copia);
    for (int i = 0; i < j.n; i++)
        for (int k = 0; k < j.n; k++)
            if (copia.cenario[i][k] != j.cenario[i][k]) return false;
    return true;
}

// M2: executa posicao da tecla
int ExecutaSolucao(Jogo &j, const string &teclas, int &pos) {
    for (pos = 1; pos <= (int) teclas.size(); pos++) {
        int r = ProcessaTecla(j, teclas[pos - 1]);
        if (r == RES_VENCEU || r == RES_DERROTA || r == RES_INVALIDA) return r;
    }
    return RES_OK; // as teclas acabaram
}

// M2:solucao
string DescreveResultado(int res, int pos, const Jogo &j) {
    if (res == RES_VENCEU)
        return "solucao valida (" + to_string(pos) + " teclas, " + to_string(j.movimentos) +
               " movimentos, " + to_string(j.rotacoes) + " rotacoes)";
    if (res == RES_DERROTA) return "derrota na tecla " + to_string(pos);
    if (res == RES_INVALIDA) return "tecla invalida na posicao " + to_string(pos);
    return "as teclas acabaram e o jogador nao chegou a saida";
}

string NomeArquivo(int numero, string extensao) {
    return "mapa" + to_string(numero) + extensao; // mapa3.csv
}

// separa por virgulas
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
// 1 jogador 1 saida
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

bool CarregaMapa(Jogo &j, int numero, string &erro) {
    ifstream arq(NomeArquivo(numero, EXT_MAPA));
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

int ContaMapas() {
    int total = 0;
    while (true) {
        ifstream arq(NomeArquivo(total + 1, EXT_MAPA));
        if (!arq.is_open()) return total;
        total++;
    }
}

bool PreparaSolucao(int numero, Jogo &j, string &teclas, string &erro) {
    if (!CarregaMapa(j, numero, erro)) {
        erro = "mapa invalido (" + erro + ")";
        return false;
    }
    ifstream arq(NomeArquivo(numero, EXT_SOL));
    if (!arq.is_open()) { erro = "arquivo .sol inexistente"; return false; }
    string linha;
    if (!LeLinha(arq, linha)) { erro = "arquivo .sol vazio"; return false; }
    teclas = "";
    for (unsigned int i = 0; i < linha.size(); i++) // M2: ignora espaco/tab
        if (linha[i] != ' ' && linha[i] != '\t') teclas += linha[i];
    if (teclas == "") { erro = "arquivo .sol vazio"; return false; }
    return true;
}

bool SalvaJogo(const Jogo &j) {
    ofstream arq(ARQ_SALVO);
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
    arq.close();
    return !arq.fail();
}

bool CarregaJogoSalvo(Jogo &j) {
    ifstream arq(ARQ_SALVO);
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
    if (t.numeroMapa < 1) return false;                            
    if (t.n < 11 || t.n > TAM || t.orientacao > 3) return false;
    if (t.sobJogador == PAREDE || t.sobJogador == JOGADOR || t.sobJogador == BLOCO || t.sobJogador > PORTA_V)
        return false;                                           
    if (!LeLinha(arq, t.titulo) || t.titulo == "") return false;
    if (!LeCenario(arq, t, erro) || !ValidaMapa(t, erro)) return false;
    LocalizaJogador(t);
    j = t;
    return true;
}

bool ExisteJogoSalvo() {
    Jogo t;
    return CarregaJogoSalvo(t);
}

bool RegistraRecorde(const Recorde &r) {
    ofstream arq(ARQ_RECORDES, ios_base::app);
    if (!arq.is_open()) return false;
    arq << r.nome << "," << r.mapa << "," << r.movimentos << "," << r.rotacoes << "\n";
    arq.close();
    return !arq.fail();
}

int CarregaRecordes(Recorde v[], int max) {
    ifstream arq(ARQ_RECORDES);
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

//criterio desempate dos recordes menos movimentos primeiro; empate: menos rotacoes
void OrdenaRecordes(Recorde v[], int qtd) {
    for (int i = 0; i < qtd - 1; i++)
        for (int k = 0; k < qtd - 1 - i; k++)
            if (v[k].movimentos > v[k + 1].movimentos ||
                (v[k].movimentos == v[k + 1].movimentos && v[k].rotacoes > v[k + 1].rotacoes))
                Trocar(v[k], v[k + 1]);
}

void DesenhaCenario(const Jogo &j) {
    for (int i = 0; i < j.n; i++) {     
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
            }
        }
        cout << endl;
    }
}

void ExibeStatus(const Jogo &j) {
    cout << "Mapa: " << j.numeroMapa << " | Orientacao: " << j.orientacao * 90   // M2: orientacao agora e 0 a 3
         << " graus | Movimentos: " << j.movimentos << " | Rotacoes: " << j.rotacoes << endl;
}

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

void ExibeSobre() {
    LimpaTela();
    cout << "Equipe: Arthur Ferraz de Aquino & Vicenzo Roman Radin" << endl;
    cout << "Outubro/2026 | Prof. Tiago Felski | Algoritmos e Programacao II" << endl;
    cout << "OBJETIVO: leve o jogador (@) ate a saida (S)." << endl;
    cout << "REGRAS: o mapa so gira com o jogador em cima de uma alavanca (A)." << endl;
    cout << "  Ao girar, os blocos (0) caem; bloco que cai em cima do jogador esmaga (derrota)." << endl;
    cout << "  Portas fechadas: = e |   Portas abertas: : e ;" << endl;
    cout << "TECLAS: W A S D mover | Q/E girar (so na alavanca) | R reiniciar | ESC ou X volta ao menu e salva" << endl;
    cout << "NOVAS OPCOES: Continuar (jogo salvo em arquivo) | Ver solucao (executa a solucao do mapa" << endl;
    cout << "  ou verifica todos) | Recordes (5 melhores de cada mapa)" << endl;
}

void ExibeItemMapa(int numero, bool ok, const string &texto) {
    if (ok) cout << numero << "- " << texto << endl;
    else cout << numero << "- INDISPONIVEL: " << texto << endl;
}

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

// M2: reproduz a solucao desenhando a cada tecla
int ReproduzSolucao(Jogo &j, const string &teclas, int &pos) {
    for (pos = 1; pos <= (int) teclas.size(); pos++) {
        LimpaTela();
        ExibeStatus(j);
        DesenhaCenario(j);
        cout << "Tecla " << pos << " de " << teclas.size() << ": " << teclas[pos - 1] << endl;
        cout << flush;
        usleep(400000); // pausa
        int r = ProcessaTecla(j, teclas[pos - 1]);
        if (r == RES_VENCEU || r == RES_DERROTA || r == RES_INVALIDA) return r;
    }
    return RES_OK;
}

int LeDigito() {
    int tecla = getch();
    if (tecla == EOF) return 6; 
    cout << (char) tecla << endl;
    return tecla - '0';
}

int LeNumero() {
    cout << flush;
    string linha;
    while (getline(cin, linha)) {
        while (!linha.empty() && (linha[linha.size() - 1] == '\r' || linha[linha.size() - 1] == ' '))
            linha.pop_back();
        if (!linha.empty()) return ParaInt(linha);
    }
    return -1;
}

// M2: nome
string PedeNome() {
    string nome, limpo = "";
    cout << "Digite seu nome (ate 15 letras, sem virgula): " << flush;
    getline(cin, nome);
    for (unsigned int i = 0; i < nome.size() && limpo.size() < 15; i++)
        if (nome[i] != ',' && nome[i] != '\r') limpo += nome[i];
    if (limpo == "") limpo = "Anonimo";
    return limpo;
}

void JogaFase(Jogo &j) {
    while (true) {
        LimpaTela();
        ExibeStatus(j);
        DesenhaCenario(j);
        cout << "Comando: ";
        int tecla = getch(); 
        if (tecla != EOF) cout << (char) tecla << endl;

        if (tecla == EOF || tecla == 27 || tecla == 'X' || tecla == 'x') { 
            if (SalvaJogo(j)) cout << "Jogo salvo. Escolha 'Continuar o jogo' no menu para retomar." << endl;
            else cout << "Nao foi possivel salvar o jogo." << endl;
            AguardaTecla();
            break;
        }

        if (tecla == 'R' || tecla == 'r') {
            Jogo novo;            
            string erro;
            if (CarregaMapa(novo, j.numeroMapa, erro)) j = novo;
            continue;
        }

        int r = ProcessaTecla(j, (char) tecla);

        if (r == RES_SEM_ALAVANCA) {
            cout << "Voce precisa estar em cima da alavanca (A) para girar o mapa." << endl;
            AguardaTecla();
        } else if (r == RES_VENCEU) {
            ExibeStatus(j);
            DesenhaCenario(j);
            cout << "PARABENS, VOCE VENCEU!" << endl;
            remove(ARQ_SALVO.c_str());
            Recorde rec;              // M2: recorde
            rec.nome = PedeNome();
            rec.mapa = j.numeroMapa;
            rec.movimentos = j.movimentos;
            rec.rotacoes = j.rotacoes;
            if (!RegistraRecorde(rec)) cout << "Nao foi possivel gravar o recorde." << endl;
            AguardaTecla();
            break;
        } else if (r == RES_DERROTA) {
            ExibeStatus(j);
            DesenhaCenario(j);
            cout << "VOCE FOI ESMAGADO POR UM BLOCO! Fim de jogo." << endl;
            AguardaTecla();
            break;
        }
    }
}

void ListaMapas(int total) {
    for (int i = 1; i <= total; i++) {
        Jogo j;
        string erro;
        if (CarregaMapa(j, i, erro)) ExibeItemMapa(i, true, j.titulo);
        else ExibeItemMapa(i, false, erro);
    }
}

// M2: executa a solucao de cada mapa e lista o resultado
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

void OpcaoNovoJogo(Jogo &jogo) {
    int total = ContaMapas();
    LimpaTela();
    ListaMapas(total);
    cout << "Mapa (numero) ou 0 aleatorio: ";
    int numero = LeNumero();

    Jogo novo;
    string erro;
    bool ok = false;
    if (numero == 0 && total > 0) {
        for (int t = 0; t < 100 && !ok; t++) ok = CarregaMapa(novo, rand() % total + 1, erro); // sorteia
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
        int c = getch();
        if (c == EOF) return;
        cout << (char) c << endl;
        if (c != 's' && c != 'S') return;
        remove(ARQ_SALVO.c_str());
    }
    jogo = novo;
    JogaFase(jogo);
}

void OpcaoContinuar(Jogo &jogo) {
    if (!CarregaJogoSalvo(jogo)) {
        cout << "Nenhum jogo salvo valido." << endl;
        AguardaTecla();
        return;
    }
    JogaFase(jogo);
}

void OpcaoVerSolucao() {
    int total = ContaMapas();
    LimpaTela();
    ListaMapas(total);
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

int main() {
    srand(time(0)); 
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
