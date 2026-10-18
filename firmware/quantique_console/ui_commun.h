// ui_commun.h - outils partages par les fichiers de l'interface
// (interface.cpp, ecran_circuit.cpp, ecran_bloch.cpp, ecran_mesures.cpp,
//  ecran_algos.cpp, ecran_tuto.cpp, ecran_qpu.cpp, formule.cpp)
#pragma once
#include <stdint.h>
#include <math.h>
#include "gfx.h"
#include "quantique.h"

namespace ui {

using namespace gfx;

// ============================ Palette ======================================
const uint16_t FOND_HAUT = rgb(4, 4, 20);
const uint16_t FOND_BAS  = rgb(18, 6, 42);
const uint16_t PANNEAU   = rgb(18, 18, 54);
const uint16_t PANNEAU2  = rgb(32, 30, 84);
const uint16_t BORD      = rgb(70, 60, 150);
const uint16_t GRILLE    = rgb(40, 40, 96);
const uint16_t TEXTE     = rgb(240, 242, 255);
const uint16_t TEXTE2    = rgb(166, 170, 220);
const uint16_t TEXTE3    = rgb(104, 106, 160);
const uint16_t CYAN      = rgb(40, 232, 255);
const uint16_t BLEU      = rgb(70, 120, 255);
const uint16_t VIOLET    = rgb(150, 90, 255);
const uint16_t MAGENTA   = rgb(255, 60, 210);
const uint16_t ROSE      = rgb(255, 120, 200);
const uint16_t JAUNE     = rgb(255, 214, 64);
const uint16_t ORANGE    = rgb(255, 140, 60);
const uint16_t ROUGE     = rgb(255, 70, 100);
const uint16_t VERT      = rgb(60, 255, 160);
const uint16_t NOIR_BLEU = rgb(6, 6, 30);

const float PI_F = 3.14159265f;

// zone de contenu entre les deux barres
const int ZY0 = 27, ZY1 = 203;

// ============================ Etat partage ==================================
extern float t, dt;                  // temps (s) et duree de l'image

struct Reglages {
    uint32_t magie;
    int32_t tirs_i;      // 0..2 : 100 / 1 000 / 8 192 tirs
    int32_t bruit_i;     // 0 parfait / 1 faible / 2 comme un vrai QPU
    int32_t vitesse_i;   // 0 lent / 1 normal / 2 rapide
    int32_t hasard;      // 0 materiel / 1 graine fixe (reproductible)
    int32_t son;         // 0 muet / 1 clics de mesure
    int32_t volume;      // 0..2
    int32_t lum;         // 5..100
    int32_t vue_mes;     // 0 histogramme / 1 amplitudes
    int32_t page_reg;    // page des reglages
    int32_t qubit_bloch; // qubit affiche sur la sphere
};
extern Reglages R;
void sauver_reglages();
int nb_tirs();
float bruit_porte();
float bruit_lecture();

// toucher : le doigt vient d'etre pose / est pose / glisse
struct Toucher {
    bool appui, debut, fin, tap;
    int x, y, x0, y0;
    int dx, dy;
    float duree;
    bool bouge;
    bool double_tap;
};
extern Toucher tc;

// ---- le circuit et sa lecture pas a pas -------------------------------------
extern q::Circuit circ;
struct Lecture {
    bool active;         // lecture pas a pas en cours (sinon : etat final affiche)
    bool pause;
    int col;             // colonne en cours
    float u;             // avancement dans la colonne (0..1)
    int bits, masque;    // resultats des mesures deja faites
    float flash;         // eclair d'effondrement (1 -> 0)
};
extern Lecture lec;
extern q::Etat etat_aff;             // etat affiche (a la position de lecture)
extern float probas_aff[q::NE_MAX];
void circuit_modifie();               // a appeler apres chaque edition
void lecture_jouer();                 // depuis le debut
void lecture_pas(int sens);           // +1 / -1 colonne
void lecture_stop();                  // revient a l'etat final
void lecture_maj();                   // chaque image
void aller_onglet(int o);
enum { O_CIRCUIT = 0, O_SPHERE, O_MESURES, O_ALGOS, O_TUTO, O_REGLAGES, NB_ONGLETS };

// ---- tirs (histogramme) --------------------------------------------------------
struct Tirs {
    int n, cible;            // tirs faits / demandes
    int compte[q::NE_MAX];   // par resultat (marginalise sur les qubits mesures)
    int masque;              // qubits mesures
    bool en_cours;
    int dernier;             // dernier resultat
    float t_dernier;
};
extern Tirs tirs;
void tirs_lancer(int n);
void tirs_maj();                      // avance de quelques tirs par image
int masque_mesure(const q::Circuit &c);
int compacter(int x, int masque);     // garde les bits mesures, a la suite
uint32_t alea32();                    // hasard (materiel ou graine fixe)
void bip_mesure(int v);               // petit clic (0 grave / 1 aigu)

// ============================ Outils ========================================
inline float sat01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline float approche(float v, float c, float k) { return v + (c - v) * (1.0f - expf(-k * dt)); }
inline float sortie(float x) { x = sat01(x); return 1 - (1 - x) * (1 - x) * (1 - x); }
inline float doux(float x) { x = sat01(x); return x * x * (3 - 2 * x); }
inline float pulse(float f, float ph = 0) { return 0.5f + 0.5f * sinf(t * f * 2 * PI_F + ph); }
inline bool dans(int x, int y, int rx, int ry, int rl, int rh) { return x >= rx && x < rx + rl && y >= ry && y < ry + rh; }
inline bool tap_dans(int rx, int ry, int rl, int rh) { return tc.tap && dans(tc.x0, tc.y0, rx, ry, rl, rh); }
inline bool debut_dans(int rx, int ry, int rl, int rh) { return tc.debut && dans(tc.x, tc.y, rx, ry, rl, rh); }
float alea();

void fmt(char *b, int n, double v, int dec);          // virgule decimale
void fmt_pct(char *b, int n, float p);                 // "94,5 %"
void ket(char *b, int n, int i, int nq);               // "|101⟩" (q0 a droite, comme IBM)
void bits(char *b, int n, int i, int nb);              // "101"
uint16_t couleur_phase(float ph);                      // roue des phases (rouge = 0)
uint16_t couleur_qubit(int k);

void panneau(int x, int y, int l, int h, uint8_t a = 150);
void toast(const char *s, uint16_t c);
void segments(int idx, int x, int y, int l, int h, const char *const *lab, int n, int sel);
int segment_touche(int x, int l, int n);
void bouton_plat(int x, int y, int l, int h, const char *txt, uint16_t c0, uint16_t c1, bool actif = true);
bool bouton(int x, int y, int l, int h, const char *txt, uint16_t c0, uint16_t c1, bool actif = true);  // + tap
bool petit_bouton(int x, int y, int l, int h, const char *txt, uint16_t c, bool actif = true);
void curseur(int x0, int x1, int y, float f, bool actif, uint16_t c0, uint16_t c1);
void eclat(float x, float y, uint16_t c, int n);      // gerbe de particules
void triangle_plein(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c);
int paragraphe(int x, int y, int l, const char *s, const PoliceAA &p, uint16_t c, uint8_t a, int interligne);
void fleches_pages(int page, int nb, float t0);       // points de pagination
int glisse_page();                                     // -1 / +1 sur un glissement horizontal, 0 sinon
void jauge(int x, int y, int n, int max, uint16_t c);  // 5 points de maturite

// sphere de Bloch : centre, rayon, vue (lacet, tangage), vecteur r
void sphere_bloch(float cx, float cy, float r, float lacet, float tangage, float bx, float by, float bz,
                  uint16_t c, bool etiquettes, const float *trace = nullptr, int ntrace = 0);
void proj_bloch(float cx, float cy, float r, float lacet, float tangage, float x, float y, float z,
                float &sx, float &sy, float &prof);

// formules mathematiques (formule.cpp) :
//   x^{2}  x_{k}  \f{num}{den}  \r{racine}  \m{a,b;c,d}  \c3{en couleur}
// taille : 0 petite (13 px), 1 moyenne (16 px), 2 grande (22 px). y = ligne de base.
int formule(float x, float y, const char *s, int taille, uint16_t c, uint8_t a = 255, int align = GAUCHE);
int largeur_formule(const char *s, int taille);
void mesure_formule(const char *s, int taille, float &w, float &asc, float &desc);

// ============================ Ecrans ========================================
void ecran_circuit();
void ecran_sphere();
void ecran_mesures();
void ecran_algos();
void ecran_tuto();
void ecran_reglages();
void ecran_qpu(bool &ouvert);         // QR codes et OpenQASM (sur Mesures)
void algos_aller(int page);
void sphere_mode(int m);             // 0 circuit, 1 bac a sable
void tuto_aller(int page);
extern bool qpu_ouvert;
bool qpu_plein_ecran_actif();

}  // namespace ui
