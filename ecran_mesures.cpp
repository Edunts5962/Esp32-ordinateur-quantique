// ecran_mesures.cpp - onglet MESURES : on fait tourner le circuit des
// centaines de fois. Chaque tir donne un resultat au hasard ; l'histogramme
// tend vers les probabilites |amplitude|² (en contour blanc).
//   Histogramme : tirs, ecart a la theorie, un tir "machine a sous"
//   Amplitudes  : chaque etat de base est une roue (longueur = |a|, angle = phase)
//   Vrai QPU    : exporter le circuit vers un vrai ordinateur quantique (ecran_qpu.cpp)
#include "ui_commun.h"
#include <stdio.h>
#include <string.h>

namespace ui {

bool qpu_ouvert = false;
static float theo[q::NE_MAX];          // probabilites theoriques (qubits mesures)
static int nb_mes = 0;
static float barre_aff[q::NE_MAX];
static int info_i = -1;
static float info_t = -10;
// un tir unique, facon machine a sous
static float un_t0 = -10;
static int un_res = 0;
static bool un_fait = true;

static void calculer_theorie()
{
    q::Etat e;
    q::executer(circ, e);
    float p[q::NE_MAX];
    q::probabilites(e, p);
    int m = masque_mesure(circ);
    nb_mes = 0;
    for (int k = 0; k < circ.nq; k++) if (m & (1 << k)) nb_mes++;
    memset(theo, 0, sizeof theo);
    for (int i = 0; i < (1 << circ.nq); i++) theo[compacter(i, m)] += p[i];
}

static void histogramme()
{
    int n = 1 << nb_mes;
    const int X0 = 8, Y0 = ZY0 + 44, LW = L - 16, HH = 92;
    panneau(X0 - 4, Y0 - 16, LW + 8, HH + 34, 140);
    // grille
    for (int g = 1; g <= 4; g++) {
        float y = Y0 + HH - HH * g / 4.0f;
        trait(X0, y, X0 + LW, y, 1, GRILLE, 120);
        char b[8];
        snprintf(b, sizeof b, "%d%%", g * 25);
        texte(X0 + LW, (int)y - 6, b, POLICE_11, TEXTE3, 180, DROITE);
    }
    float w = (float)LW / n;
    int total = tirs.n > 0 ? tirs.n : 1;
    float ecart = 0;
    for (int i = 0; i < n; i++) {
        float f = tirs.n > 0 ? (float)tirs.compte[i] / total : 0;
        ecart += fabsf(f - theo[i]);
        barre_aff[i] = approche(barre_aff[i], f, 14);
        float bx = X0 + i * w + (w > 8 ? 2 : 0.5f), bw = w > 8 ? w - 4 : w - 1;
        int hh = (int)(barre_aff[i] * HH);
        uint16_t c0 = i == tirs.dernier && tirs.en_cours ? JAUNE : CYAN;
        uint16_t c1 = i == tirs.dernier && tirs.en_cours ? ORANGE : VIOLET;
        if (hh > 0) {
            halo(bx + bw / 2, (float)(Y0 + HH - hh), bw * 0.7f + 6, c0, (uint8_t)(40 + 60 * barre_aff[i]));
            rect_arrondi_degrade((int)bx, Y0 + HH - hh, (int)(bw + 0.5f), hh, w > 10 ? 3 : 1, c0, c1);
        }
        // la theorie : un trait blanc
        if (theo[i] > 0.0005f) {
            float yt = Y0 + HH - theo[i] * HH;
            trait(bx - 1, yt, bx + bw + 1, yt, 1.6f, TEXTE, 230);
        }
        if (n <= 16) {
            char b[8];
            bits(b, sizeof b, i, nb_mes);
            const PoliceAA &f_ = n <= 8 ? POLICE_11 : POLICE_11;
            if (largeur_texte(b, f_) < w || n <= 8)
                texte((int)(bx + bw / 2), Y0 + HH + 3, b, f_, theo[i] > 0.001f ? TEXTE : TEXTE3, 255, CENTRE);
        }
    }
    // legende
    char b[96], e[16];
    if (tirs.n > 0) {
        fmt(e, sizeof e, 50 * ecart, 1);
        snprintf(b, sizeof b, "%d tir%s · écart à la théorie %s %%", tirs.n, tirs.n > 1 ? "s" : "", e);
    } else {
        snprintf(b, sizeof b, "Trait blanc = probabilité théorique |amplitude|²");
    }
    texte(X0, Y0 - 13, b, POLICE_11, TEXTE2);
    // info au toucher d'une barre
    if (tc.tap && dans(tc.x0, tc.y0, X0, Y0, LW, HH + 14)) {
        info_i = (int)((tc.x0 - X0) / w);
        if (info_i >= n) info_i = n - 1;
        info_t = t;
        tc.tap = false;
    }
    if (info_i >= 0 && info_i < n && t - info_t < 3) {
        char kb[8], pf[16], pt[16];
        bits(kb, sizeof kb, info_i, nb_mes);
        fmt_pct(pf, sizeof pf, tirs.n ? (float)tirs.compte[info_i] / total : 0);
        fmt_pct(pt, sizeof pt, theo[info_i]);
        snprintf(b, sizeof b, "%s : mesuré %s, théorie %s", kb, pf, pt);
        int lw = largeur_texte(b, POLICE_11) + 12;
        int bx = (int)(X0 + info_i * w + w / 2) - lw / 2;
        if (bx < 2) bx = 2;
        if (bx + lw > L - 2) bx = L - 2 - lw;
        rect_arrondi(bx, Y0 + 2, lw, 16, 6, PANNEAU2, 240);
        texte(bx + 6, Y0 + 4, b, POLICE_11, TEXTE);
    }
}

static void tir_unique()
{
    float u = t - un_t0;
    if (u > 4) return;
    // les chiffres defilent puis se figent, un par un
    int cx = 160, cy = ZY0 + 80;
    float e = sortie(u / 0.25f) * (u > 3.5f ? 1 - (u - 3.5f) / 0.5f : 1);
    uint8_t a = (uint8_t)(255 * sat01(e));
    int lw = nb_mes * 30 + 40;
    if (lw < 210) lw = 210;
    rect_arrondi(cx - lw / 2, cy - 36, lw, 72, 14, PANNEAU, (uint8_t)(a * 0.96f));
    contour_arrondi(cx - lw / 2, cy - 36, lw, 72, 14, 1.6f, JAUNE, a);
    halo(cx, cy, lw * 0.6f, JAUNE, (uint8_t)(40 * e));
    for (int j = 0; j < nb_mes; j++) {
        int bit = nb_mes - 1 - j;                    // q0 a droite, comme IBM
        float fige = 0.5f + 0.35f * j;
        int x = cx - nb_mes * 15 + j * 30 + 15;
        char d[4];
        if (u < fige) {
            snprintf(d, sizeof d, "%d", (int)(t * 23 + j * 7) & 1);
            texte(x, cy - 22, d, POLICE_40, TEXTE3, a, CENTRE);
        } else {
            snprintf(d, sizeof d, "%d", (un_res >> bit) & 1);
            float f = sat01((u - fige) / 0.3f);
            halo((float)x, (float)cy, 22 * (1 - f) + 6, ROUGE, (uint8_t)(a * (1 - f)));
            texte(x, cy - 22, d, POLICE_40, (un_res >> bit) & 1 ? MAGENTA : CYAN, a, CENTRE);
            if (!un_fait && j == nb_mes - 1 && f > 0) {
                un_fait = true;
                tirs.compte[un_res]++;
                tirs.n++;
                tirs.dernier = un_res;
                bip_mesure(un_res & 1);
                eclat((float)cx, (float)cy, JAUNE, 18);
            }
        }
    }
    texte(cx, cy + 22, "un seul tir : le hasard décide", POLICE_11, TEXTE2, a, CENTRE);
}

static void amplitudes_roues()
{
    // chaque etat de base : une roue. Longueur = |a|, angle = phase, couleur = phase.
    int n = 1 << etat_aff.nq;
    int cols = n <= 4 ? n : (n <= 8 ? 4 : 8);
    int lignes = (n + cols - 1) / cols;
    float cw = (float)(L - 16) / cols, ch = (float)(ZY1 - ZY0 - 66) / lignes;
    float r = (cw < ch ? cw : ch) * 0.36f;
    if (r > 30) r = 30;
    for (int i = 0; i < n; i++) {
        float cx = 8 + (i % cols) * cw + cw / 2, cy = ZY0 + 34 + (i / cols) * ch + ch / 2 - 4;
        q::cplx a = etat_aff.a[i];
        float m = sqrtf(q::norme2(a)), ph = atan2f(a.im, a.re);
        uint16_t c = couleur_phase(ph);
        anneau(cx, cy, r, 1, BORD, 160);
        if (m > 0.01f) {
            halo(cx, cy, r * m + 6, c, (uint8_t)(50 + 100 * m));
            disque(cx, cy, r * m, c, (uint8_t)(70 + 120 * m));
            float ex = cx + r * m * cosf(ph), ey = cy - r * m * sinf(ph);
            trait(cx, cy, ex, ey, 2, TEXTE);
            disque(ex, ey, 2.2f, TEXTE);
        }
        char b[12];
        if (n <= 8) ket(b, sizeof b, i, etat_aff.nq);
        else bits(b, sizeof b, i, etat_aff.nq);
        texte((int)cx, (int)(cy + r + 2), b, POLICE_11, m > 0.01f ? TEXTE : TEXTE3, 255, CENTRE);
    }
    // la roue des phases
    float lx = 20, ly = ZY1 - 20;
    for (int j = 0; j < 24; j++) {
        float a0 = j * 2 * PI_F / 24;
        arc(lx, ly, 9, 4, -a0 - 2 * PI_F / 24, -a0, couleur_phase(a0 + PI_F / 24));
    }
    texte(36, ZY1 - 26, "couleur = phase · taille = |amplitude|", POLICE_11, TEXTE2);
    texte(36, ZY1 - 14, lec.active ? "suit la lecture pas à pas" : "état final du circuit", POLICE_11, TEXTE3);
}

void ecran_mesures()
{
    static const char *VUES[3] = { "Histogramme", "Amplitudes", "Vrai QPU" };
    if (R.vue_mes < 0 || R.vue_mes > 2) R.vue_mes = 0;
    segments(9, 20, ZY0 + 3, 280, 20, VUES, 3, R.vue_mes);
    if (tap_dans(20, ZY0 + 3, 280, 20)) {
        R.vue_mes = segment_touche(20, 280, 3);
        sauver_reglages();
        tc.tap = false;
    }
    calculer_theorie();
    if (R.vue_mes == 2) { qpu_ouvert = true; ecran_qpu(qpu_ouvert); if (!qpu_ouvert) R.vue_mes = 0; return; }
    if (R.vue_mes == 1) { amplitudes_roues(); return; }

    histogramme();
    int y = ZY1 - 25;
    char b[32];
    snprintf(b, sizeof b, "Lancer %d tirs", nb_tirs());
    if (bouton(6, y, 124, 22, b, CYAN, VIOLET, !tirs.en_cours)) { tirs_lancer(nb_tirs()); info_i = -1; }
    if (petit_bouton(136, y, 56, 22, "1 tir", JAUNE, !tirs.en_cours && t - un_t0 > 1.6f)) {
        if (tirs.n == 0) { tirs.masque = masque_mesure(circ); memset(tirs.compte, 0, sizeof tirs.compte); }
        int r = q::tir(circ, alea32, bruit_porte(), bruit_lecture());
        un_res = compacter(r, masque_mesure(circ));
        un_t0 = t;
        un_fait = false;
    }
    static const char *BR[3] = { "Parfait", "Bruit faible", "Bruit QPU" };
    if (petit_bouton(198, y, 116, 22, BR[R.bruit_i], R.bruit_i == 0 ? VERT : (R.bruit_i == 1 ? JAUNE : ORANGE))) {
        R.bruit_i = (R.bruit_i + 1) % 3;
        sauver_reglages();
        toast(R.bruit_i == 0 ? "Qubits parfaits" : (R.bruit_i == 1 ? "Un peu de bruit (0,4 % par porte)"
                                                                   : "Bruit d'un vrai QPU (≈ 2 % par porte)"),
              R.bruit_i == 0 ? VERT : ORANGE);
        tirs.n = 0;
        memset(tirs.compte, 0, sizeof tirs.compte);
    }
    tir_unique();
}

}  // namespace ui
