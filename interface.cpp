// interface.cpp - console "Ordinateur quantique" : barres, navigation,
// demarrage, reglages, lecture pas a pas, tirs et outils communs.
// Les ecrans sont dans :
//   ecran_circuit.cpp : CIRCUIT (editeur, lecture pas a pas, amplitudes)
//   ecran_bloch.cpp   : SPHERE de Bloch (circuit ou bac a sable)
//   ecran_mesures.cpp : MESURES (tirs, histogramme, amplitudes) + ecran_qpu.cpp (vrai QPU)
//   ecran_algos.cpp   : ALGOS (les algorithmes quantiques et leur utilite)
//   ecran_tuto.cpp    : TUTO (les differents types d'equations)
#include "interface.h"
#include "ui_commun.h"
#include "plateforme.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace ui {

// ============================ Etat partage ==================================
float t = 0, dt = 0.033f;
Reglages R = { 0x51554231, 1, 0, 1, 0, 1, 1, 80, 0, 0, 0 };
Toucher tc;
q::Circuit circ;
Lecture lec;
q::Etat etat_aff;
float probas_aff[q::NE_MAX];
Tirs tirs;

static q::Etat etat_base, etat_cible;
static bool cible_ok = false;
static uint16_t *s_fond = nullptr;
static uint32_t ms_prec = 0;
static bool splash = true;
static float splash_t0 = 0;
static int onglet = O_CIRCUIT;
static float onglet_aff = 0;
static float t_sauver_circuit = -1;
static const char *ONGLETS[NB_ONGLETS] = { "Circuit", "Sphère", "Mesures", "Algos", "Tuto", "Réglages" };

static char toast_txt[96] = "";
static uint16_t toast_c = CYAN;
static float toast_t0 = -10;
static float seg_aff[24];
static bool seg_init[24];

struct Etoile { float x, y, v, ph; uint8_t lum; };
static Etoile etoiles[56];
struct Part { float x, y, vx, vy, vie; uint16_t c; };
static Part parts[96];

static bool p_appui = false;
static int p_x = 0, p_y = 0;
static float dernier_tap_t = -10;
static int dernier_tap_x = 0, dernier_tap_y = 0;

// ============================ Hasard ========================================
static uint32_t graine = 12345;
uint32_t alea32()
{
    if (R.hasard == 0) return plat_alea_materiel();
    graine ^= graine << 13; graine ^= graine >> 17; graine ^= graine << 5;
    return graine;
}

static uint32_t alea_e = 0x9E3779B9;
float alea()      // pour les animations seulement
{
    alea_e ^= alea_e << 13; alea_e ^= alea_e >> 17; alea_e ^= alea_e << 5;
    return (alea_e >> 8) * (1.0f / 16777216.0f);
}

int nb_tirs() { static const int N[3] = { 100, 1000, 8192 }; return N[R.tirs_i < 0 ? 0 : (R.tirs_i > 2 ? 2 : R.tirs_i)]; }
float bruit_porte() { static const float B[3] = { 0, 0.004f, 0.02f }; return B[R.bruit_i < 0 ? 0 : (R.bruit_i > 2 ? 2 : R.bruit_i)]; }
float bruit_lecture() { static const float B[3] = { 0, 0.005f, 0.025f }; return B[R.bruit_i < 0 ? 0 : (R.bruit_i > 2 ? 2 : R.bruit_i)]; }

void bip_mesure(int v)
{
    if (!R.son) return;
    plat_bip(v ? 1320.0f : 880.0f, 25, R.volume);
}

// ============================ Outils ========================================
void fmt(char *b, int n, double v, int dec)
{
    if (fabs(v) < 0.5 * pow(10.0, -dec)) v = 0;
    snprintf(b, n, "%.*f", dec, v);
    for (char *p = b; *p; p++) if (*p == '.') *p = ',';
}

void fmt_pct(char *b, int n, float p)
{
    char v[16];
    fmt(v, sizeof v, p * 100, p >= 0.995f || p < 0.0005f ? 0 : 1);
    snprintf(b, n, "%s %%", v);
}

void bits(char *b, int n, int i, int nb)
{
    int j = 0;
    for (int k = nb - 1; k >= 0 && j < n - 1; k--) b[j++] = (char)('0' + ((i >> k) & 1));
    b[j] = 0;
}

void ket(char *b, int n, int i, int nq)
{
    char s[8];
    bits(s, sizeof s, i, nq);
    snprintf(b, n, "|%s⟩", s);
}

uint16_t couleur_phase(float ph)
{
    // roue des couleurs : 0 cyan, π/2 vert, π rouge, 3π/2 violet
    float h = 0.5f - ph / (2 * PI_F);
    h -= floorf(h);
    float r = sat01(fabsf(h * 6 - 3) - 1), g = sat01(2 - fabsf(h * 6 - 2)), bl = sat01(2 - fabsf(h * 6 - 4));
    return rgb((uint8_t)(60 + 195 * r), (uint8_t)(60 + 195 * g), (uint8_t)(60 + 195 * bl));
}

uint16_t couleur_qubit(int k)
{
    static const uint16_t C[5] = { CYAN, MAGENTA, JAUNE, VERT, ORANGE };
    return C[((k % 5) + 5) % 5];
}

void sauver_reglages() { plat_nvs_ecrire("reglages", &R, sizeof R); }

void toast(const char *s, uint16_t c)
{
    strncpy(toast_txt, s, sizeof toast_txt - 1);
    toast_txt[sizeof toast_txt - 1] = 0;
    toast_c = c;
    toast_t0 = t;
}

void panneau(int x, int y, int l, int h, uint8_t a)
{
    rect_arrondi(x, y, l, h, 10, PANNEAU, a);
    contour_arrondi(x, y, l, h, 10, 1, BORD, 110);
}

void segments(int idx, int x, int y, int l, int h, const char *const *lab, int n, int sel)
{
    if (!seg_init[idx]) { seg_aff[idx] = (float)sel; seg_init[idx] = true; }
    seg_aff[idx] = approche(seg_aff[idx], (float)sel, 14);
    rect_arrondi(x, y, l, h, h / 2, PANNEAU2, 230);
    float w = (float)l / n;
    int sx = (int)(x + seg_aff[idx] * w);
    rect_arrondi_degrade(sx + 2, y + 2, (int)w - 4, h - 4, (h - 4) / 2, rgb(40, 190, 255), rgb(170, 70, 255));
    halo(sx + w / 2, (float)y + h, w * 0.5f, CYAN, 45);
    for (int i = 0; i < n; i++)
        texte((int)(x + w * i + w / 2), y + (h - 13) / 2, lab[i], POLICE_11, i == sel ? TEXTE : TEXTE2, 255, CENTRE);
}

int segment_touche(int x, int l, int n)
{
    int i = (tc.x0 - x) * n / l;
    return i < 0 ? 0 : (i >= n ? n - 1 : i);
}

void bouton_plat(int x, int y, int l, int h, const char *txt, uint16_t c0, uint16_t c1, bool actif)
{
    if (!actif) {
        rect_arrondi(x, y, l, h, h / 2 < 10 ? h / 2 : 10, PANNEAU2, 200);
        texte(x + l / 2, y + (h - 13) / 2, txt, POLICE_13, TEXTE3, 255, CENTRE);
        return;
    }
    bool presse = tc.appui && dans(tc.x, tc.y, x, y, l, h);
    float g = pulse(0.7f);
    halo(x + l / 2.0f, y + h / 2.0f, l * 0.6f, c0, (uint8_t)(18 + 22 * g));
    rect_arrondi_degrade(x, y, l, h, h / 2 < 10 ? h / 2 : 10, c0, c1, presse ? 170 : 255);
    decoupe(x, y, x + l, y + h);
    float sx = x - 30 + fmodf(t * 90, l + 120);
    trait(sx, (float)y - 2, sx - 16, (float)y + h + 2, 10, TEXTE, 60);
    sans_decoupe();
    texte(x + l / 2, y + (h - 13) / 2, txt, POLICE_13, NOIR_BLEU, 255, CENTRE);
}

bool bouton(int x, int y, int l, int h, const char *txt, uint16_t c0, uint16_t c1, bool actif)
{
    bouton_plat(x, y, l, h, txt, c0, c1, actif);
    return actif && tap_dans(x - 2, y - 3, l + 4, h + 6);
}

bool petit_bouton(int x, int y, int l, int h, const char *txt, uint16_t c, bool actif)
{
    bool presse = actif && tc.appui && dans(tc.x0, tc.y0, x, y, l, h);
    rect_arrondi(x, y, l, h, 6, presse ? c : PANNEAU2, presse ? 200 : 235);
    contour_arrondi(x, y, l, h, 6, 1, actif ? c : BORD, actif ? 170 : 80);
    texte(x + l / 2, y + (h - 13) / 2, txt, POLICE_13, actif ? (presse ? NOIR_BLEU : TEXTE) : TEXTE3, 255, CENTRE);
    return actif && tap_dans(x - 2, y - 2, l + 4, h + 4);
}

void curseur(int x0, int x1, int y, float f, bool actif, uint16_t c0, uint16_t c1)
{
    float kx = x0 + (x1 - x0) * sat01(f);
    rect_arrondi(x0, y - 3, x1 - x0, 6, 3, PANNEAU2);
    rect_arrondi_degrade(x0, y - 3, (int)(kx - x0) + 3, 6, 3, c0, c1);
    halo(kx, (float)y, 14, c1, actif ? 160 : 80);
    disque(kx, (float)y, 7, TEXTE);
    disque(kx, (float)y, 3, c1);
}

void jauge(int x, int y, int n, int max, uint16_t c)
{
    for (int i = 0; i < max; i++) {
        bool plein = i < n;
        if (plein) halo(x + i * 11 + 4.0f, y + 4.0f, 7, c, 60);
        disque(x + i * 11 + 4.0f, y + 4.0f, 3.6f, plein ? c : PANNEAU2);
        anneau(x + i * 11 + 4.0f, y + 4.0f, 3.6f, 1, plein ? c : BORD, 160);
    }
}

void eclat(float x, float y, uint16_t c, int n)
{
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < 96; i++) {
            if (parts[i].vie > 0) continue;
            float a = alea() * 2 * PI_F, v = 30 + alea() * 120;
            parts[i] = { x, y, cosf(a) * v, sinf(a) * v - 30, 0.6f + alea() * 0.8f, alea() < 0.3f ? TEXTE : c };
            break;
        }
    }
}

static void dessiner_particules()
{
    for (int i = 0; i < 96; i++) {
        Part &p = parts[i];
        if (p.vie <= 0) continue;
        p.vie -= dt;
        p.x += p.vx * dt; p.y += p.vy * dt;
        p.vy += 110 * dt;
        p.vx *= 1 - 1.5f * dt;
        uint8_t a = (uint8_t)(255 * sat01(p.vie));
        halo(p.x, p.y, 5, p.c, (uint8_t)(a / 2));
        disque(p.x, p.y, 1.4f, p.c, a);
    }
}

void triangle_plein(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c)
{
    if (y1 < y0) { float tx = x0, ty = y0; x0 = x1; y0 = y1; x1 = tx; y1 = ty; }
    if (y2 < y0) { float tx = x0, ty = y0; x0 = x2; y0 = y2; x2 = tx; y2 = ty; }
    if (y2 < y1) { float tx = x1, ty = y1; x1 = x2; y1 = y2; x2 = tx; y2 = ty; }
    int ya = (int)ceilf(y0 - 0.5f), yb = (int)ceilf(y2 - 0.5f);
    if (ya < 0) ya = 0;
    if (yb > H) yb = H;
    for (int y = ya; y < yb; y++) {
        float py = y + 0.5f;
        float xa = x0 + (x2 - x0) * (py - y0) / (y2 - y0 + 1e-9f);
        float xb = py < y1 ? x0 + (x1 - x0) * (py - y0) / (y1 - y0 + 1e-9f)
                           : x1 + (x2 - x1) * (py - y1) / (y2 - y1 + 1e-9f);
        if (xa > xb) { float tt = xa; xa = xb; xb = tt; }
        int ia = (int)ceilf(xa - 0.5f), ib = (int)ceilf(xb - 0.5f);
        if (ia < 0) ia = 0;
        if (ib > L) ib = L;
        uint16_t *p = fb + y * L;
        for (int x = ia; x < ib; x++) p[x] = c;
    }
}

// texte coupe aux espaces pour tenir dans la largeur l ; renvoie le nombre de lignes
int paragraphe(int x, int y, int l, const char *s, const PoliceAA &p, uint16_t c, uint8_t a, int interligne)
{
    char ligne[200];
    int n = 0;
    while (*s) {
        int fin = 0, dernier_espace = -1;
        int i = 0;
        for (;;) {
            if (s[i] == 0 || s[i] == '\n') { fin = i; break; }
            if (s[i] == ' ') {
                memcpy(ligne, s, i); ligne[i] = 0;
                if (largeur_texte(ligne, p) > l) { fin = dernier_espace > 0 ? dernier_espace : i; break; }
                dernier_espace = i;
            }
            i++;
            if (i >= (int)sizeof ligne - 1) { fin = dernier_espace > 0 ? dernier_espace : i; break; }
        }
        if (s[fin] == 0) {
            memcpy(ligne, s, fin); ligne[fin] = 0;
            if (largeur_texte(ligne, p) > l && dernier_espace > 0) fin = dernier_espace;
        }
        memcpy(ligne, s, fin); ligne[fin] = 0;
        for (char *q_ = ligne; *q_; q_++) if (*q_ == '_') *q_ = ' ';      // '_' : espace insecable
        texte(x, y + n * interligne, ligne, p, c, a);
        n++;
        s += fin;
        while (*s == ' ') s++;
        if (*s == '\n') s++;
    }
    return n;
}

void fleches_pages(int page, int nb, float t0)
{
    float w = nb * 6.0f;
    for (int i = 0; i < nb; i++) {
        float x = 160 - w / 2 + i * 6 + 3;
        bool c = i == page;
        if (c) halo(x, ZY1 - 11, 6, CYAN, 90);
        disque(x, ZY1 - 11, c ? 2.6f : 1.6f, c ? CYAN : TEXTE3);
    }
    (void)t0;
}

int glisse_page()
{
    if (tc.fin && tc.bouge && abs(tc.x - tc.x0) > 50 && abs(tc.x - tc.x0) > 2 * abs(tc.y - tc.y0))
        return tc.x < tc.x0 ? 1 : -1;
    return 0;
}

// ============================ Lecture du circuit ============================
static float vitesse() { static const float V[3] = { 0.55f, 1.1f, 2.4f }; return V[R.vitesse_i < 0 ? 0 : (R.vitesse_i > 2 ? 2 : R.vitesse_i)]; }

static bool colonne_mesure(int col)
{
    for (int k = 0; k < circ.nq; k++) if (circ.c[col][k].p == q::P_MESURE) return true;
    return false;
}

static void etat_final()
{
    q::executer(circ, etat_aff);
    q::probabilites(etat_aff, probas_aff);
}

void circuit_modifie()
{
    circ.nc = q::colonnes_utiles(circ);
    lec.active = false;
    lec.flash = 0;
    etat_final();
    tirs.n = 0; tirs.en_cours = false;
    tirs.masque = masque_mesure(circ);
    memset(tirs.compte, 0, sizeof tirs.compte);
    t_sauver_circuit = t + 1.5f;          // sauvegarde un peu plus tard (pas a chaque toucher)
}

void lecture_jouer()
{
    lec.active = true;
    lec.pause = false;
    lec.col = 0;
    lec.u = 0;
    lec.bits = lec.masque = 0;
    cible_ok = false;
    q::etat_zero(etat_base, circ.nq);
    etat_aff = etat_base;
}

void lecture_stop()
{
    lec.active = false;
    etat_final();
}

static void finir_colonne()
{
    if (lec.col >= circ.nc) return;
    if (colonne_mesure(lec.col)) {
        if (!cible_ok) {
            etat_cible = etat_base;
            q::appliquer_colonne(etat_cible, circ, lec.col, 1);
            int b, m;
            q::mesurer_colonne(etat_cible, circ, lec.col, alea32, b, m);
            lec.bits = (lec.bits & ~m) | b;
            lec.masque |= m;
        }
        etat_base = etat_cible;
    } else {
        q::appliquer_colonne(etat_base, circ, lec.col, 1);
    }
    cible_ok = false;
    lec.col++;
    lec.u = 0;
}

void lecture_pas(int sens)
{
    if (!lec.active) { lecture_jouer(); lec.pause = true; if (sens < 0) return; }
    lec.pause = true;
    if (sens > 0) {
        if (lec.u > 0) finir_colonne();
        else finir_colonne();
    } else {
        int cible = lec.col - (lec.u > 0 ? 0 : 1);
        if (cible < 0) cible = 0;
        q::etat_zero(etat_base, circ.nq);
        lec.col = 0; lec.u = 0; lec.bits = lec.masque = 0;
        while (lec.col < cible) finir_colonne();
    }
}

void lecture_maj()
{
    if (lec.flash > 0) lec.flash -= dt * 1.6f;
    if (!lec.active) return;
    if (lec.col >= circ.nc) {           // fini : on garde l'etat final (mesures comprises)
        etat_aff = etat_base;
        q::probabilites(etat_aff, probas_aff);
        return;
    }
    bool mes = colonne_mesure(lec.col);
    if (!lec.pause) {
        float u0 = lec.u;
        lec.u += dt * vitesse() * (mes ? 0.7f : 1.0f);
        if (mes && u0 < 0.5f && lec.u >= 0.5f) {
            lec.flash = 1;
            // eclat sur les fils mesures
            for (int k = 0; k < circ.nq; k++)
                if (circ.c[lec.col][k].p == q::P_MESURE) {
                    bip_mesure((lec.bits >> k) & 1);
                }
        }
        if (lec.u >= 1.15f) finir_colonne();    // petite pause entre deux colonnes
    }
    if (lec.col >= circ.nc) { etat_aff = etat_base; q::probabilites(etat_aff, probas_aff); return; }
    float u = sat01(lec.u);
    if (colonne_mesure(lec.col)) {
        if (!cible_ok) {
            etat_cible = etat_base;
            q::appliquer_colonne(etat_cible, circ, lec.col, 1);
            int b, m;
            q::mesurer_colonne(etat_cible, circ, lec.col, alea32, b, m);
            lec.bits = (lec.bits & ~m) | b;
            lec.masque |= m;
            cible_ok = true;
        }
        float k = doux((u - 0.3f) / 0.4f);
        float s = 0;
        for (int i = 0; i < (1 << circ.nq); i++) {
            etat_aff.a[i] = (1 - k) * etat_base.a[i] + k * etat_cible.a[i];
            s += q::norme2(etat_aff.a[i]);
        }
        etat_aff.nq = circ.nq;
        float f = s > 1e-9f ? 1 / sqrtf(s) : 0;
        for (int i = 0; i < (1 << circ.nq); i++) etat_aff.a[i] = f * etat_aff.a[i];
    } else {
        etat_aff = etat_base;
        q::appliquer_colonne(etat_aff, circ, lec.col, doux(u));
    }
    q::probabilites(etat_aff, probas_aff);
}

// ============================ Tirs ==========================================
int masque_mesure(const q::Circuit &c)
{
    int m = 0;
    for (int col = 0; col < c.nc; col++)
        for (int k = 0; k < c.nq; k++)
            if (c.c[col][k].p == q::P_MESURE) m |= 1 << k;
    return m ? m : (1 << c.nq) - 1;
}

int compacter(int x, int masque)
{
    int r = 0, j = 0;
    for (int k = 0; k < q::NQ_MAX; k++)
        if (masque & (1 << k)) { r |= ((x >> k) & 1) << j; j++; }
    return r;
}

static float cumul[q::NE_MAX];
static bool tirs_rapides = false;

void tirs_lancer(int n)
{
    if (R.hasard == 1) graine = 12345;
    tirs.n = 0;
    tirs.cible = n;
    tirs.en_cours = true;
    tirs.masque = masque_mesure(circ);
    memset(tirs.compte, 0, sizeof tirs.compte);
    tirs_rapides = bruit_porte() == 0 && !q::a_mesures_intermediaires(circ);
    if (tirs_rapides) {
        q::Etat e;
        q::executer(circ, e);
        float s = 0;
        for (int i = 0; i < (1 << circ.nq); i++) { s += q::norme2(e.a[i]); cumul[i] = s; }
    }
}

void tirs_maj()
{
    if (!tirs.en_cours) return;
    // environ 1,5 s pour remplir l'histogramme, quel que soit le nombre de tirs
    int lot = tirs.cible / 45 + 1;
    if (tirs.cible <= 100) lot = 3;
    int fin = tirs.n + lot;
    if (fin > tirs.cible) fin = tirs.cible;
    float bl = bruit_lecture();
    for (; tirs.n < fin; tirs.n++) {
        int r;
        if (tirs_rapides) {
            float u = (alea32() >> 8) * (1.0f / 16777216.0f);
            int n = 1 << circ.nq;
            r = n - 1;
            for (int i = 0; i < n; i++) if (u < cumul[i]) { r = i; break; }
            if (bl > 0)
                for (int k = 0; k < circ.nq; k++)
                    if ((alea32() >> 8) * (1.0f / 16777216.0f) < bl) r ^= 1 << k;
        } else {
            r = q::tir(circ, alea32, bruit_porte(), bl);
        }
        int c = compacter(r, tirs.masque);
        tirs.compte[c]++;
        tirs.dernier = c;
    }
    tirs.t_dernier = t;
    bip_mesure(tirs.dernier & 1);
    if (tirs.n >= tirs.cible) {
        tirs.en_cours = false;
        // enregistrement sur la carte SD
        char ligne[400];
        int nb = 0;
        for (int k = 0; k < q::NQ_MAX; k++) if (tirs.masque & (1 << k)) nb++;
        int p = snprintf(ligne, sizeof ligne, "%s,%d,%d,%s", circ.nom, tirs.cible, R.bruit_i,
                         R.hasard ? "graine" : "materiel");
        for (int i = 0; i < (1 << nb) && p < (int)sizeof ligne - 16; i++) {
            char b[8];
            bits(b, sizeof b, i, nb);
            p += snprintf(ligne + p, sizeof ligne - p, ",%s:%d", b, tirs.compte[i]);
        }
        plat_csv(ligne);
    }
}

// ============================ Fond ==========================================
static void dessiner_fond_complet()
{
    degrade_v(0, 0, L, H, FOND_HAUT, FOND_BAS);
    halo(270, 50, 150, VIOLET, 30);
    halo(40, 190, 140, BLEU, 22);
    halo(170, 250, 120, MAGENTA, 16);
}

static void fond()
{
    if (s_fond) copier(s_fond);
    else dessiner_fond_complet();
    for (int i = 0; i < 56; i++) {
        Etoile &e = etoiles[i];
        e.x -= e.v * dt;
        if (e.x < 0) { e.x += L; e.y = alea() * H; }
        float a = e.lum * (0.55f + 0.45f * sinf(t * 1.7f + e.ph));
        pixel_ajout((int)e.x, (int)e.y, TEXTE, (uint8_t)a);
    }
}

// ============================ Barres ========================================
static void logo(float cx, float cy, float r, float a_)
{
    // un atome stylise : trois orbites et un noyau
    uint8_t a = (uint8_t)(255 * a_);
    for (int i = 0; i < 3; i++) {
        float ang = i * PI_F / 3 + t * 0.4f;
        float ca = cosf(ang), sa = sinf(ang);
        float px = 0, py = 0;
        for (int j = 0; j <= 24; j++) {
            float u = j * 2 * PI_F / 24;
            float ex = r * cosf(u), ey = r * 0.38f * sinf(u);
            float x = cx + ex * ca - ey * sa, y = cy + ex * sa + ey * ca;
            if (j) trait(px, py, x, y, r > 20 ? 1.6f : 1.0f, i == 0 ? CYAN : (i == 1 ? MAGENTA : VIOLET), a);
            px = x; py = y;
        }
        float u = t * (2.2f + i * 0.6f) + i * 2;
        float ex = r * cosf(u), ey = r * 0.38f * sinf(u);
        float x = cx + ex * ca - ey * sa, y = cy + ex * sa + ey * ca;
        halo(x, y, r * 0.35f, i == 0 ? CYAN : (i == 1 ? MAGENTA : VIOLET), (uint8_t)(a * 0.6f));
        disque(x, y, r > 20 ? 3.2f : 1.6f, TEXTE, a);
    }
    halo(cx, cy, r * 0.5f, JAUNE, (uint8_t)(a * 0.5f));
    disque(cx, cy, r > 20 ? 5 : 2.2f, JAUNE, a);
}

static void barre_haut()
{
    rect(0, 0, L, 24, PANNEAU, 190);
    degrade_h(0, 24, L, 1, CYAN, MAGENTA, 150);
    logo(13, 12, 9, 1);
    texte(28, 4, "QUBITS", POLICE_13, TEXTE);
    char nom[40];
    snprintf(nom, sizeof nom, "· %s", circ.nom);
    texte(28 + largeur_texte("QUBITS", POLICE_13) + 5, 6, nom, POLICE_11, TEXTE2);

    // pastille d'etat
    char buf[48];
    uint16_t c;
    bool clignote = false;
    if (tirs.en_cours) { snprintf(buf, sizeof buf, "Tirs %d / %d", tirs.n, tirs.cible); c = JAUNE; clignote = true; }
    else if (lec.active && lec.col < circ.nc) {
        if (lec.flash > 0.2f) { snprintf(buf, sizeof buf, "Effondrement !"); c = ROUGE; clignote = true; }
        else { snprintf(buf, sizeof buf, "Pas %d / %d", lec.col + 1, circ.nc); c = lec.pause ? JAUNE : VERT; }
    } else {
        int np = 0;
        for (int col = 0; col < circ.nc; col++)
            for (int k = 0; k < circ.nq; k++) if (circ.c[col][k].p != q::P_VIDE) np++;
        snprintf(buf, sizeof buf, "%d qubit%s · %d porte%s", circ.nq, circ.nq > 1 ? "s" : "", np, np > 1 ? "s" : "");
        c = CYAN;
    }
    int l = largeur_texte(buf, POLICE_11) + 26;
    int x = L - 6 - l;
    rect_arrondi(x, 4, l, 16, 8, PANNEAU2, 230);
    contour_arrondi(x, 4, l, 16, 8, 1, c, 120);
    float a = clignote ? 0.35f + 0.65f * pulse(1.4f) : 0.8f + 0.2f * pulse(0.8f);
    halo(x + 10, 12, 7, c, (uint8_t)(90 * a));
    disque(x + 10, 12, 3, c, (uint8_t)(255 * a));
    texte(x + 18, 6, buf, POLICE_11, TEXTE);
}

static void icone_onglet(int i, float cx, float cy, uint16_t c)
{
    switch (i) {
    case O_CIRCUIT:
        for (int k = 0; k < 3; k++) trait(cx - 10, cy - 5 + k * 5, cx + 10, cy - 5 + k * 5, 1, c, 150);
        rect_arrondi((int)cx - 7, (int)cy - 8, 7, 7, 2, c);
        disque(cx + 5, cy - 0.0f, 2, c);
        trait(cx + 5, cy, cx + 5, cy + 5, 1.2f, c);
        anneau(cx + 5, cy + 5, 2.6f, 1.1f, c);
        break;
    case O_SPHERE: {
        anneau(cx, cy, 7, 1.2f, c);
        float px = 0, py = 0;
        for (int j = 0; j <= 16; j++) {
            float u = j * 2 * PI_F / 16;
            float x = cx + 7 * cosf(u), y = cy + 2.4f * sinf(u);
            if (j) trait(px, py, x, y, 0.9f, c, 140);
            px = x; py = y;
        }
        float a = t * 1.5f;
        trait(cx, cy, cx + 6 * cosf(a), cy - 4 - 2 * sinf(a), 1.6f, c);
        disque(cx + 6 * cosf(a), cy - 4 - 2 * sinf(a), 1.8f, c);
        break;
    }
    case O_MESURES:
        for (int j = 0; j < 4; j++) {
            float hh = j == 0 || j == 3 ? 3 + 9 * (0.6f + 0.4f * sinf(t * 2 + j)) : 3;
            rect((int)(cx - 9 + j * 5), (int)(cy + 6 - hh), 4, (int)hh, c);
        }
        break;
    case O_ALGOS:
        // une loupe (Grover : chercher)
        anneau(cx - 2, cy - 2, 5, 1.6f, c);
        trait(cx + 2, cy + 2, cx + 7, cy + 7, 2.2f, c);
        disque(cx - 2, cy - 2, 1.5f + pulse(1.5f), c, 200);
        break;
    case O_TUTO:
        texte((int)cx, (int)cy - 8, "ψ", POLICE_16, c, 255, CENTRE);
        break;
    default:
        for (int j = 0; j < 8; j++) {
            float an_ = j * PI_F / 4 + t * 0.3f;
            trait(cx + cosf(an_) * 5, cy + sinf(an_) * 5, cx + cosf(an_) * 7.5f, cy + sinf(an_) * 7.5f, 2.2f, c);
        }
        anneau(cx, cy, 4.5f, 2.4f, c);
        break;
    }
}

static void barre_onglets()
{
    const float W = (float)L / NB_ONGLETS;
    rect(0, 204, L, 36, PANNEAU, 215);
    degrade_h(0, 204, L, 1, MAGENTA, CYAN, 120);
    onglet_aff = approche(onglet_aff, (float)onglet, 14);
    int hx = (int)(3 + onglet_aff * W);
    rect_arrondi_degrade(hx, 207, (int)W - 6, 31, 10, rgb(30, 70, 170), rgb(110, 40, 150), 200);
    contour_arrondi(hx, 207, (int)W - 6, 31, 10, 1, CYAN, 150);
    halo(hx + W / 2 - 3, 239, 24, CYAN, 70);
    for (int i = 0; i < NB_ONGLETS; i++) {
        bool actif = i == onglet;
        float cx = i * W + W / 2;
        icone_onglet(i, cx, 216, actif ? CYAN : TEXTE3);
        texte((int)cx, 224, ONGLETS[i], POLICE_11, actif ? TEXTE : TEXTE3, 255, CENTRE);
    }
}

static void dessiner_toast()
{
    float age = t - toast_t0;
    if (age < 0 || age > 2.8f || !toast_txt[0]) return;
    float e = age < 0.3f ? sortie(age / 0.3f) : (age > 2.4f ? 1 - (age - 2.4f) / 0.4f : 1);
    int l = largeur_texte(toast_txt, POLICE_13) + 34;
    if (l > L - 8) l = L - 8;
    int x = (L - l) / 2;
    int y = (int)(-20 + 50 * e);
    uint8_t a = (uint8_t)(255 * sat01(e));
    rect_arrondi(x, y, l, 24, 12, PANNEAU2, (uint8_t)(a * 0.95f));
    contour_arrondi(x, y, l, 24, 12, 1.2f, toast_c, a);
    halo(x + 14, y + 12, 10, toast_c, (uint8_t)(a * 0.5f));
    disque(x + 14, y + 12, 4, toast_c, a);
    texte(x + 24, y + 5, toast_txt, POLICE_13, TEXTE, a);
}

// ============================ Demarrage =====================================
static void ecran_demarrage()
{
    float u = t - splash_t0;
    fond();
    // une sphere de Bloch dont le vecteur fait la ronde, entouree de l'atome
    float th = 1.1f + 0.5f * sinf(u * 0.9f), ph = u * 1.6f;
    logo(160, 82, 70, 0.55f * sortie(u / 0.6f));
    sphere_bloch(160, 82, 44, 0.5f + u * 0.25f, 0.35f, sinf(th) * cosf(ph), sinf(th) * sinf(ph), cosf(th), CYAN, false);
    float e = sortie((u - 0.3f) / 0.8f);
    uint8_t a = (uint8_t)(255 * e);
    texte(160, 140 - (int)(10 * (1 - e)), "ORDINATEUR QUANTIQUE", POLICE_22, TEXTE, a, CENTRE);
    texte(160, 168, "5 qubits dans la main · ESP32-S3", POLICE_13, TEXTE2, a, CENTRE);
    char b[80];
    snprintf(b, sizeof b, "ES3C28P · %s · hasard : %s", OQ_VERSION, plat_alea_source());
    texte(160, 214, b, POLICE_11, TEXTE3, a, CENTRE);
    halo(160, 152, 130, MAGENTA, (uint8_t)(28 * e));
    // superposition : "0" et "1" qui clignotent ensemble
    float k = pulse(0.8f);
    texte(116, 186, "|0⟩", POLICE_16, CYAN, (uint8_t)(a * (0.4f + 0.6f * k)), CENTRE);
    texte(160, 186, "+", POLICE_16, TEXTE2, a, CENTRE);
    texte(204, 186, "|1⟩", POLICE_16, MAGENTA, (uint8_t)(a * (1 - 0.6f * k)), CENTRE);
    if (u > 3.4f || (tc.tap && u > 0.4f)) { splash = false; tc.tap = false; }
}

// ============================ Reglages ======================================
static float lum_actif = 0;

static void ligne_reg(const char *titre, const char *sous, int y)
{
    texte(12, y + 2, titre, POLICE_13, TEXTE);
    if (sous) texte(12, y + 18, sous, POLICE_11, TEXTE3);
}

void ecran_reglages()
{
    static const char *PAGES[2] = { "Calcul", "Appareil" };
    if (R.page_reg < 0 || R.page_reg > 1) R.page_reg = 0;
    segments(0, 60, ZY0 + 3, 200, 20, PAGES, 2, R.page_reg);
    if (tap_dans(60, ZY0 + 3, 200, 20)) { R.page_reg = segment_touche(60, 200, 2); sauver_reglages(); }
    panneau(4, ZY0 + 28, 312, ZY1 - ZY0 - 30);
    int y = ZY0 + 34;
    const int SX = 150, SL = 160;
    if (R.page_reg == 0) {
        static const char *TIRS[3] = { "100", "1 000", "8 192" };
        ligne_reg("Nombre de tirs", "répétitions du circuit", y);
        segments(1, SX, y + 4, SL, 22, TIRS, 3, R.tirs_i);
        if (tap_dans(SX, y, SL, 30)) { R.tirs_i = segment_touche(SX, SL, 3); sauver_reglages(); }
        y += 36;
        static const char *BR[3] = { "Parfait", "Faible", "Vrai QPU" };
        ligne_reg("Bruit", "erreurs des portes et lecture", y);
        segments(2, SX, y + 4, SL, 22, BR, 3, R.bruit_i);
        if (tap_dans(SX, y, SL, 30)) {
            R.bruit_i = segment_touche(SX, SL, 3);
            sauver_reglages();
            if (R.bruit_i == 2) toast("Environ 2 % d'erreur par porte, comme un vrai QPU", ORANGE);
        }
        y += 36;
        static const char *VIT[3] = { "Lente", "Normale", "Rapide" };
        ligne_reg("Animation", "lecture pas à pas", y);
        segments(3, SX, y + 4, SL, 22, VIT, 3, R.vitesse_i);
        if (tap_dans(SX, y, SL, 30)) { R.vitesse_i = segment_touche(SX, SL, 3); sauver_reglages(); }
        y += 36;
        static const char *HAS[2] = { "Matériel", "Graine fixe" };
        ligne_reg("Hasard", "bruit radio ou reproductible", y);
        segments(4, SX, y + 4, SL, 22, HAS, 2, R.hasard);
        if (tap_dans(SX, y, SL, 30)) {
            R.hasard = segment_touche(SX, SL, 2);
            sauver_reglages();
            toast(R.hasard ? "Mêmes tirs à chaque fois (graine 12345)" : "Hasard physique de la puce", R.hasard ? VIOLET : VERT);
        }
    } else {
        static const char *SON[2] = { "Muet", "Clics" };
        ligne_reg("Son des mesures", plat_son_etat(), y);
        segments(5, SX, y + 4, SL, 22, SON, 2, R.son);
        if (tap_dans(SX, y, SL, 30)) { R.son = segment_touche(SX, SL, 2); sauver_reglages(); bip_mesure(1); }
        y += 36;
        static const char *VOL[3] = { "Doux", "Moyen", "Fort" };
        ligne_reg("Volume", nullptr, y);
        segments(6, SX, y + 4, SL, 22, VOL, 3, R.volume);
        if (tap_dans(SX, y, SL, 30)) { R.volume = segment_touche(SX, SL, 3); sauver_reglages(); bip_mesure(1); }
        y += 36;
        ligne_reg("Luminosité", nullptr, y);
        char b[16];
        snprintf(b, sizeof b, "%d %%", (int)R.lum);
        texte(12, y + 18, b, POLICE_11, TEXTE3);
        bool actif = tc.appui && dans(tc.x0, tc.y0, SX - 10, y, SL + 20, 30);
        if (actif) {
            int v = 5 + (tc.x - SX) * 95 / SL;
            R.lum = v < 5 ? 5 : (v > 100 ? 100 : v);
            plat_luminosite(R.lum);
        }
        if (tc.fin && dans(tc.x0, tc.y0, SX - 10, y, SL + 20, 30)) sauver_reglages();
        lum_actif = approche(lum_actif, actif ? 1.0f : 0.0f, 10);
        curseur(SX, SX + SL, y + 15, (R.lum - 5) / 95.0f, lum_actif > 0.5f, JAUNE, ORANGE);
        y += 38;
        char l1[96], l2[96];
        snprintf(l1, sizeof l1, "Hasard : %s · SD : %s", plat_alea_source(), plat_sd() ? "oui" : "non");
        snprintf(l2, sizeof l2, "%s · %lu Ko libres · %d algorithmes", OQ_VERSION,
                 (unsigned long)(plat_memoire_libre() / 1024), q::NB_ALGOS);
        texte(12, y, l1, POLICE_11, TEXTE2);
        texte(12, y + 15, l2, POLICE_11, TEXTE3);
    }
}

// ============================ API ===========================================
void init(uint16_t *f)
{
    s_fond = f;
    for (int i = 0; i < 56; i++)
        etoiles[i] = { alea() * L, alea() * H, 2 + alea() * 7, alea() * 6.28f, (uint8_t)(50 + alea() * 150) };
    Reglages lu;
    if (plat_nvs_lire("reglages", &lu, sizeof lu) && lu.magie == R.magie) R = lu;
    if (R.lum < 5 || R.lum > 100) R.lum = 80;
    q::Circuit c;
    if (plat_nvs_lire("circuit", &c, sizeof c) && c.nq >= 1 && c.nq <= q::NQ_MAX) circ = c;
    else q::charger_algo(circ, 2);                 // l'etat de Bell pour commencer
    if (s_fond) {
        uint16_t *ancien = gfx::fb;
        gfx::fb = s_fond;
        dessiner_fond_complet();
        gfx::fb = ancien;
    }
    plat_luminosite(R.lum);
    circuit_modifie();
    t_sauver_circuit = -1;
}

void toucher(bool appui, int x, int y)
{
    tc.debut = appui && !p_appui;
    tc.fin = !appui && p_appui;
    tc.dx = appui && p_appui ? x - p_x : 0;
    tc.dy = appui && p_appui ? y - p_y : 0;
    if (tc.debut) { tc.x0 = x; tc.y0 = y; tc.duree = 0; tc.bouge = false; }
    if (appui) {
        tc.x = x; tc.y = y;
        if (abs(x - tc.x0) > 8 || abs(y - tc.y0) > 8) tc.bouge = true;
    }
    tc.tap = tc.fin && !tc.bouge;
    tc.double_tap = false;
    if (tc.tap) {
        if (t - dernier_tap_t < 0.4f && abs(tc.x0 - dernier_tap_x) < 20 && abs(tc.y0 - dernier_tap_y) < 20) {
            tc.double_tap = true;
            dernier_tap_t = -10;
        } else {
            dernier_tap_t = t;
            dernier_tap_x = tc.x0;
            dernier_tap_y = tc.y0;
        }
    }
    tc.appui = appui;
    p_appui = appui; p_x = x; p_y = y;
}

void aller_onglet(int o)
{
    if (o < 0 || o >= NB_ONGLETS) return;
    if (o == O_ALGOS && onglet != O_ALGOS) algos_aller(-1);
    onglet = o;
}

void aller(int o, int page)
{
    aller_onglet(o);
    if (page >= 0 && o == O_ALGOS) algos_aller(page);
    if (page >= 0 && o == O_TUTO) tuto_aller(page);
}

void passer_demarrage() { splash = false; }

void image(uint32_t ms)
{
    if (ms_prec) dt = (ms - ms_prec) / 1000.0f;
    if (dt <= 0 || dt > 0.2f) dt = 0.033f;
    ms_prec = ms;
    t += dt;
    if (tc.appui) tc.duree += dt;

    lecture_maj();
    tirs_maj();
    if (t_sauver_circuit > 0 && t > t_sauver_circuit) {
        plat_nvs_ecrire("circuit", &circ, sizeof circ);
        t_sauver_circuit = -1;
    }

    if (splash) { ecran_demarrage(); return; }
    if (onglet == O_MESURES && qpu_plein_ecran_actif()) {   // QR code en grand, sans les barres
        fond();
        ecran_mesures();
        return;
    }

    if (tc.tap && tc.y0 >= 204) {
        int o = tc.x0 * NB_ONGLETS / L;
        if (o >= NB_ONGLETS) o = NB_ONGLETS - 1;
        aller_onglet(o);
        qpu_ouvert = false;
        tc.tap = false;
    }
    if (tc.y0 < 24 || tc.y0 >= 204) tc.debut = false;    // un doigt pose sur une barre n'agit pas sur l'ecran

    fond();
    decoupe(0, 25, L, 204);
    switch (onglet) {
    case O_CIRCUIT: ecran_circuit(); break;
    case O_SPHERE: ecran_sphere(); break;
    case O_MESURES: ecran_mesures(); break;
    case O_ALGOS: ecran_algos(); break;
    case O_TUTO: ecran_tuto(); break;
    default: ecran_reglages(); break;
    }
    sans_decoupe();
    dessiner_particules();
    barre_haut();
    barre_onglets();
    dessiner_toast();
}

}  // namespace ui
