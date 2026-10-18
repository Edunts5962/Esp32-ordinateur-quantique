// ecran_algos.cpp - onglet ALGOS : les grands algorithmes quantiques connus,
// a quoi ils servent (ou serviront) dans la vie, le gain espere sur un
// ordinateur classique et ou on en est aujourd'hui. Chaque page a son
// animation ; "Essayer" charge une demonstration dans le circuit, "Vrai QPU"
// prepare son export vers un vrai ordinateur quantique.
#include "ui_commun.h"
#include <stdio.h>
#include <string.h>

namespace ui {

static int page = 0;
static float page_t0 = 0;
static float glisse_x = 0;

typedef void (*FnAnim)(float x, float y, float w, float h, float u);

struct Fiche {
    const char *titre;
    const char *origine;       // auteur, annee
    const char *gain;          // badge
    uint16_t c_gain;
    const char *usages[3];     // a quoi ca sert
    int maturite;              // 0..5
    const char *aujourdhui;    // ou on en est
    FnAnim anim;
    int essayer;               // algorithme de la console, ou -1
};

// ============================================================ outils
static void fleche(float x0, float y0, float x1, float y1, float ep, uint16_t c, uint8_t a = 255)
{
    trait(x0, y0, x1, y1, ep, c, a);
    float an = atan2f(y1 - y0, x1 - x0);
    triangle(x1, y1, x1 - 7 * cosf(an - 0.4f), y1 - 7 * sinf(an - 0.4f), x1 - 7 * cosf(an + 0.4f), y1 - 7 * sinf(an + 0.4f), c, a);
}

static float hash01(int i) { uint32_t h = (uint32_t)i * 2654435761u; h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15; return (h & 0xFFFF) / 65535.0f; }

// ============================================================ animations
static void a_intro(float x, float y, float w, float h, float u)
{
    // une puce quantique : une grille de qubits qui scintillent, parfois une erreur
    float cx = x + w / 2, cy = y + h / 2 - 6;
    rect_arrondi((int)(cx - 42), (int)(cy - 30), 84, 60, 8, rgb(30, 26, 70));
    contour_arrondi((int)(cx - 42), (int)(cy - 30), 84, 60, 8, 1.4f, VIOLET);
    for (int i = 0; i < 6; i++) {
        trait(cx - 42 - 7, cy - 24 + i * 9.6f, cx - 42, cy - 24 + i * 9.6f, 2, JAUNE, 180);
        trait(cx + 42, cy - 24 + i * 9.6f, cx + 42 + 7, cy - 24 + i * 9.6f, 2, JAUNE, 180);
    }
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 6; i++) {
            int k = j * 6 + i;
            float px = cx - 33 + i * 13.2f, py = cy - 20 + j * 13.3f;
            float s = pulse(0.6f + hash01(k) * 1.4f, k);
            bool err = fmodf(u * 0.8f + hash01(k + 50) * 7, 7.0f) < 0.25f;
            uint16_t c = err ? ROUGE : couleur_phase(k * 0.7f + u);
            halo(px, py, 7, c, (uint8_t)(60 + 120 * s));
            disque(px, py, 2.6f, err ? ROUGE : TEXTE);
            if (i < 5) trait(px + 3, py, px + 11, py, 0.8f, GRILLE);
            if (j < 3) trait(px, py + 3, px, py + 11, 0.8f, GRILLE);
        }
    texte((int)cx, (int)(y + h - 13), "rouge = erreur", POLICE_11, TEXTE3, 255, CENTRE);
}

static void a_shor(float x, float y, float w, float h, float u)
{
    // 7^x mod 15 = 1, 7, 4, 13, 1, 7, 4, 13 ... periode 4  ->  15 = 3 x 5
    static const int V[8] = { 1, 7, 4, 13, 1, 7, 4, 13 };
    float cyc = fmodf(u * 0.25f, 1.0f);
    int nb = (int)(cyc * 20);
    if (nb > 8) nb = 8;
    float bw = (w - 12) / 8, base = y + h - 20;
    texte((int)(x + 2), (int)y, "7ˣ mod 15", POLICE_11, TEXTE2);
    for (int i = 0; i < nb; i++) {
        float hh = (h - 44) * V[i] / 13.0f;
        uint16_t c = couleur_phase(i * PI_F / 2);
        rect_arrondi_degrade((int)(x + 6 + i * bw), (int)(base - hh), (int)bw - 3, (int)hh + 1, 2, c, melange(c, NOIR_BLEU, 130));
        char b[4];
        snprintf(b, sizeof b, "%d", V[i]);
        texte((int)(x + 6 + i * bw + bw / 2 - 1), (int)(base - hh) - 12, b, POLICE_11, TEXTE, 255, CENTRE);
    }
    if (cyc > 0.45f) {
        float k = sortie((cyc - 0.45f) / 0.1f);
        float x0 = x + 6 + bw / 2, x1 = x + 6 + 4 * bw + bw / 2;
        uint8_t a = (uint8_t)(255 * k);
        trait(x0, base + 3, x1, base + 3, 1.6f, JAUNE, a);
        trait(x0, base, x0, base + 6, 1.6f, JAUNE, a);
        trait(x1, base, x1, base + 6, 1.6f, JAUNE, a);
        texte((int)((x0 + x1) / 2), (int)base + 5, "période 4", POLICE_11, JAUNE, a, CENTRE);
    }
    if (cyc > 0.65f) {
        float k = sortie((cyc - 0.65f) / 0.12f);
        rect_arrondi((int)(x + w / 2 - 50), (int)(y + h / 2 - 16), 100, 26, 10, PANNEAU2, (uint8_t)(240 * k));
        halo(x + w / 2, y + h / 2 - 3, 50 * k, VERT, (uint8_t)(90 * k));
        texte((int)(x + w / 2), (int)(y + h / 2 - 12), "15 = 3 × 5", POLICE_16, VERT, (uint8_t)(255 * k), CENTRE);
        if (cyc < 0.68f) eclat(x + w / 2, y + h / 2, VERT, 3);
    }
}

static void a_grover(float x, float y, float w, float h, float u)
{
    int tour = ((int)(u * 0.6f)) % 4;
    float k = doux(fmodf(u * 0.6f, 1.0f) * 1.6f);
    float th = asinf(1 / sqrtf(8));
    float a0 = tour == 0 ? sinf(th) : sinf((2 * (tour - 1) + 1) * th);
    float a1 = tour == 0 ? sinf(th) : sinf((2 * tour + 1) * th);
    if (tour == 3) { a0 = a1 = sinf(5 * th); }
    float am = a0 + (a1 - a0) * k;
    float autres = sqrtf((1 - am * am) / 7);
    float bw = (w - 10) / 8, base = y + h - 18;
    // la moyenne (inversion par rapport a la moyenne)
    for (int i = 0; i < 8; i++) {
        bool cible = i == 5;
        float v = cible ? am : autres;
        float hh = (h - 40) * v * v;
        uint16_t c = cible ? JAUNE : CYAN;
        if (cible) { halo(x + 5 + i * bw + bw / 2, base - hh, 14, JAUNE, 140); }
        rect_arrondi_degrade((int)(x + 5 + i * bw), (int)(base - hh), (int)bw - 3, (int)hh + 1, 2, c, melange(c, NOIR_BLEU, 140));
    }
    // la loupe qui balaie
    float lx = x + 5 + bw * (tour == 3 ? 5.5f : 0.5f + 7 * pulse(0.25f));
    anneau(lx, y + 14, 7, 1.6f, TEXTE);
    trait(lx + 5, y + 19, lx + 10, y + 24, 2, TEXTE);
    char b[40], p[16];
    fmt_pct(p, sizeof p, am * am);
    snprintf(b, sizeof b, "tour %d : %s", tour > 2 ? 2 : tour, p);
    texte((int)(x + w - 4), (int)y + 4, b, POLICE_11, JAUNE, 255, DROITE);
    texte((int)(x + w / 2), (int)base + 3, "8 tiroirs, 1 trésor", POLICE_11, TEXTE3, 255, CENTRE);
}

static void a_qft(float x, float y, float w, float h, float u)
{
    for (int k = 0; k < 8; k++) {
        float cx = x + 16 + (k % 4) * (w - 32) / 3, cy = y + 18 + (k / 4) * (h - 36);
        float a = u * 1.1f * k;
        uint16_t c = couleur_phase(a);
        halo(cx, cy, 14, c, 60);
        anneau(cx, cy, 11, 1, BORD);
        fleche(cx, cy, cx + 10 * cosf(a), cy - 10 * sinf(a), 1.8f, c);
    }
}

static void a_phase(float x, float y, float w, float h, float u)
{
    // un cadran : l'aiguille tourne d'une fraction de tour, les bits l'ecrivent
    float cx = x + w / 2, cy = y + h / 2 - 6, r = h * 0.30f;
    anneau(cx, cy, r, 2, BORD);
    for (int i = 0; i < 8; i++) {
        float a = i * PI_F / 4;
        trait(cx + (r - 5) * cosf(a), cy - (r - 5) * sinf(a), cx + r * cosf(a), cy - r * sinf(a), 1.4f, TEXTE2);
    }
    float cible = PI_F / 4;
    float a = cible * doux(fmodf(u * 0.4f, 1.0f) * 1.5f);
    halo(cx + r * cosf(a), cy - r * sinf(a), 10, JAUNE, 180);
    fleche(cx, cy, cx + (r - 2) * cosf(a), cy - (r - 2) * sinf(a), 2.4f, JAUNE);
    arc(cx, cy, r * 0.45f, 3, -a, 0, MAGENTA, 200);
    texte((int)cx, (int)(y + h - 16), "1/8 de tour = 0,001₂", POLICE_11, JAUNE, 255, CENTRE);
}

static void a_simulation(float x, float y, float w, float h, float u)
{
    // une molecule : des atomes relies, des nuages d'electrons
    struct A { float x, y, r; uint16_t c; };
    const A at[5] = { { 0.5f, 0.45f, 9, ROUGE }, { 0.25f, 0.3f, 6, TEXTE }, { 0.75f, 0.3f, 6, TEXTE },
                      { 0.3f, 0.75f, 7, BLEU }, { 0.72f, 0.72f, 7, BLEU } };
    for (int i = 1; i < 5; i++)
        trait(x + w * at[0].x, y + h * at[0].y, x + w * at[i].x, y + h * at[i].y, 3, TEXTE3);
    for (int i = 0; i < 5; i++) {
        float px = x + w * at[i].x + 1.5f * sinf(u * 5 + i), py = y + h * at[i].y + 1.5f * cosf(u * 4 + i);
        for (int j = 0; j < 3; j++) {
            float a = u * (2 + j) + i * 2 + j * 2.1f;
            float ex = px + at[i].r * 2.2f * cosf(a), ey = py + at[i].r * 1.2f * sinf(a);
            halo(ex, ey, 5, CYAN, 110);
            disque(ex, ey, 1.3f, TEXTE);
        }
        halo(px, py, at[i].r * 2, at[i].c, 100);
        disque(px, py, at[i].r, at[i].c);
        disque(px - at[i].r * 0.3f, py - at[i].r * 0.3f, at[i].r * 0.3f, TEXTE, 120);
    }
}

static void a_vqe(float x, float y, float w, float h, float u)
{
    // E(θ) : un paysage d'energie ; l'ordinateur classique fait rouler la bille au fond
    float x0 = x + 8, x1 = x + w - 8, yb = y + h - 14;
    float px = 0, py = 0;
    auto E = [](float s) { return 0.55f + 0.35f * cosf(s * 2 * 3.14159f * 1.3f + 0.6f) + 0.1f * cosf(s * 9); };
    for (int i = 0; i <= 50; i++) {
        float s = i / 50.0f, xx = x0 + (x1 - x0) * s, yy = yb - (h - 40) * (1 - E(s));
        if (i) { trait(px, py, xx, yy, 4, VERT, 50); trait(px, py, xx, yy, 1.8f, VERT); }
        px = xx; py = yy;
    }
    // descente de gradient
    float cyc = fmodf(u * 0.35f, 1.0f);
    float s = 0.08f;
    for (int it = 0; it < (int)(cyc * 40); it++) {
        float d = (E(s + 0.01f) - E(s - 0.01f)) / 0.02f;
        s -= 0.012f * d;
        if (s < 0) s = 0;
        if (s > 1) s = 1;
    }
    float bx_ = x0 + (x1 - x0) * s, by_ = yb - (h - 40) * (1 - E(s)) - 6;
    halo(bx_, by_, 12, JAUNE, 200);
    disque(bx_, by_, 5, JAUNE);
    texte((int)(x + 4), (int)y + 2, "E(θ) = ⟨ψ(θ)|Ĥ|ψ(θ)⟩", POLICE_11, TEXTE2);
    texte((int)(x + w / 2), (int)yb, "θ (réglé par le PC)", POLICE_11, TEXTE3, 255, CENTRE);
}

static void a_qaoa(float x, float y, float w, float h, float u)
{
    // coupe maximale : colorier les villes en 2 groupes pour couper le plus de routes
    const float P[6][2] = { { 0.2f, 0.25f }, { 0.55f, 0.15f }, { 0.85f, 0.35f }, { 0.75f, 0.8f }, { 0.35f, 0.75f }, { 0.5f, 0.48f } };
    const int AR[9][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 }, { 4, 0 }, { 5, 0 }, { 5, 2 }, { 5, 3 }, { 1, 4 } };
    static const int BON = 0b101010 ^ 0b000001;
    int etape = ((int)(u * 1.2f)) % 12;
    int col = etape >= 9 ? BON : (int)(hash01(etape * 7 + 1) * 64);
    int coupes = 0;
    for (int i = 0; i < 9; i++) {
        int a = AR[i][0], b = AR[i][1];
        bool cut = ((col >> a) & 1) != ((col >> b) & 1);
        coupes += cut;
        float ax = x + w * P[a][0], ay = y + 6 + (h - 26) * P[a][1], bx2 = x + w * P[b][0], by2 = y + 6 + (h - 26) * P[b][1];
        if (cut) trait(ax, ay, bx2, by2, 5, JAUNE, 70);
        trait(ax, ay, bx2, by2, cut ? 2 : 1, cut ? JAUNE : TEXTE3);
    }
    for (int i = 0; i < 6; i++) {
        float px = x + w * P[i][0], py = y + 6 + (h - 26) * P[i][1];
        uint16_t c = (col >> i) & 1 ? MAGENTA : CYAN;
        halo(px, py, 10, c, 150);
        disque(px, py, 5, c);
    }
    char b[32];
    snprintf(b, sizeof b, "%d routes coupées%s", coupes, etape >= 9 ? " : le max !" : "");
    texte((int)(x + w / 2), (int)(y + h - 15), b, POLICE_11, etape >= 9 ? VERT : TEXTE2, 255, CENTRE);
}

static void a_hhl(float x, float y, float w, float h, float u)
{
    float k = doux(fmodf(u * 0.4f, 1.0f) * 1.4f);
    formule(x + w / 2, y + 30, "A\\m{x₁;x₂;x₃} = \\m{b₁;b₂;b₃}", 0, TEXTE, 255, CENTRE);
    // la solution qui apparait : amplitudes = composantes de x
    static const float S[4] = { 0.6f, 0.2f, 0.7f, 0.35f };
    float bw = (w - 30) / 4;
    for (int i = 0; i < 4; i++) {
        float hh = (h - 62) * S[i] * k;
        uint16_t c = couleur_phase(i * 1.2f);
        rect_arrondi_degrade((int)(x + 15 + i * bw), (int)(y + h - 14 - hh), (int)bw - 4, (int)hh + 1, 3, c, melange(c, NOIR_BLEU, 140));
    }
    texte((int)(x + w / 2), (int)(y + h - 13), "x dans les amplitudes", POLICE_11, TEXTE3, 255, CENTRE);
}

static void a_ia(float x, float y, float w, float h, float u)
{
    // deux nuages de points et une frontiere courbe qui s'ajuste
    for (int i = 0; i < 26; i++) {
        float a = hash01(i) * 6.28f, r = 0.15f + 0.25f * hash01(i + 99);
        bool cl = i % 2;
        float cx = cl ? 0.68f : 0.32f, cy = cl ? 0.35f : 0.62f;
        float px = x + w * (cx + r * cosf(a) * 0.7f), py = y + (h - 16) * (cy + r * sinf(a) * 0.7f);
        halo(px, py, 5, cl ? MAGENTA : CYAN, 110);
        disque(px, py, 2, cl ? MAGENTA : CYAN);
    }
    float ph = 0.5f * sinf(u * 0.8f) * expf(-0.05f * fmodf(u, 30));
    float px = 0, py = 0;
    for (int i = 0; i <= 30; i++) {
        float s = i / 30.0f;
        float xx = x + w * s, yy = y + (h - 16) * (0.15f + 0.75f * s + 0.12f * sinf(s * 6 + ph));
        if (i) { trait(px, py, xx, yy, 4, JAUNE, 60); trait(px, py, xx, yy, 1.6f, JAUNE); }
        px = xx; py = yy;
    }
}

static void a_bb84(float x, float y, float w, float h, float u)
{
    // Alice envoie des photons polarises a Bob ; si Eve espionne, des erreurs apparaissent
    float xa = x + 14, xb = x + w - 14, cy = y + h / 2 - 8;
    bool eve = fmodf(u, 10) > 5;
    texte((int)xa, (int)y + 2, "Alice", POLICE_11, CYAN, 255, CENTRE);
    texte((int)xb, (int)y + 2, "Bob", POLICE_11, VERT, 255, CENTRE);
    disque(xa, cy, 6, CYAN);
    disque(xb, cy, 6, VERT);
    if (eve) {
        float ex = x + w / 2;
        halo(ex, cy + 22, 12, ROUGE, (uint8_t)(120 + 100 * pulse(2)));
        disque(ex, cy + 22, 5, ROUGE);
        texte((int)ex, (int)cy + 30, "Ève", POLICE_11, ROUGE, 255, CENTRE);
    }
    for (int i = 0; i < 4; i++) {
        float f = fmodf(u * 0.5f + i * 0.25f, 1.0f);
        float px = xa + (xb - xa) * f;
        int pol = (int)(hash01((int)(u * 0.5f + i * 0.25f) * 4 + i) * 4);
        float a = pol * PI_F / 4;
        bool erreur = eve && f > 0.5f && hash01(i + (int)(u * 0.5f) * 9) > 0.5f;
        uint16_t c = erreur ? ROUGE : JAUNE;
        halo(px, cy, 8, c, 150);
        trait(px - 7 * cosf(a), cy - 7 * sinf(a), px + 7 * cosf(a), cy + 7 * sinf(a), 2, c);
    }
    texte((int)(x + w / 2), (int)(y + h - 14), eve ? "espion détecté : erreurs !" : "clé secrète partagée",
          POLICE_11, eve ? ROUGE : VERT, 255, CENTRE);
}

static void a_teleport(float x, float y, float w, float h, float u)
{
    float cyc = fmodf(u * 0.35f, 1.0f);
    float xa = x + 28, xb = x + w - 28, cy = y + h / 2 - 4;
    float th = 1.0f, ph = 0.8f;
    float ka = cyc < 0.5f ? 1 : 1 - sortie((cyc - 0.5f) / 0.15f);
    float kb = cyc < 0.6f ? 0 : sortie((cyc - 0.6f) / 0.15f);
    sphere_bloch(xa, cy, 20, 0.6f, 0.3f, sinf(th) * cosf(ph) * ka, sinf(th) * sinf(ph) * ka, cosf(th) * ka, CYAN, false);
    sphere_bloch(xb, cy, 20, 0.6f, 0.3f, sinf(th) * cosf(ph) * kb, sinf(th) * sinf(ph) * kb, cosf(th) * kb, VERT, false);
    // l'intrication : un lien ondule ; les 2 bits classiques voyagent dessous
    for (int i = 0; i < 14; i++) {
        float f = i / 13.0f;
        float xx = xa + 22 + (xb - xa - 44) * f, yy = cy + 5 * sinf(f * 10 - u * 5);
        disque(xx, yy, 1.4f, VIOLET, 200);
    }
    if (cyc > 0.45f && cyc < 0.65f) {
        float f = (cyc - 0.45f) / 0.2f;
        float bx = xa + (xb - xa) * f;
        rect_arrondi((int)bx - 12, (int)cy + 26, 24, 14, 5, PANNEAU2);
        texte((int)bx, (int)cy + 27, "01", POLICE_11, JAUNE, 255, CENTRE);
    }
    texte((int)xa, (int)(y + h - 13), "Alice", POLICE_11, CYAN, 255, CENTRE);
    texte((int)xb, (int)(y + h - 13), "Bob", POLICE_11, VERT, 255, CENTRE);
}

static void a_hasard(float x, float y, float w, float h, float u)
{
    // un flot de bits vraiment aleatoires
    for (int j = 0; j < 6; j++)
        for (int i = 0; i < 12; i++) {
            int k = (int)(u * 8) + i * 31 + j * 97;
            int b = hash01(k) > 0.5f;
            float fy = fmodf(u * 30 + j * 13, h);
            (void)fy;
            texte((int)(x + 8 + i * (w - 16) / 11), (int)(y + 4 + j * (h - 24) / 6), b ? "1" : "0", POLICE_11,
                  b ? MAGENTA : CYAN, (uint8_t)(110 + 140 * hash01(k + 7)), CENTRE);
        }
    float cx = x + w / 2, cy = y + h / 2 - 6;
    halo(cx, cy, 22 + 4 * sinf(u * 6), JAUNE, 150);
    rect_arrondi((int)cx - 15, (int)cy - 15, 30, 30, 6, PANNEAU2);
    contour_arrondi((int)cx - 15, (int)cy - 15, 30, 30, 6, 1.6f, JAUNE);
    int d = 1 + (int)(hash01((int)(u * 4)) * 6);
    char b[4];
    snprintf(b, sizeof b, "%d", d);
    texte((int)cx, (int)cy - 11, b, POLICE_16, JAUNE, 255, CENTRE);
}

static void a_correction(float x, float y, float w, float h, float u)
{
    // code de surface : des qubits de donnees (ronds) surveilles par des qubits de mesure (carres)
    int err = ((int)(u * 0.7f)) % 12;
    float cyc = fmodf(u * 0.7f, 1.0f);
    int eid = (err * 6 + 8) % 34;                 // un qubit de donnees (indice pair)
    for (int j = 0; j < 5; j++)
        for (int i = 0; i < 7; i++) {
            float px = x + 10 + i * (w - 20) / 6, py = y + 8 + j * (h - 30) / 4;
            int id = j * 7 + i;
            bool donnee = (id % 2) == 0;
            int d = id - eid;
            bool voisin = !donnee && ((abs(d) == 1 && (id / 7) == (eid / 7)) || abs(d) == 7);
            if (donnee) {
                bool e_ = id == eid && cyc < 0.7f;
                halo(px, py, 6, e_ ? ROUGE : CYAN, e_ ? 220 : 70);
                disque(px, py, 3, e_ ? ROUGE : TEXTE);
            } else {
                bool allume = voisin && cyc > 0.2f && cyc < 0.7f;
                if (allume) halo(px, py, 7, JAUNE, 160);
                rect((int)px - 3, (int)py - 3, 6, 6, allume ? JAUNE : VIOLET, allume ? 255 : 150);
            }
        }
    texte((int)(x + w / 2), (int)(y + h - 14), cyc < 0.7f ? "erreur repérée par ses voisins" : "corrigée !",
          POLICE_11, cyc < 0.7f ? JAUNE : VERT, 255, CENTRE);
}

static void a_oracle(float x, float y, float w, float h, float u)
{
    // une boite noire : une seule question suffit
    float cx = x + w / 2, cy = y + h / 2 - 6;
    rect_arrondi((int)cx - 26, (int)cy - 22, 52, 44, 8, rgb(10, 10, 20));
    contour_arrondi((int)cx - 26, (int)cy - 22, 52, 44, 8, 1.6f, VIOLET);
    texte((int)cx, (int)cy - 8, "f ?", POLICE_16, VIOLET, 255, CENTRE);
    float f = fmodf(u * 0.5f, 1.0f);
    if (f < 0.5f) {
        float px = x + 6 + (cx - 26 - x - 6) * f * 2;
        halo(px, cy, 9, CYAN, 180);
        texte((int)px, (int)cy - 18, "1 question", POLICE_11, CYAN, 255, CENTRE);
    } else {
        float px = cx + 26 + (x + w - 6 - cx - 26) * (f - 0.5f) * 2;
        halo(px, cy, 9, VERT, 180);
        texte((int)px, (int)cy - 18, "réponse", POLICE_11, VERT, 255, CENTRE);
    }
}

// ============================================================ les fiches
static const Fiche FICHES[] = {
    { "Où en est-on ?", "le calcul quantique en 2026", "à lire d'abord", CYAN,
      { "Des dizaines à plus de mille qubits par puce.",
        "Mais bruités : chaque porte se trompe un peu.",
        "Le défi : corriger les erreurs (qubits logiques)." },
      2, "Gain = vitesse face à un PC classique. Points : 1 = idée, 5 = déjà utilisé.", a_intro, -1 },
    { "Shor : factoriser", "Peter Shor, 1994", "exponentiel", VERT,
      { "Casser RSA : sites web, banques, cartes.",
        "D'où la crypto post-quantique (NIST, 2024).",
        "Son cœur : trouver une période (QFT)." },
      1, "Seulement de tout petits nombres (15, 21) : il faudra des qubits corrigés.", a_shor, 9 },
    { "Grover : chercher", "Lov Grover, 1996", "quadratique √N", JAUNE,
      { "Chercher sans index : √N essais au lieu de N.",
        "Logique (SAT), recherche de clés.",
        "AES-128 tombe à ≈ 64 bits : AES-256 conseillé." },
      1, "Tourne sur 2 ou 3 qubits ; un vrai gain exige la correction d'erreurs.", a_grover, 7 },
    { "Fourier quantique (QFT)", "Coppersmith, 1994", "exponentiel", VERT,
      { "Brique de Shor et de l'estimation de phase.",
        "Trouver périodes et fréquences cachées.",
        "n² portes pour 2ⁿ valeurs." },
      2, "Exécutée sur les processeurs actuels, pour quelques qubits.", a_qft, 9 },
    { "Estimation de phase", "Alexeï Kitaev, 1995", "exponentiel", VERT,
      { "Lire les énergies d'une molécule.",
        "Chimie de précision, nouveaux matériaux.",
        "Une étape de l'algorithme de Shor." },
      1, "Version complète : pour les futures machines corrigées.", a_phase, 10 },
    { "Simuler la nature", "Richard Feynman, 1981", "exponentiel", VERT,
      { "Catalyseurs (engrais), batteries, supraconducteurs.",
        "Nouveaux médicaments.",
        "Aimants et matière exotique." },
      2, "Premières simulations scientifiques de spins, encore débattues.", a_simulation, -1 },
    { "VQE : chimie hybride", "Peruzzo et al., 2014", "incertain", ORANGE,
      { "Énergie de petites molécules (H₂, LiH).",
        "Le PC règle θ, le QPU mesure l'énergie.",
        "Conçu pour les machines bruitées." },
      2, "Marche sur de petites molécules, sans battre les supercalculateurs.", a_vqe, -1 },
    { "QAOA : optimiser", "Farhi, Goldstone, Gutmann, 2014", "incertain", ORANGE,
      { "Tournées de livraison, plannings.",
        "Réseaux électriques, portefeuilles.",
        "Ex. : couper le plus de routes (Max-Cut)." },
      2, "Beaucoup d'essais ; aucun gain net démontré pour l'instant.", a_qaoa, -1 },
    { "HHL : équations linéaires", "Harrow, Hassidim, Lloyd, 2009", "exponentiel*", JAUNE,
      { "Résoudre A·x = b (structures, fluides).",
        "Finance, apprentissage automatique.",
        "* si A est creuse et la lecture de x limitée." },
      1, "Démos sur 2 × 2 ; ses conditions limitent l'avantage réel.", a_hhl, -1 },
    { "Apprentissage quantique", "depuis les années 2010", "incertain", ORANGE,
      { "Classer des données, reconnaître des formes.",
        "Analyser des données de capteurs quantiques.",
        "Domaine de recherche très actif." },
      1, "Aucun avantage démontré sur des données ordinaires : prudence !", a_ia, -1 },
    { "Cryptographie quantique", "BB84 : Bennett et Brassard, 1984", "sécurité", CYAN,
      { "Partager une clé secrète : l'espion est vu.",
        "Banques, États, satellite Micius (2017).",
        "Pas besoin d'ordinateur quantique." },
      4, "Des produits et des réseaux existent ; la distance reste limitée.", a_bb84, 2 },
    { "Téléportation quantique", "Bennett et al., 1993", "nouveau", CYAN,
      { "Envoyer un état : paire intriquée + 2 bits.",
        "Relier des processeurs quantiques.",
        "Base du futur internet quantique." },
      3, "Démontrée sur plus de 1 000 km (satellite, 2017).", a_teleport, 8 },
    { "Hasard quantique (QRNG)", "physique quantique", "vrai hasard", CYAN,
      { "Clés de chiffrement imprévisibles.",
        "Loteries, tirages, simulations.",
        "Des puces QRNG jusque dans des téléphones." },
      5, "Déjà commercialisé. Essaie : 5 qubits en H, puis la mesure !", a_hasard, 11 },
    { "Correction d'erreurs", "Shor 1995, codes de surface", "indispensable", VIOLET,
      { "1 qubit logique = beaucoup de qubits physiques.",
        "Indispensable pour Shor, Grover, la chimie.",
        "Repère les erreurs sans lire les données." },
      2, "2024 : agrandir le code réduit enfin les erreurs. Le grand chantier.", a_correction, -1 },
    { "Deutsch-Jozsa, Bernstein-V.", "1992 et 1993", "exponentiel (jouet)", VIOLET,
      { "Une seule question à une boîte noire.",
        "Problèmes artificiels, parfaits pour apprendre.",
        "Tournent sur tous les QPU du monde." },
      5, "Utilité pratique : nulle. Valeur pédagogique : énorme !", a_oracle, 5 },
};
static const int NB_FICHES = sizeof FICHES / sizeof FICHES[0];

void algos_aller(int p)
{
    if (p < 0) { page_t0 = t; return; }
    page = p >= NB_FICHES ? NB_FICHES - 1 : p;
    page_t0 = t;
}

void ecran_algos()
{
    int g = glisse_page();
    if (g) {
        int np = page + g;
        if (np >= 0 && np < NB_FICHES) { algos_aller(np); glisse_x = g * 40.0f; }
    }
    glisse_x = approche(glisse_x, 0, 12);
    const Fiche &F = FICHES[page];
    float u = t - page_t0;
    float e = sortie(u / 0.35f);
    uint8_t a = (uint8_t)(255 * e);
    int dx = (int)glisse_x;

    texte(8 + dx, ZY0 + 2, F.titre, POLICE_16, TEXTE, a);
    texte(8 + dx, ZY0 + 21, F.origine, POLICE_11, TEXTE3, a);
    // badge du gain
    char b[40];
    snprintf(b, sizeof b, "gain : %s", F.gain);
    int lw = largeur_texte(b, POLICE_11) + 16;
    rect_arrondi(L - 6 - lw, ZY0 + 20, lw, 15, 7, melange(NOIR_BLEU, F.c_gain, 80), a);
    contour_arrondi(L - 6 - lw, ZY0 + 20, lw, 15, 7, 1, F.c_gain, a);
    halo(L - 6 - lw / 2.0f, ZY0 + 27, lw * 0.5f, F.c_gain, (uint8_t)(30 * pulse(0.7f)));
    texte(L - 6 - lw + 8, ZY0 + 21, b, POLICE_11, TEXTE, a);

    // animation a gauche
    int ay = ZY0 + 38, ah = 84, aw = 126;
    rect_arrondi(6, ay, aw, ah, 8, PANNEAU, 150);
    contour_arrondi(6, ay, aw, ah, 8, 1, melange(BORD, F.c_gain, 60), 150);
    decoupe(7, ay + 1, 6 + aw - 1, ay + ah - 1);
    F.anim(8 + dx, ay + 2, aw - 4, ah - 4, u);
    decoupe(0, 25, L, 204);

    // a quoi ca sert
    int x = 138 + dx, y = ay - 1;
    texte(x, y, page == 0 ? "En bref" : "À quoi ça sert ?", POLICE_11, F.c_gain, a);
    y += 13;
    for (int i = 0; i < 3 && F.usages[i]; i++) {
        float k = sortie((u - 0.15f - i * 0.12f) / 0.3f);
        disque(x + 3.0f, y + 6.0f, 2, F.c_gain, (uint8_t)(255 * k));
        int n = paragraphe(x + 9, y, L - x - 12, F.usages[i], POLICE_11, TEXTE, (uint8_t)(255 * k), 12);
        y += n * 12 + 1;
    }
    // aujourd'hui : bandeau sous l'animation
    int sy = ay + ah + 3;
    rect_arrondi(6, sy, L - 12, 28, 8, PANNEAU2, 150);
    texte(12 + dx, sy + 2, "Aujourd'hui", POLICE_11, TEXTE2, a);
    jauge(14 + dx, sy + 16, F.maturite, 5, F.maturite >= 4 ? VERT : (F.maturite >= 2 ? JAUNE : ORANGE));
    paragraphe(80 + dx, sy + 2, L - 92, F.aujourdhui, POLICE_11, TEXTE, a, 12);

    // navigation et actions
    int ny = ZY1 - 21;
    if (petit_bouton(6, ny, 28, 19, "‹", CYAN, page > 0)) algos_aller(page - 1);
    if (petit_bouton(L - 34, ny, 28, 19, "›", CYAN, page < NB_FICHES - 1)) algos_aller(page + 1);
    fleches_pages(page, NB_FICHES, page_t0);
    if (F.essayer >= 0) {
        if (petit_bouton(40, ny, 58, 19, "Essayer", JAUNE)) {
            q::charger_algo(circ, F.essayer);
            circuit_modifie();
            aller_onglet(O_CIRCUIT);
            lecture_jouer();
            snprintf(b, sizeof b, "Chargé : %s", q::ALGOS[F.essayer].court);
            toast(b, JAUNE);
        }
        if (petit_bouton(L - 102, ny, 64, 19, "Vrai QPU", MAGENTA)) {
            q::charger_algo(circ, F.essayer);
            circuit_modifie();
            R.vue_mes = 2;
            aller_onglet(O_MESURES);
            toast("Envoie-le sur un vrai ordinateur quantique", MAGENTA);
        }
    }
}

}  // namespace ui
