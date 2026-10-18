// ecran_tuto.cpp - onglet TUTO : les differents types d'equations de
// l'informatique quantique, une par page, chacune avec son animation.
//   lineaire, quadratique (normalisation), parametrique (Bloch), Euler,
//   matricielle, unitarite, exponentielle de matrice, produit tensoriel,
//   equation sans solution (intrication), probabiliste (Born),
//   differentielle (Schrodinger), aux valeurs propres, trigonometrique
//   (Grover), transformee de Fourier (QFT).
#include "ui_commun.h"
#include <stdio.h>
#include <string.h>

namespace ui {

static int page = 0;
static float page_t0 = 0;
static float glisse_x = 0;

typedef void (*FnAnim)(float x, float y, float w, float h, float u);

struct Page {
    const char *titre;
    const char *type;          // le type d'equation (badge)
    uint16_t couleur;
    const char *formule;       // syntaxe de formule.cpp
    int taille;                // 0, 1, 2
    const char *formule2;      // seconde ligne (ou nullptr)
    const char *texte;
    FnAnim anim;
    int essayer;               // >= 0 : algorithme a charger ; -2 : bac a sable ; -1 : rien
};

// ============================================================ animations
static void fleche(float x0, float y0, float x1, float y1, float ep, uint16_t c, uint8_t a = 255)
{
    trait(x0, y0, x1, y1, ep, c, a);
    float an = atan2f(y1 - y0, x1 - x0);
    triangle(x1, y1, x1 - 7 * cosf(an - 0.4f), y1 - 7 * sinf(an - 0.4f), x1 - 7 * cosf(an + 0.4f), y1 - 7 * sinf(an + 0.4f), c, a);
}

static void a_intro(float x, float y, float w, float h, float u)
{
    static const char *SYM[10] = { "ψ", "|0⟩", "|1⟩", "⊗", "α", "β", "e^iφ", "ħ", "⟨x|", "√2" };
    for (int i = 0; i < 10; i++) {
        float a = i * 0.628f + u * 0.5f;
        float r = 26 + 14 * sinf(u * 0.9f + i);
        float sx = x + w / 2 + r * 1.6f * cosf(a), sy = y + h / 2 + r * sinf(a);
        uint16_t c = couleur_qubit(i);
        halo(sx, sy, 10, c, 60);
        texte((int)sx, (int)sy - 7, SYM[i], POLICE_13, c, 230, CENTRE);
    }
    halo(x + w / 2, y + h / 2, 20 + 4 * sinf(u * 3), VIOLET, 120);
    texte((int)(x + w / 2), (int)(y + h / 2) - 11, "ψ", POLICE_22, TEXTE, 255, CENTRE);
}

static void a_lineaire(float x, float y, float w, float h, float u)
{
    float ox = x + 20, oy = y + h - 12, R = h - 22;
    float th = 0.75f + 0.65f * sinf(u * 0.8f);
    fleche(ox, oy, ox, oy - R, 1.6f, CYAN);
    texte((int)ox - 4, (int)(oy - R) - 14, "|0⟩", POLICE_11, CYAN, 255, DROITE);
    fleche(ox, oy, ox + R, oy, 1.6f, MAGENTA);
    texte((int)(ox + R) + 2, (int)oy - 6, "|1⟩", POLICE_11, MAGENTA);
    arc(ox, oy, R, 1, -PI_F / 2, 0, GRILLE);
    float px = ox + R * sinf(th), py = oy - R * cosf(th);
    trait_pointille(px, py, ox, py, 1, 3, 3, CYAN, 180);
    trait_pointille(px, py, px, oy, 1, 3, 3, MAGENTA, 180);
    halo(px, py, 12, JAUNE, 120);
    fleche(ox, oy, px, py, 2.4f, JAUNE);
    texte((int)px + 4, (int)py - 16, "ψ", POLICE_16, JAUNE);
    char b[24], v[12];
    fmt(v, sizeof v, cosf(th), 2);
    snprintf(b, sizeof b, "α = %s", v);
    texte((int)ox + 4, (int)py - 13, b, POLICE_11, CYAN);
    fmt(v, sizeof v, sinf(th), 2);
    snprintf(b, sizeof b, "β = %s", v);
    texte((int)px - 10, (int)oy + 1, b, POLICE_11, MAGENTA);
}

static void a_born(float x, float y, float w, float h, float u)
{
    float th = 0.78f + 0.7f * sinf(u * 0.9f);
    float p0 = cosf(th) * cosf(th);
    // barre empilee : P(0) + P(1) = 100 %
    float bx = x + 10, by = y + 14, bw = w - 20, bh = 20;
    rect_arrondi((int)bx, (int)by, (int)bw, (int)bh, 8, PANNEAU2);
    rect_arrondi_degrade((int)bx, (int)by, (int)(bw * p0), (int)bh, 8, CYAN, BLEU);
    rect_arrondi_degrade((int)(bx + bw * p0), (int)by, (int)(bw * (1 - p0)) + 1, (int)bh, 8, VIOLET, MAGENTA);
    char b[16];
    fmt_pct(b, sizeof b, p0);
    texte((int)bx + 6, (int)by + 3, b, POLICE_11, TEXTE);
    fmt_pct(b, sizeof b, 1 - p0);
    texte((int)(bx + bw) - 6, (int)by + 3, b, POLICE_11, TEXTE, 255, DROITE);
    texte((int)(x + w / 2), (int)by + 24, "100 % au total", POLICE_11, VERT, 255, CENTRE);
    // pluie de mesures
    for (int i = 0; i < 14; i++) {
        float ph = fmodf(u * 0.9f + i * 0.37f, 1.0f);
        float px = x + 12 + i * (w - 24) / 13, py = y + 50 + ph * (h - 54);
        bool un = sinf(i * 12.9898f + floorf(u * 0.9f + i * 0.37f) * 78.233f) * 0.5f + 0.5f > p0;
        texte((int)px, (int)py, un ? "1" : "0", POLICE_13, un ? MAGENTA : CYAN, (uint8_t)(255 * (1 - ph)), CENTRE);
    }
}

static void a_bloch(float x, float y, float w, float h, float u)
{
    float th = 1.0f + 0.8f * sinf(u * 0.6f), ph = u * 1.3f;
    sphere_bloch(x + w / 2, y + h / 2 + 2, h * 0.38f, 0.5f + u * 0.2f, 0.3f, sinf(th) * cosf(ph), sinf(th) * sinf(ph),
                 cosf(th), JAUNE, true);
}

static void a_euler(float x, float y, float w, float h, float u)
{
    float cx = x + 36, cy = y + h / 2, r = 26;
    anneau(cx, cy, r, 1.2f, BORD);
    trait(cx - r - 4, cy, cx + r + 4, cy, 1, GRILLE);
    trait(cx, cy - r - 4, cx, cy + r + 4, 1, GRILLE);
    float a = u * 1.8f;
    float px = cx + r * cosf(a), py = cy - r * sinf(a);
    trait_pointille(px, py, px, cy, 1, 2, 2, CYAN, 200);
    trait_pointille(px, py, cx, py, 1, 2, 2, MAGENTA, 200);
    halo(px, py, 10, couleur_phase(a), 160);
    fleche(cx, cy, px, py, 2.2f, couleur_phase(a));
    // les deux ondes : cos (cyan) et sin (magenta) deroulees a droite
    float x0 = cx + r + 14, x1 = x + w - 4;
    float pxa = 0, pya = 0, pxb = 0, pyb = 0;
    for (int i = 0; i <= 40; i++) {
        float v = a - i * 0.12f;
        float xx = x0 + (x1 - x0) * i / 40;
        float ya = cy - r * 0.8f * cosf(v) * 0.5f - 14, yb = cy - r * 0.8f * sinf(v) * 0.5f + 16;
        if (i) { trait(pxa, pya, xx, ya, 1.6f, CYAN); trait(pxb, pyb, xx, yb, 1.6f, MAGENTA); }
        pxa = xx; pya = ya; pxb = xx; pyb = yb;
    }
    texte((int)x1, (int)cy - 34, "cos φ", POLICE_11, CYAN, 255, DROITE);
    texte((int)x1, (int)cy + 26, "sin φ", POLICE_11, MAGENTA, 255, DROITE);
}

static void a_matrice(float x, float y, float w, float h, float u)
{
    // H appliquee a |0⟩ : la fleche bascule, colonne par colonne
    float k = doux(fmodf(u * 0.45f, 2.0f) < 1 ? fmodf(u * 0.45f, 1.0f) * 1.4f : 1 - (fmodf(u * 0.45f, 1.0f)) * 1.4f);
    float a0 = 1 - k + k * 0.7071f, a1 = k * 0.7071f;
    float cx = x + 14, cy = y + h - 10, R = h - 24;
    arc(cx, cy, R, 1, -PI_F / 2, 0, GRILLE);
    fleche(cx, cy, cx, cy - R, 1.2f, TEXTE3);
    fleche(cx, cy, cx + R, cy, 1.2f, TEXTE3);
    float n = sqrtf(a0 * a0 + a1 * a1);
    float px = cx + R * a1 / n, py = cy - R * a0 / n;
    halo(px, py, 12, CYAN, 140);
    fleche(cx, cy, px, py, 2.4f, CYAN);
    char b[48], v0[12], v1[12];
    fmt(v0, sizeof v0, a0 / n, 2);
    fmt(v1, sizeof v1, a1 / n, 2);
    for (char *c_ = v0; *c_; c_++) if (*c_ == ',') *c_ = '.';
    for (char *c_ = v1; *c_; c_++) if (*c_ == ',') *c_ = '.';
    snprintf(b, sizeof b, "\\m{%s;%s}", v0, v1);
    formule(x + w - 4, y + h - 14, b, 0, CYAN, 255, DROITE);
    texte((int)(x + w - 4), (int)(y + 2), k > 0.5f ? "H|0⟩" : "|0⟩", POLICE_13, JAUNE, 255, DROITE);
}

static void a_unitaire(float x, float y, float w, float h, float u)
{
    // H puis H : on revient au depart (le calcul est reversible)
    float ph = fmodf(u * 0.5f, 2.0f);
    float cx = x + w / 2, cy = y + h / 2 + 4, r = h * 0.34f;
    float th = ph < 1 ? doux(ph) * PI_F / 2 : (1 - doux(ph - 1)) * PI_F / 2;
    sphere_bloch(cx, cy, r, 0.6f, 0.3f, sinf(th), 0, cosf(th), ph < 1 ? CYAN : MAGENTA, false);
    texte((int)cx, (int)y + 2, ph < 1 ? "U : aller" : "U† : retour", POLICE_13, ph < 1 ? CYAN : MAGENTA, 255, CENTRE);
}

static void a_rotation(float x, float y, float w, float h, float u)
{
    float a = u * 1.4f;
    float cx = x + w / 2, cy = y + h / 2 + 4, r = h * 0.36f;
    // rotation autour de x : (0, -sin a, cos a)
    sphere_bloch(cx, cy, r, 0.55f, 0.35f, 0, -sinf(a), cosf(a), VIOLET, false);
    float sx, sy, pr, ex, ey, ep;
    proj_bloch(cx, cy, r * 1.3f, 0.55f, 0.35f, 1, 0, 0, sx, sy, pr);
    proj_bloch(cx, cy, r * 1.3f, 0.55f, 0.35f, -1, 0, 0, ex, ey, ep);
    trait(ex, ey, sx, sy, 4, ROSE, 70);
    trait(ex, ey, sx, sy, 1.4f, ROSE);
    texte((int)sx + 3, (int)sy - 6, "x", POLICE_11, ROSE);
    char b[24], v[12];
    fmt(v, sizeof v, fmodf(a * 180 / PI_F, 360), 0);
    snprintf(b, sizeof b, "θ = %s°", v);
    texte((int)x + 2, (int)y + 2, b, POLICE_11, TEXTE2);
}

static void a_tensoriel(float x, float y, float w, float h, float u)
{
    // le nombre d'amplitudes double a chaque qubit
    int n = 1 + ((int)(u * 1.2f)) % 10;
    float bw = (w - 8) / 10.0f;
    for (int i = 1; i <= 10; i++) {
        float v = i / 10.0f;
        float hh = (h - 30) * v * v;
        uint16_t c = i <= n ? couleur_phase(i * 0.6f) : PANNEAU2;
        rect_arrondi_degrade((int)(x + 4 + (i - 1) * bw), (int)(y + h - 14 - hh), (int)bw - 2, (int)hh + 1, 2, c,
                             melange(c, NOIR_BLEU, 140));
    }
    char b[48];
    snprintf(b, sizeof b, "%d qubit%s : %d", n, n > 1 ? "s" : "", 1 << n);
    texte((int)(x + w / 2), (int)y + 2, b, POLICE_11, JAUNE, 255, CENTRE);
    texte((int)(x + w / 2), (int)(y + h - 12), "50 qubits : 10¹⁵", POLICE_11, TEXTE2, 255, CENTRE);
}

static void a_intrication(float x, float y, float w, float h, float u)
{
    // deux particules eloignees : mesurees, elles donnent toujours le meme resultat
    float cyc = fmodf(u * 0.55f, 1.0f);
    int tirage = (int)(u * 0.55f) * 7919 % 13 > 6 ? 1 : 0;
    float xa = x + 22, xb = x + w - 22, cy = y + h / 2;
    // le lien
    for (int i = 0; i < 18; i++) {
        float f = i / 17.0f;
        float xx = xa + (xb - xa) * f, yy = cy + 7 * sinf(f * 12 - u * 6);
        disque(xx, yy, 1.4f, VIOLET, (uint8_t)(cyc < 0.5f ? 200 : 200 * (1 - (cyc - 0.5f) * 2)));
    }
    for (int k = 0; k < 2; k++) {
        float px = k ? xb : xa;
        if (cyc < 0.5f) {
            float s = pulse(2.5f, k * 1.0f);
            halo(px, cy, 16, melange(CYAN, MAGENTA, (uint8_t)(255 * s)), 160);
            texte((int)px, (int)cy - 8, "?", POLICE_16, TEXTE, 255, CENTRE);
        } else {
            uint16_t c = tirage ? MAGENTA : CYAN;
            halo(px, cy, 16 + 10 * (1 - (cyc - 0.5f) * 2), c, 200);
            texte((int)px, (int)cy - 8, tirage ? "1" : "0", POLICE_16, TEXTE, 255, CENTRE);
        }
    }
    texte((int)(x + w / 2), (int)(y + h - 14), cyc < 0.5f ? "pas de valeur…" : "identiques !",
          POLICE_11, cyc < 0.5f ? TEXTE2 : VERT, 255, CENTRE);
}

static void a_mesure(float x, float y, float w, float h, float u)
{
    float cyc = fmodf(u * 0.4f, 1.0f);
    float cx = x + w / 2, cy = y + h / 2 - 4;
    if (cyc < 0.55f) {
        // le nuage des possibles
        for (int i = 0; i < 26; i++) {
            float a = i * 2.4f + u * (0.6f + (i % 5) * 0.1f), r = 8 + (i * 7) % 26;
            halo(cx + r * 1.5f * cosf(a), cy + r * 0.8f * sinf(a), 5, couleur_qubit(i), 120);
        }
        texte((int)cx, (int)(y + h - 14), "tout à la fois", POLICE_11, TEXTE2, 255, CENTRE);
    } else {
        float k = (cyc - 0.55f) / 0.45f;
        halo(cx, cy, 26 * (1 - k) + 8, ROUGE, (uint8_t)(220 * (1 - k)));
        disque(cx, cy, 5, JAUNE);
        texte((int)cx + 10, (int)cy - 8, "|x⟩", POLICE_13, JAUNE);
        texte((int)cx, (int)(y + h - 14), "il en reste une", POLICE_11, JAUNE, 255, CENTRE);
    }
}

static void a_schrodinger(float x, float y, float w, float h, float u)
{
    // precession de Larmor : le qubit tourne autour de z (champ magnetique)
    float cx = x + w / 2, cy = y + h / 2 + 6, r = h * 0.36f;
    float th = 0.9f, ph = u * 2.2f;
    static float tr[3 * 40];
    for (int i = 0; i < 40; i++) {
        float p = ph - i * 0.08f;
        tr[3 * (39 - i)] = sinf(th) * cosf(p);
        tr[3 * (39 - i) + 1] = sinf(th) * sinf(p);
        tr[3 * (39 - i) + 2] = cosf(th);
    }
    sphere_bloch(cx, cy, r, 0.5f, 0.38f, sinf(th) * cosf(ph), sinf(th) * sinf(ph), cosf(th), VERT, false, tr, 40);
    fleche(cx, cy + r + 8, cx, cy - r - 12, 1.2f, ROSE, 180);
    texte((int)cx + 4, (int)(cy - r - 14), "B", POLICE_11, ROSE);
}

static void a_propres(float x, float y, float w, float h, float u)
{
    // niveaux d'energie d'une molecule : l'electron saute, un photon part
    static const float E[4] = { 0.92f, 0.62f, 0.38f, 0.2f };
    float x0 = x + 12, x1 = x + w * 0.6f;
    for (int i = 0; i < 4; i++) {
        float yy = y + 6 + (h - 16) * E[i];
        trait(x0, yy, x1, yy, 1.6f, i == 0 ? CYAN : TEXTE2);
        char b[8];
        snprintf(b, sizeof b, "E%s", i == 0 ? "₀" : (i == 1 ? "₁" : (i == 2 ? "₂" : "₃")));
        texte((int)x1 + 4, (int)yy - 7, b, POLICE_11, TEXTE2);
    }
    float cyc = fmodf(u * 0.6f, 1.0f);
    int haut = 1 + ((int)(u * 0.6f)) % 3;
    float ya = y + 6 + (h - 16) * E[haut], yb = y + 6 + (h - 16) * E[0];
    float ye = cyc < 0.6f ? ya : ya + (yb - ya) * doux((cyc - 0.6f) / 0.15f);
    float xe = (x0 + x1) / 2;
    halo(xe, ye, 10, JAUNE, 180);
    disque(xe, ye, 3.5f, JAUNE);
    if (cyc > 0.7f) {
        float k = (cyc - 0.7f) / 0.3f;
        float px = xe + k * (w * 0.45f);
        uint16_t c = couleur_phase(haut * 1.4f);
        float pxp = 0, pyp = 0;
        for (int i = 0; i <= 12; i++) {
            float xx = px - i * 2.5f, yy = (ya + yb) / 2 + 4 * sinf(i * 1.2f - u * 20);
            if (i) trait(pxp, pyp, xx, yy, 1.6f, c, (uint8_t)(255 - i * 18));
            pxp = xx; pyp = yy;
        }
        halo(px, (ya + yb) / 2, 8, c, 200);
    }
}

static void a_grover(float x, float y, float w, float h, float u)
{
    // 8 reponses : l'amplitude de la bonne grandit a chaque tour (sin((2k+1)θ))
    int tour = ((int)(u * 0.7f)) % 3;
    float k = doux(fmodf(u * 0.7f, 1.0f) * 2);
    float th = asinf(1 / sqrtf(8));
    float am0 = sinf((2 * (tour > 0 ? tour - 1 : 0) + 1) * th), am1 = sinf((2 * tour + 1) * th);
    float am = am0 + (am1 - am0) * k;
    float autres = sqrtf((1 - am * am) / 7);
    float bw = (w - 10) / 8;
    float base = y + h - 16;
    for (int i = 0; i < 8; i++) {
        bool cible = i == 5;
        float v = cible ? am : autres;
        float hh = (h - 30) * v * v;
        uint16_t c = cible ? JAUNE : CYAN;
        if (cible) halo(x + 5 + i * bw + bw / 2, base - hh, 12, JAUNE, 120);
        rect_arrondi_degrade((int)(x + 5 + i * bw), (int)(base - hh), (int)bw - 3, (int)hh + 1, 2, c, melange(c, NOIR_BLEU, 140));
    }
    char b[40], p[16];
    fmt_pct(p, sizeof p, am * am);
    snprintf(b, sizeof b, "tour %d : %s", tour, p);
    texte((int)(x + w / 2), (int)y + 2, b, POLICE_11, JAUNE, 255, CENTRE);
    texte((int)(x + w / 2), (int)base + 2, "000  …  101  …  111", POLICE_11, TEXTE3, 255, CENTRE);
}

static void a_qft(float x, float y, float w, float h, float u)
{
    // 8 roues : la k-ieme tourne k fois plus vite (les frequences de Fourier)
    for (int k = 0; k < 8; k++) {
        float cx = x + 14 + (k % 4) * (w - 28) / 3, cy = y + 18 + (k / 4) * (h - 34);
        float a = u * 1.2f * k;
        uint16_t c = couleur_phase(a);
        anneau(cx, cy, 10, 1, BORD);
        halo(cx, cy, 12, c, 50);
        fleche(cx, cy, cx + 9 * cosf(a), cy - 9 * sinf(a), 1.6f, c);
    }
    texte((int)(x + w / 2), (int)(y + h - 12), "la k-ième tourne k fois", POLICE_11, TEXTE3, 255, CENTRE);
}

// ============================================================ les pages
static const Page PAGES[] = {
    { "Les équations du quantique", "sommaire", CYAN,
      "|ψ⟩ = α|0⟩ + β|1⟩", 2, nullptr,
      "Un ordinateur quantique calcule avec des vecteurs de nombres complexes. Chaque page montre un type "
      "d'équation, ce qu'il dit, et une animation. Glisse pour avancer.",
      a_intro, -1 },
    { "La superposition", "équation linéaire", CYAN,
      "|ψ⟩ = \\c1{α}|0⟩ + \\c2{β}|1⟩", 2, nullptr,
      "L'état d'un qubit est un vecteur : un mélange (une combinaison linéaire) de |0⟩ et |1⟩. "
      "Les coefficients α et β sont des nombres complexes : les amplitudes.",
      a_lineaire, 0 },
    { "Normalisation, règle de Born", "équation quadratique", VERT,
      "|α|^{2} + |β|^{2} = 1", 2, "P(0) = |α|^{2}      P(1) = |β|^{2}",
      "Pour obtenir une probabilité, on élève l'amplitude au carré. La somme fait toujours 1 : "
      "c'est une contrainte du second degré.",
      a_born, 0 },
    { "La sphère de Bloch", "équation paramétrique", JAUNE,
      "|ψ⟩ = cos\\f{θ}{2}|0⟩ + e^{iφ} sin\\f{θ}{2}|1⟩", 1, nullptr,
      "Deux angles suffisent : θ (latitude) et φ (longitude). Un état = un point de la sphère. "
      "Nord = |0⟩, sud = |1⟩, équateur = 50/50.",
      a_bloch, -2 },
    { "La phase : formule d'Euler", "identité complexe", MAGENTA,
      "e^{iφ} = cos φ + i sin φ", 2, nullptr,
      "Une phase est une flèche qui tourne. Invisible à la mesure, elle décide pourtant des interférences. "
      "C'est la même formule que pour Fourier (saison 3) !",
      a_euler, 1 },
    { "Les portes sont des matrices", "équation matricielle", CYAN,
      "|ψ'⟩ = U|ψ⟩      H = \\f{1}{\\r{2}}\\m{1,1;1,−1}", 1, nullptr,
      "Une porte multiplie le vecteur d'état par une matrice. Hadamard (H) change |0⟩ en (|0⟩ + |1⟩)/√2 : "
      "la superposition.",
      a_matrice, 0 },
    { "Unitarité : tout est réversible", "équation de contrainte", VIOLET,
      "U^{†}U = I", 2, nullptr,
      "U† (la transposée conjuguée) défait U : on peut toujours revenir en arrière. Sauf après une mesure, "
      "qui détruit l'information.",
      a_unitaire, -2 },
    { "Rotations : l'exponentielle", "exponentielle de matrice", VIOLET,
      "R_{x}(θ) = e^{−iθX/2} = cos\\f{θ}{2} I − i sin\\f{θ}{2} X", 0, nullptr,
      "Toute porte à un qubit est une rotation de la sphère. L'exponentielle d'une matrice suit Euler : "
      "on remplace i par la matrice X.",
      a_rotation, -2 },
    { "Plusieurs qubits", "produit tensoriel", ORANGE,
      "|a⟩ ⊗ |b⟩ = |ab⟩", 2, "n qubits → 2^{n} amplitudes",
      "Le nombre d'amplitudes double à chaque qubit. Avec 300 qubits, il y en a plus que d'atomes dans "
      "l'univers observable !",
      a_tensoriel, 11 },
    { "L'intrication", "équation sans solution", ROSE,
      "\\f{|00⟩ + |11⟩}{\\r{2}} ≠ |a⟩ ⊗ |b⟩", 1, nullptr,
      "Pour l'écrire (a₀|0⟩+a₁|1⟩)⊗(b₀|0⟩+b₁|1⟩), il faudrait a₀b₁ = 0, a₀b₀ ≠ 0 et a₁b₁ ≠ 0 : impossible. "
      "Les deux qubits n'ont plus d'état chacun.",
      a_intrication, 2 },
    { "La mesure", "équation probabiliste", ROUGE,
      "P(x) = |⟨x|ψ⟩|^{2}", 2, "après la mesure :  |ψ⟩ → |x⟩",
      "On obtient x au hasard, avec la probabilité |⟨x|ψ⟩|² (règle de Born). Puis l'état « s'effondre » sur |x⟩.",
      a_mesure, 0 },
    { "L'équation de Schrödinger", "équation différentielle", VERT,
      "iħ\\f{d|ψ⟩}{dt} = Ĥ|ψ⟩  ⇒  |ψ(t)⟩ = e^{−iĤt/ħ}|ψ(0)⟩", 0, nullptr,
      "Elle dit comment l'état change avec le temps. Sa solution est une exponentielle : une rotation. "
      "Un spin tourne autour d'un champ magnétique.",
      a_schrodinger, -2 },
    { "Valeurs propres : l'énergie", "équation aux valeurs propres", JAUNE,
      "Ĥ|E⟩ = E|E⟩", 2, nullptr,
      "Ĥ agit sur un état propre sans le changer : il le multiplie par un nombre, l'énergie E. "
      "Les trouver, c'est la chimie quantique.",
      a_propres, 10 },
    { "Interférence et Grover", "équation trigonométrique", JAUNE,
      "P_{k} = sin^{2}((2k + 1)θ)     sin θ = \\f{1}{\\r{N}}", 1, nullptr,
      "Les amplitudes s'ajoutent comme des vagues : ½ + ½ = 1, ½ − ½ = 0. Grover fait grandir la bonne "
      "réponse : environ √N tours au lieu de N.",
      a_grover, 7 },
    { "Fourier quantique (QFT)", "somme (transformée)", CYAN,
      "y_{k} = \\f{1}{\\r{N}} Σ_{j} x_{j} e^{2πi jk/N}", 1, nullptr,
      "La transformée de Fourier du son, mais sur les amplitudes, avec n² portes pour N = 2ⁿ valeurs. "
      "Elle trouve des périodes : le cœur de Shor.",
      a_qft, 9 },
};
static const int NB_PAGES = sizeof PAGES / sizeof PAGES[0];

void tuto_aller(int p)
{
    page = p < 0 ? 0 : (p >= NB_PAGES ? NB_PAGES - 1 : p);
    page_t0 = t;
}

void ecran_tuto()
{
    int g = glisse_page();
    if (g) { tuto_aller(page + g); glisse_x = g * 40.0f; }
    glisse_x = approche(glisse_x, 0, 12);
    const Page &P = PAGES[page];
    float u = t - page_t0;
    float e = sortie(u / 0.35f);
    uint8_t a = (uint8_t)(255 * e);
    int dx = (int)glisse_x;

    // titre et type d'equation
    texte(8 + dx, ZY0 + 1, P.titre, POLICE_16, TEXTE, a);
    int lw = largeur_texte(P.type, POLICE_11) + 16;
    int by = ZY0 + 20;
    rect_arrondi(8 + dx, by, lw, 15, 7, melange(NOIR_BLEU, P.couleur, 80), a);
    contour_arrondi(8 + dx, by, lw, 15, 7, 1, P.couleur, a);
    texte(16 + dx, by + 1, P.type, POLICE_11, TEXTE, a);
    char num[12];
    snprintf(num, sizeof num, "%d / %d", page + 1, NB_PAGES);
    texte(L - 8, by + 1, num, POLICE_11, TEXTE3, 255, DROITE);

    // la formule, dans un bandeau lumineux (hauteur selon la formule)
    float w1, a1, d1, w2 = 0, a2 = 0, d2 = 0;
    mesure_formule(P.formule, P.taille, w1, a1, d1);
    if (P.formule2) mesure_formule(P.formule2, 1, w2, a2, d2);
    int fy = ZY0 + 38;
    int fh = (int)(10 + a1 + d1 + (P.formule2 ? 4 + a2 + d2 : 0));
    rect_arrondi(6, fy, L - 12, fh, 10, rgb(10, 10, 36), 220);
    contour_arrondi(6, fy, L - 12, fh, 10, 1.2f, P.couleur, (uint8_t)(120 + 80 * pulse(0.5f)));
    halo(160, fy + fh / 2.0f, 120, P.couleur, (uint8_t)(16 * e));
    float yb1 = fy + 5 + a1;
    formule(160 + dx, yb1, P.formule, P.taille, TEXTE, a, CENTRE);
    if (P.formule2) formule(160 + dx, yb1 + d1 + 4 + a2, P.formule2, 1, P.couleur, a, CENTRE);

    // animation a gauche, explication a droite
    int ay = fy + fh + 4, ah = ZY1 - ay - 24;
    rect_arrondi(6, ay, 116, ah, 8, PANNEAU, 140);
    decoupe(7, ay + 1, 121, ay + ah - 1);
    P.anim(8 + dx, ay + 2, 112, ah - 4, u);
    decoupe(0, 25, L, 204);
    paragraphe(128 + dx, ay + 1, L - 132, P.texte, POLICE_11, TEXTE2, a, 12);

    // navigation
    int ny = ZY1 - 21;
    if (petit_bouton(6, ny, 28, 19, "‹", CYAN, page > 0)) tuto_aller(page - 1);
    if (petit_bouton(L - 34, ny, 28, 19, "›", CYAN, page < NB_PAGES - 1)) tuto_aller(page + 1);
    fleches_pages(page, NB_PAGES, page_t0);
    if (P.essayer != -1) {
        if (petit_bouton(40, ny, 62, 19, "Essayer", JAUNE)) {
            if (P.essayer == -2) {
                aller_onglet(O_SPHERE);
                sphere_mode(1);
                toast("Bac à sable : touche les portes", JAUNE);
            } else {
                q::charger_algo(circ, P.essayer);
                circuit_modifie();
                aller_onglet(O_CIRCUIT);
                lecture_jouer();
                char b[64];
                snprintf(b, sizeof b, "Chargé : %s", q::ALGOS[P.essayer].court);
                toast(b, JAUNE);
            }
        }
    }
}

}  // namespace ui
