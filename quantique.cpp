// quantique.cpp - simulateur du vecteur d'etat, export OpenQASM, algorithmes
#include "quantique.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

namespace q {

static const float PI_ = 3.14159265358979f;

const char *nom_porte(int p)
{
    static const char *N[P_NB] = { "", "H", "X", "Y", "Z", "S", "S†", "T", "T†", "Rx", "Ry", "Rz", "●", "×", "M" };
    return p >= 0 && p < P_NB ? N[p] : "?";
}

bool porte_angle(int p) { return p == P_RX || p == P_RY || p == P_RZ; }

char code_porte(int p)
{
    static const char K[P_NB + 1] = ".HXYZSsTtryz@xM";
    return p >= 0 && p < P_NB ? K[p] : '.';
}

// V = e^(iα)(cos(θ/2) I − i sin(θ/2) n·σ)
struct Rot { float alpha, theta, nx, ny, nz; };

static Rot rotation(int p, float angle)
{
    const float R2 = 0.70710678f;
    switch (p) {
    case P_H:  return { PI_ / 2, PI_, R2, 0, R2 };
    case P_X:  return { PI_ / 2, PI_, 1, 0, 0 };
    case P_Y:  return { PI_ / 2, PI_, 0, 1, 0 };
    case P_Z:  return { PI_ / 2, PI_, 0, 0, 1 };
    case P_S:  return { PI_ / 4, PI_ / 2, 0, 0, 1 };
    case P_SD: return { -PI_ / 4, -PI_ / 2, 0, 0, 1 };
    case P_T:  return { PI_ / 8, PI_ / 4, 0, 0, 1 };
    case P_TD: return { -PI_ / 8, -PI_ / 4, 0, 0, 1 };
    case P_RX: return { 0, angle, 1, 0, 0 };
    case P_RY: return { 0, angle, 0, 1, 0 };
    case P_RZ: return { 0, angle, 0, 0, 1 };
    default:   return { 0, 0, 0, 0, 1 };
    }
}

// matrice 2x2 de V^u
static void matrice(int p, float angle, float u, cplx m[4])
{
    Rot r = rotation(p, angle);
    float c = cosf(r.theta * u / 2), s = sinf(r.theta * u / 2);
    cplx ph = C(cosf(r.alpha * u), sinf(r.alpha * u));
    m[0] = ph * C(c, -s * r.nz);
    m[1] = ph * C(-s * r.ny, -s * r.nx);
    m[2] = ph * C(s * r.ny, -s * r.nx);
    m[3] = ph * C(c, s * r.nz);
}

static void appliquer_1q(Etat &e, int t, uint32_t cm, const cplx m[4])
{
    int n = 1 << e.nq;
    uint32_t bt = 1u << t;
    for (int i = 0; i < n; i++) {
        if (i & bt) continue;
        if ((i & cm) != cm) continue;
        int j = i | bt;
        cplx a0 = e.a[i], a1 = e.a[j];
        e.a[i] = m[0] * a0 + m[1] * a1;
        e.a[j] = m[2] * a0 + m[3] * a1;
    }
}

// SWAP^u : partie symetrique inchangee, partie antisymetrique multipliee par e^(iπu)
static void appliquer_swap(Etat &e, int qa, int qb, uint32_t cm, float u)
{
    int n = 1 << e.nq;
    uint32_t ba = 1u << qa, bb = 1u << qb;
    cplx ph = C(cosf(PI_ * u), sinf(PI_ * u));
    for (int i = 0; i < n; i++) {
        if (!(i & ba) || (i & bb)) continue;        // i : qa=1, qb=0
        if ((i & cm) != cm) continue;
        int j = (i & ~ba) | bb;                      // j : qa=0, qb=1
        cplx sy = 0.5f * (e.a[i] + e.a[j]), an = 0.5f * (e.a[i] - e.a[j]);
        cplx ar = ph * an;
        e.a[i] = sy + ar;
        e.a[j] = sy - ar;
    }
}

void etat_zero(Etat &e, int nq)
{
    e.nq = nq;
    for (int i = 0; i < NE_MAX; i++) e.a[i] = C(0);
    e.a[0] = C(1);
}

static uint32_t masque_controles(const Circuit &c, int col)
{
    uint32_t m = 0;
    for (int k = 0; k < c.nq; k++) if (c.c[col][k].p == P_CTRL) m |= 1u << k;
    return m;
}

void appliquer_colonne(Etat &e, const Circuit &c, int col, float u)
{
    if (col < 0 || col >= NC_MAX) return;
    uint32_t cm = masque_controles(c, col);
    int sw[2], ns = 0;
    for (int k = 0; k < c.nq; k++) {
        int p = c.c[col][k].p;
        if (p >= P_H && p <= P_RZ) {
            cplx m[4];
            matrice(p, c.c[col][k].angle, u, m);
            appliquer_1q(e, k, cm, m);
        } else if (p == P_SWAP && ns < 2) {
            sw[ns++] = k;
        }
    }
    if (ns == 2) appliquer_swap(e, sw[0], sw[1], cm, u);
}

static float proba_un(const Etat &e, int k)
{
    float p = 0;
    for (int i = 0; i < (1 << e.nq); i++) if (i & (1 << k)) p += norme2(e.a[i]);
    return p;
}

static void effondrer(Etat &e, int k, int v)
{
    float s = 0;
    for (int i = 0; i < (1 << e.nq); i++) {
        if (((i >> k) & 1) != v) e.a[i] = C(0);
        else s += norme2(e.a[i]);
    }
    float f = s > 1e-12f ? 1.0f / sqrtf(s) : 0;
    for (int i = 0; i < (1 << e.nq); i++) e.a[i] = f * e.a[i];
}

static float uniforme(uint32_t (*alea32)()) { return (alea32() >> 8) * (1.0f / 16777216.0f); }

bool mesurer_colonne(Etat &e, const Circuit &c, int col, uint32_t (*alea32)(), int &bits, int &masque)
{
    bits = 0; masque = 0;
    for (int k = 0; k < c.nq; k++) {
        if (c.c[col][k].p != P_MESURE) continue;
        float p1 = proba_un(e, k);
        int v = uniforme(alea32) < p1 ? 1 : 0;
        effondrer(e, k, v);
        masque |= 1 << k;
        bits |= v << k;
    }
    return masque != 0;
}

void executer(const Circuit &c, Etat &e, int jusqua)
{
    etat_zero(e, c.nq);
    int fin = jusqua < 0 ? c.nc : jusqua;
    for (int col = 0; col < fin && col < NC_MAX; col++) appliquer_colonne(e, c, col, 1);
}

void probabilites(const Etat &e, float *p)
{
    for (int i = 0; i < (1 << e.nq); i++) p[i] = norme2(e.a[i]);
}

void bloch(const Etat &e, int k, float &x, float &y, float &z)
{
    float r00 = 0, r11 = 0;
    cplx r01 = C(0);
    for (int i = 0; i < (1 << e.nq); i++) {
        if (i & (1 << k)) { r11 += norme2(e.a[i]); continue; }
        r00 += norme2(e.a[i]);
        r01 = r01 + e.a[i] * conj(e.a[i | (1 << k)]);
    }
    x = 2 * r01.re;
    y = -2 * r01.im;
    z = r00 - r11;
}

float purete(const Etat &e, int k)
{
    float x, y, z;
    bloch(e, k, x, y, z);
    return sqrtf(x * x + y * y + z * z);
}

bool a_mesures_intermediaires(const Circuit &c)
{
    for (int k = 0; k < c.nq; k++) {
        bool mesure = false;
        for (int col = 0; col < c.nc; col++) {
            int p = c.c[col][k].p;
            if (p == P_MESURE) mesure = true;
            else if (mesure && p != P_VIDE) return true;
        }
        // un qubit mesure qui sert ensuite de controle ou de cible a un autre
    }
    // une mesure suivie de portes sur d'autres qubits ne change rien aux statistiques
    return false;
}

int tir(const Circuit &c, uint32_t (*alea32)(), float bruit_porte, float bruit_lecture)
{
    Etat e;
    etat_zero(e, c.nq);
    for (int col = 0; col < c.nc; col++) {
        appliquer_colonne(e, c, col, 1);
        if (bruit_porte > 0) {
            for (int k = 0; k < c.nq; k++) {
                int p = c.c[col][k].p;
                if (p == P_VIDE || p == P_MESURE) continue;
                if (uniforme(alea32) >= bruit_porte) continue;
                cplx m[4];
                matrice(P_X + (int)(uniforme(alea32) * 3) % 3, 0, 1, m);   // erreur de Pauli X, Y ou Z
                appliquer_1q(e, k, 0, m);
            }
        }
        int b, mk;
        mesurer_colonne(e, c, col, alea32, b, mk);
    }
    // mesure finale de tout le registre
    float r = uniforme(alea32), s = 0;
    int n = 1 << c.nq, res = n - 1;
    for (int i = 0; i < n; i++) {
        s += norme2(e.a[i]);
        if (r < s) { res = i; break; }
    }
    if (bruit_lecture > 0)
        for (int k = 0; k < c.nq; k++)
            if (uniforme(alea32) < bruit_lecture) res ^= 1 << k;
    return res;
}

// ---------------------------------------------------------------- edition
void vider(Circuit &c, int nq)
{
    memset(&c, 0, sizeof c);
    c.nq = nq < 1 ? 1 : (nq > NQ_MAX ? NQ_MAX : nq);
    c.nc = 0;
    c.algo = -1;
    strcpy(c.nom, "Mon circuit");
}

int colonnes_utiles(const Circuit &c)
{
    int n = 0;
    for (int col = 0; col < NC_MAX; col++)
        for (int k = 0; k < c.nq; k++)
            if (c.c[col][k].p != P_VIDE) n = col + 1;
    return n;
}

void poser(Circuit &c, int col, int qb, int p, float angle)
{
    if (col < 0 || col >= NC_MAX || qb < 0 || qb >= c.nq) return;
    c.c[col][qb].p = (uint8_t)p;
    c.c[col][qb].angle = angle;
    c.nc = colonnes_utiles(c);
}

static int porte_depuis_code(char ch)
{
    for (int p = 0; p < P_NB; p++) if (code_porte(p) == ch) return p;
    return P_VIDE;
}

void depuis_texte(Circuit &c, int nq, const char *txt, const float *angles)
{
    vider(c, nq);
    int col = 0, k = 0, ia = 0;
    for (const char *s = txt; *s && col < NC_MAX; s++) {
        if (*s == '|') { col++; k = 0; continue; }
        if (k < nq) {
            int p = porte_depuis_code(*s);
            c.c[col][k].p = (uint8_t)p;
            c.c[col][k].angle = porte_angle(p) && angles ? angles[ia++] : 1.5708f;
        }
        k++;
    }
    c.nc = colonnes_utiles(c);
}

// ---------------------------------------------------------------- OpenQASM 2.0
static int ajout(char *buf, int n, int pos, const char *fmt_, ...)
{
    if (pos >= n - 1) return pos;
    va_list ap;
    va_start(ap, fmt_);
    int r = vsnprintf(buf + pos, n - pos, fmt_, ap);
    va_end(ap);
    if (r < 0) return pos;
    return pos + r >= n ? n - 1 : pos + r;
}

bool vers_qasm(const Circuit &c, char *buf, int n)
{
    bool ok = true;
    int pos = 0;
    pos = ajout(buf, n, pos, "OPENQASM 2.0;\ninclude \"qelib1.inc\";\nqreg q[%d];\ncreg c[%d];\n", c.nq, c.nq);
    bool mesure = false;
    for (int col = 0; col < c.nc; col++) {
        int ctl[NQ_MAX], nctl = 0, sw[2], ns = 0;
        for (int k = 0; k < c.nq; k++) {
            int p = c.c[col][k].p;
            if (p == P_CTRL) ctl[nctl++] = k;
            if (p == P_SWAP && ns < 2) sw[ns++] = k;
        }
        for (int k = 0; k < c.nq; k++) {
            int p = c.c[col][k].p;
            float a = c.c[col][k].angle;
            if (p < P_H || p > P_RZ) continue;
            static const char *SIMPLE[] = { "", "h", "x", "y", "z", "s", "sdg", "t", "tdg", "rx", "ry", "rz" };
            if (nctl == 0) {
                if (porte_angle(p)) pos = ajout(buf, n, pos, "%s(%.4f) q[%d];\n", SIMPLE[p], a, k);
                else pos = ajout(buf, n, pos, "%s q[%d];\n", SIMPLE[p], k);
            } else if (nctl == 1) {
                int t0 = ctl[0];
                switch (p) {
                case P_H: pos = ajout(buf, n, pos, "ch q[%d],q[%d];\n", t0, k); break;
                case P_X: pos = ajout(buf, n, pos, "cx q[%d],q[%d];\n", t0, k); break;
                case P_Y: pos = ajout(buf, n, pos, "cy q[%d],q[%d];\n", t0, k); break;
                case P_Z: pos = ajout(buf, n, pos, "cz q[%d],q[%d];\n", t0, k); break;
                case P_S: pos = ajout(buf, n, pos, "cu1(pi/2) q[%d],q[%d];\n", t0, k); break;
                case P_SD: pos = ajout(buf, n, pos, "cu1(-pi/2) q[%d],q[%d];\n", t0, k); break;
                case P_T: pos = ajout(buf, n, pos, "cu1(pi/4) q[%d],q[%d];\n", t0, k); break;
                case P_TD: pos = ajout(buf, n, pos, "cu1(-pi/4) q[%d],q[%d];\n", t0, k); break;
                case P_RX: pos = ajout(buf, n, pos, "cu3(%.4f,-pi/2,pi/2) q[%d],q[%d];\n", a, t0, k); break;
                case P_RY: pos = ajout(buf, n, pos, "cu3(%.4f,0,0) q[%d],q[%d];\n", a, t0, k); break;
                case P_RZ: pos = ajout(buf, n, pos, "crz(%.4f) q[%d],q[%d];\n", a, t0, k); break;
                }
            } else if (nctl == 2 && (p == P_X || p == P_Z)) {
                if (p == P_Z) pos = ajout(buf, n, pos, "h q[%d];\n", k);
                pos = ajout(buf, n, pos, "ccx q[%d],q[%d],q[%d];\n", ctl[0], ctl[1], k);
                if (p == P_Z) pos = ajout(buf, n, pos, "h q[%d];\n", k);
            } else {
                ok = false;
            }
        }
        if (ns == 2) {
            if (nctl == 0) pos = ajout(buf, n, pos, "swap q[%d],q[%d];\n", sw[0], sw[1]);
            else if (nctl == 1) pos = ajout(buf, n, pos, "cswap q[%d],q[%d],q[%d];\n", ctl[0], sw[0], sw[1]);
            else ok = false;
        }
        for (int k = 0; k < c.nq; k++)
            if (c.c[col][k].p == P_MESURE) { pos = ajout(buf, n, pos, "measure q[%d] -> c[%d];\n", k, k); mesure = true; }
    }
    if (!mesure) pos = ajout(buf, n, pos, "measure q -> c;\n");
    return ok && pos < n - 1;
}

// ---------------------------------------------------------------- algorithmes
// format : colonnes separees par '|', un caractere par qubit (q0 en premier)
const Algo ALGOS[] = {
    { "Pile ou face quantique", "Pile ou face", 1, "H|M", { 0 },
      { "H met |0⟩ en superposition : (|0⟩ + |1⟩)/√2",
        "Mesure : 0 ou 1, au hasard, 50 % chacun" },
      "0 : 50 %   1 : 50 %" },
    { "Interférence (Mach-Zehnder)", "Interférence", 1, "H|Z|H|M", { 0 },
      { "H : la « particule » prend les deux chemins à la fois",
        "Z : change le signe de |1⟩ (invisible à la mesure !)",
        "H : les chemins interfèrent : |1⟩ à 100 %",
        "Mesure : toujours 1 (sans Z : toujours 0)" },
      "1 : 100 %" },
    { "État de Bell (intrication)", "Bell", 2, "H.|@X|MM", { 0 },
      { "H sur q0 : superposition",
        "CNOT : q1 copie q0… dans les deux branches à la fois",
        "Mesure : 00 ou 11, jamais 01 ni 10" },
      "00 : 50 %   11 : 50 %" },
    { "GHZ : 3 qubits intriqués", "GHZ", 3, "H..|@X.|.@X|MMM", { 0 },
      { "H sur q0 : superposition",
        "CNOT q0 → q1",
        "CNOT q1 → q2 : (|000⟩ + |111⟩)/√2",
        "Mesure : 000 ou 111, les trois toujours d'accord" },
      "000 : 50 %   111 : 50 %" },
    { "Deutsch-Jozsa", "Deutsch-Jozsa", 3, "..X|HHH|@.X|.@X|HH.|MM.", { 0 },
      { "q2 (l'aide) passe à |1⟩",
        "H partout : on interroge les 4 entrées à la fois",
        "Oracle f(x) = x0 ⊕ x1 : 1ʳᵉ partie",
        "Oracle : 2ᵉ partie (la phase « remonte » sur l'entrée)",
        "H : les réponses interfèrent",
        "Mesure : 00 = constante, autre = équilibrée" },
      "q1 q0 = 11 : 100 % → équilibrée" },
    { "Bernstein-Vazirani (secret 101)", "Bernstein-V.", 4, "...X|HHHH|@..X|..@X|HHH.|MMM.", { 0 },
      { "q3 (l'aide) passe à |1⟩",
        "H partout : superposition de toutes les questions",
        "Oracle s·x : le bit 0 du secret",
        "Oracle : le bit 2 du secret",
        "H : le secret apparaît",
        "Mesure : 101 en UNE seule question (il en faut 3 sans quantique)" },
      "101 : 100 %" },
    { "Grover (2 qubits, cherche 11)", "Grover 2q", 2, "HH|@Z|HH|XX|@Z|XX|HH|MM", { 0 },
      { "H H : les 4 réponses à égalité (25 % chacune)",
        "Oracle : marque |11⟩ d'un signe −",
        "Diffusion : H H",
        "Diffusion : X X",
        "Diffusion : CZ (inversion par rapport à la moyenne)",
        "Diffusion : X X",
        "Diffusion : H H → |11⟩ a grandi jusqu'à 100 %",
        "Mesure : trouvé en UNE seule question" },
      "11 : 100 %" },
    { "Grover (3 qubits, cherche 101)", "Grover 3q", 3,
      "HHH|.X.|@@Z|.X.|HHH|XXX|@@Z|XXX|HHH|.X.|@@Z|.X.|HHH|XXX|@@Z|XXX|HHH|MMM", { 0 },
      { "H : les 8 réponses à égalité (12,5 %)",
        "Oracle : X sur q1 pour viser 101",
        "Oracle : CCZ marque la cible d'un signe −",
        "Oracle : X défait",
        "Diffusion : H", "Diffusion : X", "Diffusion : CCZ", "Diffusion : X",
        "Diffusion : H → 101 passe à 78 %",
        "2ᵉ tour : oracle (X)", "2ᵉ tour : oracle (CCZ)", "2ᵉ tour : oracle (X)",
        "2ᵉ tour : diffusion (H)", "2ᵉ tour : diffusion (X)", "2ᵉ tour : diffusion (CCZ)",
        "2ᵉ tour : diffusion (X)", "2ᵉ tour : diffusion (H) → 101 à 94,5 %",
        "Mesure : 101 presque à coup sûr" },
      "101 : 94,5 %" },
    { "Téléportation quantique", "Téléportation", 3, "y..|z..|.H.|.@X|@X.|H..|.@X|@.Z|..z|..y|..M",
      { 1.1f, 0.8f, -0.8f, -1.1f },
      { "Le message : q0 tourné de Ry(1,1)…",
        "… puis de Rz(0,8) : un état quelconque",
        "Alice et Bob partagent une paire de Bell : H sur q1",
        "CNOT q1 → q2 : q1 chez Alice, q2 chez Bob",
        "Alice : CNOT message → sa moitié",
        "Alice : H sur le message",
        "Bob corrige avec X (selon q1)",
        "Bob corrige avec Z (selon q0) : q2 = le message !",
        "Vérification : on défait Rz(0,8) sur q2…",
        "… et Ry(1,1) : q2 doit revenir à |0⟩",
        "Mesure de q2 : 0 à 100 %" },
      "q2 = 0 : 100 %" },
    { "QFT : trouver une période", "QFT période", 3, ".HH|..H|.@S|@.T|.H.|@S.|H..|x.x|MMM", { 0 },
      { "État périodique : 0, 2, 4, 6 (période 2)",
        "QFT : H sur q2",
        "QFT : S contrôlée", "QFT : T contrôlée", "QFT : H sur q1",
        "QFT : S contrôlée", "QFT : H sur q0", "QFT : on remet les bits dans l'ordre",
        "Pics à 0 et 4 : période = 8 / 4 = 2 (le cœur de Shor)" },
      "000 : 50 %   100 : 50 %" },
    { "Estimation de phase (T)", "Phase", 4, "...X|HHH.|@..T|.@.S|..@Z|x.x.|H...|@s..|.H..|@.t.|.@s.|..H.|MMM.",
      { 0 },
      { "q3 = |1⟩ : un état propre de T",
        "H sur les 3 compteurs",
        "T contrôlée par q0 (1 fois)",
        "T² = S contrôlée par q1 (2 fois)",
        "T⁴ = Z contrôlée par q2 (4 fois)",
        "QFT inverse : ordre des bits",
        "QFT inverse : H", "QFT inverse : S† contrôlée", "QFT inverse : H",
        "QFT inverse : T† contrôlée", "QFT inverse : S† contrôlée", "QFT inverse : H",
        "Mesure : 001 = 1/8 de tour : la phase de T !" },
      "001 : 100 % → phase = 1/8" },
    { "Générateur de hasard (5 qubits)", "Hasard", 5, "HHHHH|MMMMM", { 0 },
      { "H sur chaque qubit : 32 réponses à égalité",
        "Mesure : un nombre de 0 à 31, vraiment au hasard" },
      "32 résultats à ≈ 3,1 %" },
};
const int NB_ALGOS = sizeof ALGOS / sizeof ALGOS[0];

void charger_algo(Circuit &c, int i)
{
    if (i < 0 || i >= NB_ALGOS) return;
    depuis_texte(c, ALGOS[i].nq, ALGOS[i].texte, ALGOS[i].angles);
    c.algo = i;
    strncpy(c.nom, ALGOS[i].court, sizeof c.nom - 1);
    c.nom[sizeof c.nom - 1] = 0;
}

}  // namespace q
