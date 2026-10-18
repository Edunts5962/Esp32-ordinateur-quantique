// ecran_bloch.cpp - onglet SPHERE : la sphere de Bloch en 3D.
//   Circuit     : chaque qubit du circuit (a la position de lecture). Un qubit
//                 intrique n'a plus d'etat a lui : sa fleche raccourcit (|r| < 1).
//   Bac a sable : un qubit seul ; chaque porte le fait tourner sous nos yeux,
//                 autour de son axe (les portes sont des rotations).
#include "ui_commun.h"
#include <stdio.h>
#include <string.h>

namespace ui {

// ---------------------------------------------------------------- projection
void proj_bloch(float cx, float cy, float r, float lacet, float tangage, float x, float y, float z,
                float &sx, float &sy, float &prof)
{
    float cl = cosf(lacet), sl = sinf(lacet), ct = cosf(tangage), st = sinf(tangage);
    float x1 = x * cl - y * sl, y1 = x * sl + y * cl;
    float zs = z * ct + y1 * st;
    prof = y1 * ct - z * st;
    sx = cx + r * x1;
    sy = cy - r * zs;
}

static void cercle_3d(float cx, float cy, float r, float lac, float tan_, int axe, uint16_t c, float ep)
{
    float px = 0, py = 0, pp = 0;
    for (int i = 0; i <= 48; i++) {
        float u = i * 2 * PI_F / 48, a = cosf(u), b = sinf(u);
        float x = 0, y = 0, z = 0;
        if (axe == 0) { x = a; y = b; }           // equateur
        else if (axe == 1) { x = a; z = b; }      // meridien x-z
        else { y = a; z = b; }                    // meridien y-z
        float sx, sy, pr;
        proj_bloch(cx, cy, r, lac, tan_, x, y, z, sx, sy, pr);
        if (i) trait(px, py, sx, sy, ep, c, (pp + pr) > 0 ? 150 : 50);
        px = sx; py = sy; pp = pr;
    }
}

static void axe(float cx, float cy, float r, float lac, float tan_, float x, float y, float z,
                const char *lab, uint16_t c, bool etiq)
{
    float sx, sy, pr, ex, ey, ep;
    proj_bloch(cx, cy, r, lac, tan_, -x, -y, -z, ex, ey, ep);
    proj_bloch(cx, cy, r, lac, tan_, x, y, z, sx, sy, pr);
    trait_pointille(ex, ey, sx, sy, 1, 3, 3, c, 110);
    if (etiq && lab) {
        float lx, ly, lp;
        proj_bloch(cx, cy, r * 1.22f, lac, tan_, x, y, z, lx, ly, lp);
        texte((int)lx, (int)ly - 7, lab, POLICE_11, c, lp > -0.2f ? 255 : 140, CENTRE);
    }
}

void sphere_bloch(float cx, float cy, float r, float lac, float tan_, float bx, float by, float bz,
                  uint16_t c, bool etiq, const float *trace, int ntrace)
{
    // le globe : un verre sombre qui brille
    halo(cx, cy, r * 1.35f, c, 26);
    disque(cx, cy, r, rgb(14, 14, 44), 200);
    halo(cx - r * 0.35f, cy - r * 0.4f, r * 0.55f, TEXTE, 22);
    anneau(cx, cy, r, 1.4f, melange(BORD, c, 90), 220);
    cercle_3d(cx, cy, r, lac, tan_, 0, BORD, 1);
    cercle_3d(cx, cy, r, lac, tan_, 1, GRILLE, 0.8f);
    cercle_3d(cx, cy, r, lac, tan_, 2, GRILLE, 0.8f);
    axe(cx, cy, r, lac, tan_, 0, 0, 1, "|0⟩", CYAN, etiq);
    axe(cx, cy, r, lac, tan_, 1, 0, 0, "|+⟩", TEXTE2, etiq);
    axe(cx, cy, r, lac, tan_, 0, 1, 0, "|+i⟩", TEXTE2, etiq);
    if (etiq) {
        float sx, sy, pr;
        proj_bloch(cx, cy, r * 1.2f, lac, tan_, 0, 0, -1, sx, sy, pr);
        texte((int)sx, (int)sy - 5, "|1⟩", POLICE_11, MAGENTA, 255, CENTRE);
        proj_bloch(cx, cy, r * 1.22f, lac, tan_, -1, 0, 0, sx, sy, pr);
        texte((int)sx, (int)sy - 7, "|−⟩", POLICE_11, TEXTE3, 255, CENTRE);
    }
    // trace laissee par la pointe
    if (trace && ntrace > 1) {
        float px = 0, py = 0;
        for (int i = 0; i < ntrace; i++) {
            float sx, sy, pr;
            proj_bloch(cx, cy, r, lac, tan_, trace[3 * i], trace[3 * i + 1], trace[3 * i + 2], sx, sy, pr);
            if (i) {
                uint8_t a = (uint8_t)(40 + 180 * i / ntrace);
                trait(px, py, sx, sy, 2.2f, JAUNE, (uint8_t)(a * (pr > 0 ? 1 : 0.5f)));
            }
            px = sx; py = sy;
        }
    }
    // ombre sur l'equateur
    float sx, sy, pr, ox, oy, op;
    proj_bloch(cx, cy, r, lac, tan_, bx, by, 0, ox, oy, op);
    trait_pointille(cx, cy, ox, oy, 1, 2, 3, JAUNE, 120);
    proj_bloch(cx, cy, r, lac, tan_, bx, by, bz, sx, sy, pr);
    trait_pointille(ox, oy, sx, sy, 1, 2, 3, JAUNE, 120);
    // la fleche (le qubit)
    float n = sqrtf(bx * bx + by * by + bz * bz);
    trait(cx, cy, sx, sy, 6, c, 60);
    trait(cx, cy, sx, sy, 2.4f, c);
    halo(sx, sy, 10 + 4 * pulse(1.3f), c, (uint8_t)(120 + 80 * n));
    disque(sx, sy, 4.2f, TEXTE);
    disque(sx, sy, 2.4f, c);
    disque(cx, cy, 2.2f, TEXTE2);
}

// ---------------------------------------------------------------- ecran
static float lacet = 0.6f, tangage = 0.32f;
static int mode = 0;                    // 0 circuit, 1 bac a sable
static float trace[3 * 64];
static int ntrace = 0;
static void ntrace_raz() { ntrace = 0; }
void sphere_mode(int m) { mode = m; ntrace_raz(); }
static float trace_t = 0;
static int trace_q = -1;

// bac a sable
static q::Etat sb, sb_dep;
static int sb_porte = -1;
static float sb_u = 0;
static bool sb_init = false;
static float sb_flash = 0;
static int sb_res = 0;

static void ajouter_trace(float x, float y, float z)
{
    if (t - trace_t < 0.03f) return;
    trace_t = t;
    if (ntrace > 0) {
        float *d = &trace[3 * (ntrace - 1)];
        if (fabsf(d[0] - x) + fabsf(d[1] - y) + fabsf(d[2] - z) < 0.01f) return;
    }
    if (ntrace == 64) { memmove(trace, trace + 3, sizeof(float) * 3 * 63); ntrace--; }
    trace[3 * ntrace] = x; trace[3 * ntrace + 1] = y; trace[3 * ntrace + 2] = z;
    ntrace++;
}

static void infos(int x, int y, float bx, float by, float bz, int k, bool sandbox)
{
    float n = sqrtf(bx * bx + by * by + bz * bz);
    float th = acosf(n > 1e-6f ? bz / n : 1), ph = atan2f(by, bx);
    if (ph < 0) ph += 2 * PI_F;
    char b[64], v[16];
    panneau(x, y, 128, 146, 160);
    snprintf(b, sizeof b, sandbox ? "Un qubit seul" : "Qubit q%d", k);
    texte(x + 8, y + 6, b, POLICE_13, sandbox ? JAUNE : couleur_qubit(k));
    fmt(v, sizeof v, th * 180 / PI_F, 0);
    snprintf(b, sizeof b, "θ = %s°", v);
    texte(x + 8, y + 26, b, POLICE_11, TEXTE);
    fmt(v, sizeof v, n > 0.02f && fabsf(bz / (n + 1e-9f)) < 0.999f ? ph * 180 / PI_F : 0, 0);
    snprintf(b, sizeof b, "φ = %s°", v);
    texte(x + 68, y + 26, b, POLICE_11, TEXTE);
    // probabilites
    float p0 = (1 + bz) / 2;
    texte(x + 8, y + 44, "P(0)", POLICE_11, CYAN);
    rect_arrondi(x + 38, y + 46, 80, 8, 4, PANNEAU2);
    rect_arrondi_degrade(x + 38, y + 46, (int)(80 * p0) + 1, 8, 4, CYAN, BLEU);
    texte(x + 8, y + 60, "P(1)", POLICE_11, MAGENTA);
    rect_arrondi(x + 38, y + 62, 80, 8, 4, PANNEAU2);
    rect_arrondi_degrade(x + 38, y + 62, (int)(80 * (1 - p0)) + 1, 8, 4, MAGENTA, VIOLET);
    // purete
    fmt(v, sizeof v, n, 2);
    snprintf(b, sizeof b, "|r| = %s", v);
    texte(x + 8, y + 78, b, POLICE_11, TEXTE);
    if (n > 0.98f) {
        texte(x + 8, y + 94, "État pur : la flèche", POLICE_11, VERT);
        texte(x + 8, y + 107, "touche la sphère", POLICE_11, VERT);
    } else if (n < 0.05f) {
        texte(x + 8, y + 94, "Totalement intriqué !", POLICE_11, ROSE);
        texte(x + 8, y + 107, "Il n'a plus d'état à lui", POLICE_11, TEXTE2);
    } else {
        texte(x + 8, y + 94, "Partiellement intriqué", POLICE_11, ORANGE);
        texte(x + 8, y + 107, "(ou mesuré)", POLICE_11, TEXTE2);
    }
    if (sandbox) texte(x + 8, y + 126, "Touche une porte ↓", POLICE_11, TEXTE3);
}

static void mode_circuit()
{
    int k = R.qubit_bloch;
    if (k >= circ.nq) k = R.qubit_bloch = 0;
    if (k != trace_q) { ntrace = 0; trace_q = k; }
    // puces des qubits
    for (int i = 0; i < circ.nq; i++) {
        int x = 6 + i * 34, y = ZY0 + 28;
        bool sel = i == k;
        char b[6];
        snprintf(b, sizeof b, "q%d", i);
        if (sel) halo(x + 15.0f, y + 9.0f, 16, couleur_qubit(i), 70);
        rect_arrondi(x, y, 30, 18, 9, sel ? melange(PANNEAU2, couleur_qubit(i), 90) : PANNEAU2);
        texte(x + 15, y + 2, b, POLICE_11, sel ? TEXTE : TEXTE2, 255, CENTRE);
        if (tap_dans(x, y, 30, 18)) { R.qubit_bloch = i; sauver_reglages(); }
    }
    float bx, by, bz;
    q::bloch(etat_aff, k, bx, by, bz);
    if (lec.active) ajouter_trace(bx, by, bz);
    sphere_bloch(92, 130, 42, lacet, tangage, bx, by, bz, couleur_qubit(k), true, trace, ntrace);
    infos(186, ZY0 + 28, bx, by, bz, k, false);
    // lecture depuis cet ecran
    bool joue = lec.active && !lec.pause && lec.col < circ.nc;
    if (petit_bouton(6, ZY1 - 26, 30, 22, joue ? "❚❚" : "▶", VERT, circ.nc > 0)) {
        if (!lec.active || lec.col >= circ.nc) { lecture_jouer(); ntrace = 0; }
        else lec.pause = !lec.pause;
    }
    if (petit_bouton(40, ZY1 - 26, 24, 22, "›", CYAN, circ.nc > 0)) lecture_pas(1);
    char b[48];
    if (lec.active && lec.col < circ.nc) snprintf(b, sizeof b, "colonne %d / %d", lec.col + 1, circ.nc);
    else snprintf(b, sizeof b, "état final du circuit");
    texte(194, ZY0 + 154, b, POLICE_11, lec.active ? JAUNE : TEXTE3);
}

static void sb_appliquer(int p)
{
    if (sb_porte >= 0) {                 // finit la porte en cours
        q::Circuit c;
        q::vider(c, 1);
        c.c[0][0].p = (uint8_t)sb_porte;
        c.c[0][0].angle = PI_F / 4;
        sb = sb_dep;
        q::appliquer_colonne(sb, c, 0, 1);
    }
    sb_dep = sb;
    sb_porte = p;
    sb_u = 0;
}

static void mode_bac()
{
    if (!sb_init) { q::etat_zero(sb, 1); sb_dep = sb; sb_init = true; }
    // animation de la porte en cours
    float axx = 0, axy = 0, axz = 0;
    if (sb_porte >= 0) {
        sb_u += dt * 1.4f;
        q::Circuit c;
        q::vider(c, 1);
        c.c[0][0].p = (uint8_t)sb_porte;
        c.c[0][0].angle = PI_F / 4;
        sb = sb_dep;
        q::appliquer_colonne(sb, c, 0, doux(sb_u));
        switch (sb_porte) {
        case q::P_H: axx = 0.7071f; axz = 0.7071f; break;
        case q::P_X: case q::P_RX: axx = 1; break;
        case q::P_Y: case q::P_RY: axy = 1; break;
        default: axz = 1; break;
        }
        if (sb_u >= 1) sb_porte = -1;
    }
    float bx, by, bz;
    q::bloch(sb, 0, bx, by, bz);
    ajouter_trace(bx, by, bz);
    const float cx = 92, cy = 112, r = 44;
    sphere_bloch(cx, cy, r, lacet, tangage, bx, by, bz, JAUNE, true, trace, ntrace);
    if (axx != 0 || axy != 0 || axz != 0) {         // l'axe de rotation s'allume
        float sx, sy, pr, ex, ey, ep;
        proj_bloch(cx, cy, r * 1.25f, lacet, tangage, axx, axy, axz, sx, sy, pr);
        proj_bloch(cx, cy, r * 1.25f, lacet, tangage, -axx, -axy, -axz, ex, ey, ep);
        float a = sinf(PI_F * sat01(sb_u));
        trait(ex, ey, sx, sy, 5, ROSE, (uint8_t)(70 * a));
        trait(ex, ey, sx, sy, 1.6f, ROSE, (uint8_t)(255 * a));
        halo(sx, sy, 9, ROSE, (uint8_t)(160 * a));
    }
    if (sb_flash > 0) {
        sb_flash -= dt * 1.5f;
        float sx, sy, pr;
        proj_bloch(cx, cy, r, lacet, tangage, 0, 0, sb_res ? -1 : 1, sx, sy, pr);
        halo(sx, sy, 40 * sb_flash + 8, ROUGE, (uint8_t)(200 * sat01(sb_flash)));
        texte((int)cx, (int)cy + (int)r + 8, sb_res ? "Mesuré : 1" : "Mesuré : 0", POLICE_13, JAUNE,
              (uint8_t)(255 * sat01(sb_flash * 2)), CENTRE);
    }
    infos(186, ZY0 + 28, bx, by, bz, 0, true);
    // les portes du bac a sable
    static const int P[10] = { q::P_H, q::P_X, q::P_Y, q::P_Z, q::P_S, q::P_T, q::P_RX, q::P_RY, q::P_RZ, -1 };
    static const char *N[10] = { "H", "X", "Y", "Z", "S", "T", "Rx", "Ry", "Rz", "M" };
    for (int i = 0; i < 10; i++) {
        int x = 6 + i * 31, y = ZY1 - 25;
        uint16_t c = i == 0 ? CYAN : (i < 4 ? MAGENTA : (i < 6 ? JAUNE : (i < 9 ? VIOLET : ORANGE)));
        if (petit_bouton(x, y, 28, 22, N[i], c)) {
            if (P[i] >= 0) sb_appliquer(P[i]);
            else {                                       // mesure : effondrement
                sb_appliquer(-1);
                float p1 = (1 - bz) / 2;
                sb_res = (alea32() >> 8) * (1.0f / 16777216.0f) < p1 ? 1 : 0;
                q::etat_zero(sb, 1);
                if (sb_res) { sb.a[0] = q::C(0); sb.a[1] = q::C(1); }
                sb_dep = sb;
                sb_flash = 1;
                bip_mesure(sb_res);
                eclat(cx, cy - (sb_res ? -r : r), ORANGE, 14);
            }
        }
    }
    if (petit_bouton(6, ZY0 + 28, 48, 18, "↺ |0⟩", CYAN)) { sb_appliquer(-1); q::etat_zero(sb, 1); sb_dep = sb; ntrace = 0; }
}

void ecran_sphere()
{
    static const char *MODES[2] = { "Circuit", "Bac à sable" };
    segments(8, 70, ZY0 + 3, 180, 20, MODES, 2, mode);
    if (tap_dans(70, ZY0 + 3, 180, 20)) { mode = segment_touche(70, 180, 2); ntrace = 0; tc.tap = false; }
    // glisser sur la sphere : tourner la vue
    if (tc.appui && dans(tc.x0, tc.y0, 0, ZY0 + 48, 184, 120)) {
        lacet -= tc.dx * 0.012f;
        tangage += tc.dy * 0.01f;
        if (tangage > 1.2f) tangage = 1.2f;
        if (tangage < -0.6f) tangage = -0.6f;
    } else if (!tc.appui) {
        lacet += dt * 0.12f;                       // rotation lente, pour voir la 3D
    }
    if (mode == 0) mode_circuit();
    else mode_bac();
}

}  // namespace ui
