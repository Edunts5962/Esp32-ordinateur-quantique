// quantique.h - le "processeur quantique" de la console : un simulateur exact
// du vecteur d'etat (jusqu'a 5 qubits, 32 amplitudes complexes).
//
// Un circuit est une grille colonnes x qubits. Dans une colonne :
//   - les portes a un qubit (H, X, Y, Z, S, S†, T, T†, Rx, Ry, Rz) agissent
//     sur leur fil ;
//   - les points de controle (●) controlent TOUTES les portes de la colonne
//     (● + X = CNOT, ● ● + X = Toffoli, ● + Z = CZ) ;
//   - deux croix (×) forment un SWAP ;
//   - M mesure le qubit (l'etat s'effondre).
//
// Chaque porte a un qubit s'ecrit V = e^(iα)·(cos(θ/2)·I − i·sin(θ/2)·n·σ) :
// une rotation d'angle θ autour de l'axe n de la sphere de Bloch. On peut donc
// appliquer une FRACTION u de la porte (V^u : rotation de θ·u), ce qui anime
// les portes en douceur, sur la sphere comme sur les amplitudes.
// Ce fichier ne depend pas d'Arduino : il tourne aussi sur PC.
#pragma once
#include <stdint.h>

namespace q {

const int NQ_MAX = 5;               // qubits de l'editeur
const int NE_MAX = 1 << NQ_MAX;     // amplitudes
const int NC_MAX = 24;              // colonnes d'un circuit

struct cplx { float re, im; };
inline cplx C(float r, float i = 0) { return { r, i }; }
inline cplx operator+(cplx a, cplx b) { return { a.re + b.re, a.im + b.im }; }
inline cplx operator-(cplx a, cplx b) { return { a.re - b.re, a.im - b.im }; }
inline cplx operator*(cplx a, cplx b) { return { a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re }; }
inline cplx operator*(float k, cplx a) { return { k * a.re, k * a.im }; }
inline cplx conj(cplx a) { return { a.re, -a.im }; }
inline float norme2(cplx a) { return a.re * a.re + a.im * a.im; }

enum Porte : uint8_t {
    P_VIDE = 0, P_H, P_X, P_Y, P_Z, P_S, P_SD, P_T, P_TD, P_RX, P_RY, P_RZ,
    P_CTRL, P_SWAP, P_MESURE, P_NB
};
const char *nom_porte(int p);       // "H", "X", "S†", "Rx"...
bool porte_angle(int p);            // Rx, Ry, Rz : un angle a regler
char code_porte(int p);             // caractere du format texte des circuits

struct Cellule { uint8_t p; float angle; };

struct Circuit {
    int nq;                          // 1..5
    int nc;                          // colonnes utilisees
    Cellule c[NC_MAX][NQ_MAX];
    int algo;                        // indice dans ALGOS (legendes), -1 = circuit libre
    char nom[28];
};

struct Etat {
    int nq;
    cplx a[NE_MAX];                  // a[i] : amplitude de |i>, bit k de i = qubit k
};

// ---- calcul -----------------------------------------------------------------
void etat_zero(Etat &e, int nq);
// applique la colonne col (fraction u de 0 a 1, sauf les mesures). Les mesures
// sont ignorees ici (voir mesurer_colonne).
void appliquer_colonne(Etat &e, const Circuit &c, int col, float u = 1);
// mesures de la colonne : tire chaque qubit mesure, effondre l'etat.
// bits : resultat (bit k = qubit k), masque : qubits mesures. Renvoie true s'il y en a.
bool mesurer_colonne(Etat &e, const Circuit &c, int col, uint32_t (*alea32)(), int &bits, int &masque);
// etat final "theorique" (les mesures ne changent pas les probabilites finales)
void executer(const Circuit &c, Etat &e, int jusqua = -1);
void probabilites(const Etat &e, float *p);
// vecteur de Bloch du qubit k (matrice densite reduite) : |r| < 1 si intrique
void bloch(const Etat &e, int k, float &x, float &y, float &z);
float purete(const Etat &e, int k);

// un tir complet : etat initial, portes, bruit, mesures intermediaires et finales.
// Renvoie l'entier mesure (qubits non mesures : mesures a la fin).
int tir(const Circuit &c, uint32_t (*alea32)(), float bruit_porte, float bruit_lecture);
bool a_mesures_intermediaires(const Circuit &c);

// ---- edition ----------------------------------------------------------------
void vider(Circuit &c, int nq);
void poser(Circuit &c, int col, int qb, int p, float angle = 1.5708f);
int colonnes_utiles(const Circuit &c);
// lit un circuit ecrit en texte : colonnes separees par '|', un caractere par qubit
//   . vide  H X Y Z  S s(S†) T t(T†)  r(Rx) y(Ry) z(Rz)  @ controle  x swap  M mesure
// les angles de r/y/z sont pris dans 'angles', dans l'ordre
void depuis_texte(Circuit &c, int nq, const char *txt, const float *angles = nullptr);

// ---- export OpenQASM 2.0 (IBM Quantum Composer) ------------------------------
// renvoie false si une porte n'existe pas dans qelib1 (ex. 3 controles)
bool vers_qasm(const Circuit &c, char *buf, int n);

// ---- bibliotheque d'algorithmes ------------------------------------------------
struct Algo {
    const char *nom;          // "Grover (2 qubits)"
    const char *court;        // pour les listes
    int nq;
    const char *texte;        // circuit
    float angles[4];
    const char *legendes[NC_MAX];   // explication de chaque colonne (pas a pas)
    const char *attendu;      // ce qu'on doit mesurer
};
extern const Algo ALGOS[];
extern const int NB_ALGOS;
void charger_algo(Circuit &c, int i);

}  // namespace q
