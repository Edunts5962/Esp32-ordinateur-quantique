// ecran_circuit.cpp - onglet CIRCUIT : l'editeur de circuits quantiques,
// la lecture pas a pas (les portes s'appliquent en douceur, les mesures font
// "s'effondrer" l'etat) et les amplitudes en direct, couleur = phase.
#include "ui_commun.h"
#include <stdio.h>
#include <string.h>

namespace ui {

// palette d'outils (dans l'ordre affiche)
static const int OUTILS[] = { q::P_H, q::P_X, q::P_Y, q::P_Z, q::P_S, q::P_T, q::P_SD, q::P_TD,
                              q::P_RX, q::P_RY, q::P_RZ, q::P_CTRL, q::P_SWAP, q::P_MESURE, -2 };
static const int NB_OUTILS = sizeof OUTILS / sizeof OUTILS[0];
static int outil = -1;                // indice dans OUTILS, -1 : aucun (selection)
static float defil = 0, defil_aff = 0;
static int insp_col = -1, insp_q = -1;   // inspecteur ouvert sur une case
static float insp_t0 = 0;
static int bar_info = -1;
static float bar_info_t = -10;

// geometrie
static const int PY = ZY0 + 2;           // palette
static const int CX0 = 44;               // debut des colonnes
static const int CW = 22;                // largeur d'une colonne
// la hauteur du circuit s'adapte au nombre de qubits : la place restante va aux amplitudes
static const int CY0 = ZY0 + 27;
static float esp() { static const float E[6] = { 22, 24, 22, 20, 18, 16 }; return E[circ.nq < 0 ? 0 : (circ.nq > 5 ? 5 : circ.nq)]; }
#define CY1 ((int)(CY0 + esp() * circ.nq + 4))
static int nb_vis() { return (L - CX0 - 4) / CW; }
static float fil_y(int k) { return CY0 + 2 + esp() * (k + 0.5f); }
static float col_x(int col) { return CX0 + (col - defil_aff) * CW + CW / 2.0f; }

static uint16_t couleur_porte(int p)
{
    switch (p) {
    case q::P_H: return CYAN;
    case q::P_X: case q::P_Y: case q::P_Z: return MAGENTA;
    case q::P_S: case q::P_SD: case q::P_T: case q::P_TD: return JAUNE;
    case q::P_RX: case q::P_RY: case q::P_RZ: return VIOLET;
    case q::P_MESURE: return ORANGE;
    default: return TEXTE;
    }
}

// dessine une porte centree en (x, y) ; s : demi-cote
static void dessiner_porte(int p, float x, float y, float s, float eclat_, bool cnot, float angle)
{
    uint16_t c = couleur_porte(p);
    if (p == q::P_CTRL) {
        halo(x, y, 7, CYAN, (uint8_t)(50 + 100 * eclat_));
        disque(x, y, 3.6f, TEXTE);
        return;
    }
    if (p == q::P_SWAP) {
        trait(x - 4.5f, y - 4.5f, x + 4.5f, y + 4.5f, 2, TEXTE);
        trait(x - 4.5f, y + 4.5f, x + 4.5f, y - 4.5f, 2, TEXTE);
        return;
    }
    if (p == q::P_X && cnot) {        // cible d'un CNOT : ⊕
        halo(x, y, s + 4, MAGENTA, (uint8_t)(40 + 120 * eclat_));
        disque(x, y, s - 0.5f, NOIR_BLEU);
        anneau(x, y, s - 0.5f, 1.6f, MAGENTA);
        trait(x - s + 1.5f, y, x + s - 1.5f, y, 1.6f, MAGENTA);
        trait(x, y - s + 1.5f, x, y + s - 1.5f, 1.6f, MAGENTA);
        return;
    }
    if (eclat_ > 0) halo(x, y, s + 8, c, (uint8_t)(160 * eclat_));
    rect_arrondi((int)(x - s), (int)(y - s), (int)(2 * s), (int)(2 * s), 4, melange(NOIR_BLEU, c, 70));
    contour_arrondi((int)(x - s), (int)(y - s), (int)(2 * s), (int)(2 * s), 4, 1.3f, c);
    if (p == q::P_MESURE) {
        arc(x, y + 3, s * 0.65f, 1.3f, PI_F * 1.1f, PI_F * 1.9f, c);
        float a = -PI_F / 2 + 0.6f * sinf(t * 3);
        trait(x, y + 3, x + s * 0.75f * cosf(a), y + 3 + s * 0.75f * sinf(a), 1.3f, TEXTE);
        return;
    }
    const char *n = q::nom_porte(p);
    const PoliceAA &f = largeur_texte(n, POLICE_13) > 2 * s - 2 ? POLICE_11 : POLICE_13;
    texte((int)x, (int)(y - (f.ascendante + f.descendante) / 2 - 0.5f), n, f, TEXTE, 255, CENTRE);
    (void)angle;
}

static bool colonne_a_controle(int col)
{
    for (int k = 0; k < circ.nq; k++) if (circ.c[col][k].p == q::P_CTRL) return true;
    return false;
}

static void dessiner_palette()
{
    float w = (float)(L - 6) / NB_OUTILS;
    for (int i = 0; i < NB_OUTILS; i++) {
        float x = 3 + i * w + w / 2, y = PY + 11;
        bool sel = i == outil;
        if (sel) {
            halo(x, y, 14, CYAN, (uint8_t)(90 + 60 * pulse(1.2f)));
            rect_arrondi((int)(x - w / 2 + 1), PY, (int)w - 2, 22, 5, PANNEAU2);
            contour_arrondi((int)(x - w / 2 + 1), PY, (int)w - 2, 22, 5, 1.4f, CYAN);
        }
        int p = OUTILS[i];
        if (p == -2) {                       // gomme
            rect_arrondi((int)x - 7, (int)y - 4, 14, 8, 2, ROSE, sel ? 255 : 180);
            rect_arrondi((int)x - 7, (int)y - 4, 5, 8, 2, TEXTE, sel ? 255 : 180);
            continue;
        }
        if (p == q::P_CTRL) {
            disque(x, y - 3, 3, TEXTE);
            trait(x, y - 3, x, y + 4, 1.2f, TEXTE);
            anneau(x, y + 5, 3, 1.2f, MAGENTA);
            continue;
        }
        dessiner_porte(p, x, y, 8, 0, false, 0);
    }
    if (tc.tap && tc.y0 >= PY && tc.y0 < PY + 23) {
        int i = (int)((tc.x0 - 3) / w);
        if (i >= 0 && i < NB_OUTILS) {
            outil = outil == i ? -1 : i;
            insp_col = -1;
            if (outil >= 0) {
                static const char *AIDE[] = {
                    "H : superposition (|0⟩ → |0⟩+|1⟩)", "X : le NON quantique (0 ↔ 1)", "Y : demi-tour autour de y",
                    "Z : change le signe de |1⟩", "S : quart de tour de phase", "T : huitième de tour de phase",
                    "S† : S à l'envers", "T† : T à l'envers", "Rx(θ) : rotation autour de x", "Ry(θ) : rotation autour de y",
                    "Rz(θ) : rotation autour de z", "● : contrôle toute la colonne (CNOT…)", "× × : échange deux qubits",
                    "M : mesure (l'état s'effondre)", "Gomme : touche une porte pour l'effacer" };
                toast(AIDE[outil], couleur_porte(OUTILS[outil]) == TEXTE ? CYAN : couleur_porte(OUTILS[outil]));
            }
            tc.tap = false;
        }
    }
}

static void dessiner_fils()
{
    int nv = nb_vis();
    float xs = CX0 - 2, xe = L - 6;
    for (int k = 0; k < circ.nq; k++) {
        float y = fil_y(k);
        uint16_t c = couleur_qubit(k);
        trait(xs, y, xe, y, 1.2f, TEXTE3);
        char b[8];
        snprintf(b, sizeof b, "q%d", k);
        texte(4, (int)y - 7, b, POLICE_11, c);
        texte(CX0 - 4, (int)y - 7, "|0⟩", POLICE_11, TEXTE2, 255, DROITE);
        (void)nv;
    }
}

static void dessiner_portes()
{
    float s = esp() / 2 - 2.5f;
    if (s > 8.5f) s = 8.5f;
    if (s < 6) s = 6;
    int c0 = (int)floorf(defil_aff), c1 = c0 + nb_vis() + 1;
    if (c1 > q::NC_MAX) c1 = q::NC_MAX;
    for (int col = c0 < 0 ? 0 : c0; col < c1; col++) {
        float x = col_x(col);
        if (x < CX0 - CW / 2 || x > L) continue;
        // eclat de la colonne en cours de lecture
        float ec = 0;
        if (lec.active && col == lec.col) ec = sinf(PI_F * sat01(lec.u));
        // trait vertical entre controles / cibles / swaps
        int kmin = 99, kmax = -1;
        bool ctl = colonne_a_controle(col);
        int nsw = 0;
        for (int k = 0; k < circ.nq; k++) if (circ.c[col][k].p == q::P_SWAP) nsw++;
        for (int k = 0; k < circ.nq; k++) {
            int p = circ.c[col][k].p;
            if (p == q::P_VIDE || p == q::P_MESURE) continue;
            if (!ctl && !(p == q::P_SWAP && nsw >= 2)) continue;
            if (k < kmin) kmin = k;
            if (k > kmax) kmax = k;
        }
        if (kmax > kmin) {
            if (ec > 0) trait(x, fil_y(kmin), x, fil_y(kmax), 5, CYAN, (uint8_t)(90 * ec));
            trait(x, fil_y(kmin), x, fil_y(kmax), 1.6f, TEXTE);
        }
        for (int k = 0; k < circ.nq; k++) {
            int p = circ.c[col][k].p;
            if (p == q::P_VIDE) continue;
            dessiner_porte(p, x, fil_y(k), s, ec, ctl, circ.c[col][k].angle);
        }
        if (insp_col == col) {
            float y = fil_y(insp_q);
            contour_arrondi((int)(x - s - 3), (int)(y - s - 3), (int)(2 * s + 6), (int)(2 * s + 6), 6, 1.4f, JAUNE,
                            (uint8_t)(150 + 100 * pulse(1.5f)));
        }
    }
    // faisceau de lecture
    if (lec.active && lec.col < circ.nc) {
        float x = col_x(lec.col) - CW / 2 + CW * sat01(lec.u);
        float h0 = fil_y(0) - 12, h1 = fil_y(circ.nq - 1) + 12;
        for (int i = 0; i < 5; i++) trait(x - i * 2.0f, h0, x - i * 2.0f, h1, 1.5f, CYAN, (uint8_t)(150 - i * 28));
        halo(x, (h0 + h1) / 2, 26, CYAN, 40);
    } else if (lec.active) {
        float x = col_x(circ.nc - 1) + CW / 2 + 2;
        trait(x, fil_y(0) - 10, x, fil_y(circ.nq - 1) + 10, 1.5f, VERT, 160);
    }
    // eclair d'effondrement
    if (lec.flash > 0 && lec.active) {
        int col = lec.col < circ.nc ? lec.col : circ.nc - 1;
        for (int k = 0; k < circ.nq; k++) {
            if (circ.c[col][k].p != q::P_MESURE) continue;
            float x = col_x(col), y = fil_y(k);
            halo(x, y, 30 * lec.flash + 6, ROUGE, (uint8_t)(200 * lec.flash));
            char b[4];
            snprintf(b, sizeof b, "%d", (lec.bits >> k) & 1);
            texte((int)x + 12, (int)y - 9, b, POLICE_16, JAUNE, (uint8_t)(255 * sat01(lec.flash * 2)));
            if (lec.flash > 0.97f) eclat(x, y, ORANGE, 10);
        }
    }
    // resultats des mesures faites, en bout de fil
    if (lec.active && lec.masque) {
        for (int k = 0; k < circ.nq; k++) {
            if (!(lec.masque & (1 << k))) continue;
            char b[4];
            snprintf(b, sizeof b, "%d", (lec.bits >> k) & 1);
            float y = fil_y(k);
            rect_arrondi(L - 17, (int)y - 8, 14, 16, 4, PANNEAU2);
            texte(L - 10, (int)y - 7, b, POLICE_13, JAUNE, 255, CENTRE);
        }
    }
}

static void edition()
{
    // glisser horizontalement : faire defiler les colonnes
    if (tc.appui && tc.bouge && dans(tc.x0, tc.y0, CX0, CY0 - 6, L - CX0, CY1 - CY0 + 12)) {
        defil -= tc.dx / (float)CW;
        defil_aff = defil;
    }
    float maxd = (float)(circ.nc + 2 - nb_vis());
    if (maxd < 0) maxd = 0;
    if (!tc.appui) {
        if (defil > maxd) defil = approche(defil, maxd, 10);
        if (defil < 0) defil = approche(defil, 0, 10);
    }
    if (lec.active && !tc.appui) {
        float c = lec.col - nb_vis() + 3.0f;
        if (c > defil) defil = c;
        if (lec.col < defil) defil = (float)lec.col;
    }
    defil_aff = approche(defil_aff, defil, 12);

    if (!tc.tap || !dans(tc.x0, tc.y0, CX0, CY0 - 6, L - CX0, CY1 - CY0 + 12)) return;
    int col = (int)floorf((tc.x0 - CX0) / (float)CW + defil_aff);
    float e = esp();
    float y0 = fil_y(0) - e / 2;
    int k = (int)floorf((tc.y0 - y0) / e);
    if (col < 0 || col >= q::NC_MAX || k < 0 || k >= circ.nq) return;
    tc.tap = false;
    int p_existant = circ.c[col][k].p;
    if (outil < 0) {
        if (p_existant != q::P_VIDE) { insp_col = col; insp_q = k; insp_t0 = t; }
        else { insp_col = -1; toast("Choisis une porte en haut, puis touche le fil", CYAN); }
        return;
    }
    int p = OUTILS[outil];
    if (lec.active) lecture_stop();
    if (p == -2 || p == p_existant) {
        if (p_existant != q::P_VIDE && q::porte_angle(p_existant) && p == p_existant) { insp_col = col; insp_q = k; insp_t0 = t; return; }
        q::poser(circ, col, k, q::P_VIDE);
        eclat(col_x(col), fil_y(k), ROSE, 6);
    } else {
        q::poser(circ, col, k, p, q::porte_angle(p) ? PI_F / 2 : 0);
        eclat(col_x(col), fil_y(k), couleur_porte(p) == TEXTE ? CYAN : couleur_porte(p), 8);
        if (p == q::P_SWAP) {
            int n = 0;
            for (int j = 0; j < circ.nq; j++) if (circ.c[col][j].p == q::P_SWAP) n++;
            if (n == 1) toast("Pose une 2ᵉ croix dans la même colonne", JAUNE);
        }
        if (p == q::P_CTRL) toast("● contrôle les autres portes de la colonne", CYAN);
    }
    circ.algo = -1;
    strcpy(circ.nom, "Mon circuit");
    circuit_modifie();
}

// inspecteur d'une porte : angle, suppression
static void inspecteur()
{
    if (insp_col < 0) return;
    q::Cellule &ce = circ.c[insp_col][insp_q];
    if (ce.p == q::P_VIDE) { insp_col = -1; return; }
    int x = 12, y = ZY0 + 22, l = 296, h = 96;
    float e = sortie((t - insp_t0) / 0.25f);
    y += (int)(10 * (1 - e));
    rect_arrondi(x, y, l, h, 12, PANNEAU, (uint8_t)(245 * e));
    contour_arrondi(x, y, l, h, 12, 1.4f, couleur_porte(ce.p) == TEXTE ? CYAN : couleur_porte(ce.p), (uint8_t)(220 * e));
    char b[64];
    snprintf(b, sizeof b, "Porte %s sur q%d, colonne %d", q::nom_porte(ce.p), insp_q, insp_col + 1);
    texte(x + 12, y + 8, b, POLICE_13, TEXTE);
    bool modif = false;
    if (q::porte_angle(ce.p)) {
        // curseur d'angle de -π a π
        int sx0 = x + 14, sx1 = x + l - 14, sy = y + 44;
        char a[24];
        fmt(a, sizeof a, ce.angle / PI_F, 2);
        snprintf(b, sizeof b, "θ = %s π  (%d°)", a, (int)lroundf(ce.angle * 180 / PI_F));
        texte(x + 12, y + 24, b, POLICE_11, TEXTE2);
        bool actif = tc.appui && dans(tc.x0, tc.y0, sx0 - 10, sy - 12, sx1 - sx0 + 20, 24);
        if (actif) {
            float f = sat01((tc.x - sx0) / (float)(sx1 - sx0));
            float v = (f * 2 - 1) * PI_F;
            v = roundf(v / (PI_F / 24)) * (PI_F / 24);        // pas de 7,5°
            if (v != ce.angle) { ce.angle = v; modif = true; }
        }
        curseur(sx0, sx1, sy, (ce.angle / PI_F + 1) / 2, actif, VIOLET, CYAN);
        static const char *PR[4] = { "π/4", "π/2", "π", "−π/2" };
        static const float PV[4] = { PI_F / 4, PI_F / 2, PI_F, -PI_F / 2 };
        for (int i = 0; i < 4; i++)
            if (petit_bouton(x + 12 + i * 44, y + h - 30, 40, 22, PR[i], VIOLET)) { ce.angle = PV[i]; modif = true; }
    } else {
        static const char *INFO[q::P_NB] = { "", "Hadamard : crée la superposition", "NON quantique : |0⟩ ↔ |1⟩",
                                             "Pauli Y : demi-tour autour de y", "Pauli Z : phase −1 sur |1⟩",
                                             "S = √Z : quart de tour de phase", "S† : inverse de S", "T = √S : π/4 de phase",
                                             "T† : inverse de T", "", "", "", "Contrôle : agit si ce qubit vaut 1",
                                             "Échange (SWAP)", "Mesure : 0 ou 1, l'état s'effondre" };
        paragraphe(x + 12, y + 28, l - 24, INFO[ce.p], POLICE_11, TEXTE2, 255, 14);
    }
    if (petit_bouton(x + l - 84, y + h - 30, 74, 22, "Supprimer", ROUGE)) {
        ce.p = q::P_VIDE;
        modif = true;
        insp_col = -1;
    }
    if (petit_bouton(x + l - 26, y + 6, 18, 18, "×", TEXTE3)) insp_col = -1;
    if (modif) { circ.algo = -1; strcpy(circ.nom, "Mon circuit"); circuit_modifie(); }
    if (tc.tap && !dans(tc.x0, tc.y0, x, y, l, h)) { insp_col = -1; tc.tap = false; }
    if (tc.tap) tc.tap = false;
}

// barres des amplitudes : hauteur = probabilite, couleur = phase
static void amplitudes(int x0, int y0, int l, int h)
{
    int n = 1 << circ.nq;
    float w = (float)l / n;
    panneau(x0 - 3, y0 - 3, l + 6, h + 6, 120);
    for (int i = 0; i < n; i++) {
        float p = probas_aff[i];
        q::cplx a = etat_aff.a[i];
        float ph = atan2f(a.im, a.re);
        uint16_t c = couleur_phase(ph);
        float bx = x0 + i * w + (w > 6 ? 1 : 0.3f);
        float bw = w > 6 ? w - 2 : w - 0.6f;
        int hh = (int)(p * (h - (n <= 8 ? 13 : 4)));
        if (p > 0.001f) {
            if (p > 0.2f) halo(bx + bw / 2, (float)(y0 + h - (n <= 8 ? 13 : 4) - hh), bw * 0.8f + 4, c, (uint8_t)(70 * p));
            rect_arrondi_degrade((int)bx, y0 + h - (n <= 8 ? 13 : 4) - hh, (int)(bw + 0.5f), hh + 1, w > 8 ? 2 : 0, c,
                                 melange(c, NOIR_BLEU, 150));
        }
        if (n <= 8) {
            char b[8];
            bits(b, sizeof b, i, circ.nq);
            texte((int)(bx + bw / 2), y0 + h - 12, b, POLICE_11, p > 0.001f ? TEXTE : TEXTE3, 255, CENTRE);
        }
    }
    if (tc.tap && dans(tc.x0, tc.y0, x0, y0, l, h)) {
        bar_info = (int)((tc.x0 - x0) / w);
        if (bar_info >= n) bar_info = n - 1;
        bar_info_t = t;
        tc.tap = false;
    }
    if (bar_info >= 0 && bar_info < n && t - bar_info_t < 3) {
        char kb[16], pb[16], b[80];
        ket(kb, sizeof kb, bar_info, circ.nq);
        fmt_pct(pb, sizeof pb, probas_aff[bar_info]);
        q::cplx a = etat_aff.a[bar_info];
        int deg = (int)lroundf(atan2f(a.im, a.re) * 180 / PI_F);
        snprintf(b, sizeof b, "%s : %s, phase %d°", kb, pb, probas_aff[bar_info] > 1e-4f ? deg : 0);
        int lw = largeur_texte(b, POLICE_11) + 12;
        int bx = (int)(x0 + bar_info * w + w / 2) - lw / 2;
        if (bx < 2) bx = 2;
        if (bx + lw > L - 2) bx = L - 2 - lw;
        rect_arrondi(bx, y0 - 2, lw, 16, 6, PANNEAU2, 240);
        texte(bx + 6, y0, b, POLICE_11, TEXTE);
    }
}

static void commandes()
{
    int y = CY1 + 3;
    // jouer / pause
    bool joue = lec.active && !lec.pause && lec.col < circ.nc;
    if (petit_bouton(4, y, 30, 22, joue ? "❚❚" : "▶", VERT, circ.nc > 0)) {
        if (!lec.active || lec.col >= circ.nc) lecture_jouer();
        else lec.pause = !lec.pause;
        insp_col = -1;
    }
    if (petit_bouton(37, y, 24, 22, "‹", CYAN, circ.nc > 0)) lecture_pas(-1);
    if (petit_bouton(64, y, 24, 22, "›", CYAN, circ.nc > 0)) lecture_pas(1);
    if (petit_bouton(91, y, 24, 22, "■", ROSE, lec.active)) lecture_stop();
    // qubits
    char b[8];
    snprintf(b, sizeof b, "%dq", circ.nq);
    if (petit_bouton(121, y, 20, 22, "−", TEXTE2, circ.nq > 1)) {
        circ.nq--;
        for (int col = 0; col < q::NC_MAX; col++) circ.c[col][circ.nq].p = q::P_VIDE;
        circ.algo = -1; strcpy(circ.nom, "Mon circuit");
        circuit_modifie();
    }
    texte(152, y + 4, b, POLICE_13, TEXTE, 255, CENTRE);
    if (petit_bouton(163, y, 20, 22, "+", TEXTE2, circ.nq < q::NQ_MAX)) {
        circ.nq++;
        circ.algo = -1; strcpy(circ.nom, "Mon circuit");
        circuit_modifie();
    }
    static float t_vider = -10;
    bool confirme = t - t_vider < 2.5f;
    if (petit_bouton(188, y, 54, 22, confirme ? "Sûr ?" : "Vider", confirme ? ROUGE : TEXTE3, circ.nc > 0)) {
        if (confirme) {
            int n = circ.nq;
            q::vider(circ, n);
            circuit_modifie();
            t_vider = -10;
            toast("Circuit vidé", ROSE);
        } else {
            t_vider = t;
        }
    }
    if (petit_bouton(246, y, 70, 22, "Mesurer", ORANGE, circ.nc > 0)) {
        aller_onglet(O_MESURES);
        tirs_lancer(nb_tirs());
    }
}

static void legende()
{
    int y = CY1 + 26;
    const char *s = nullptr;
    char b[120];
    if (lec.active && circ.algo >= 0 && lec.col < circ.nc) s = q::ALGOS[circ.algo].legendes[lec.col];
    else if (lec.active && lec.col >= circ.nc) {
        if (lec.masque) {
            char r[8];
            int nb = 0;
            for (int k = 0; k < circ.nq; k++) if (lec.masque & (1 << k)) nb++;
            bits(r, sizeof r, compacter(lec.bits, lec.masque), nb);
            snprintf(b, sizeof b, "Résultat de ce passage : %s · rejoue : le hasard change !", r);
        } else {
            snprintf(b, sizeof b, "Fin du circuit : voici l'état final");
        }
        s = b;
    } else if (circ.algo >= 0) {
        snprintf(b, sizeof b, "Attendu : %s · ▶ pas à pas", q::ALGOS[circ.algo].attendu);
        s = b;
    } else if (circ.nc == 0) s = "Choisis une porte en haut, puis touche un fil";
    if (!s) return;
    texte(6, y, s, POLICE_11, lec.active ? JAUNE : TEXTE2);
}

void ecran_circuit()
{
    dessiner_palette();
    decoupe(0, CY0 - 2, L, CY1 + 2);
    dessiner_fils();
    edition();
    dessiner_portes();
    decoupe(0, 25, L, 204);
    commandes();
    legende();
    amplitudes(6, CY1 + 41, L - 12, ZY1 - (CY1 + 41) - 3);
    inspecteur();
}

}  // namespace ui
