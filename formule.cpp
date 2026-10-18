// formule.cpp - un tout petit moteur de formules mathematiques pour l'ecran.
//
//   texte ordinaire (UTF-8 : ψ α β θ φ ⟨ ⟩ ⊗ ħ ...)
//   x^{2}            exposant            x_{k}          indice
//   \f{num}{den}     fraction            \r{x}          racine carree
//   \m{a,b;c,d}      matrice (',' entre colonnes, ';' entre lignes)
//   \c3{...}         en couleur (1 cyan, 2 magenta, 3 jaune, 4 vert, 5 orange, 6 violet, 7 gris)
//
// Mise en page en deux temps : on mesure les boites (largeur, hauteur au-dessus
// et au-dessous de la ligne de base), puis on dessine.
#include "ui_commun.h"
#include <string.h>

namespace ui {

struct Boite { float w, asc, desc; };

static const PoliceAA *police(int taille, int niv)
{
    static const PoliceAA *T[3][3] = { { &POLICE_13, &POLICE_11, &POLICE_11 },
                                       { &POLICE_16, &POLICE_11, &POLICE_11 },
                                       { &POLICE_22, &POLICE_13, &POLICE_11 } };
    if (taille < 0) taille = 0;
    if (taille > 2) taille = 2;
    if (niv > 2) niv = 2;
    return T[taille][niv];
}

static uint16_t couleur_code(char c, uint16_t def)
{
    switch (c) {
    case '1': return CYAN;
    case '2': return MAGENTA;
    case '3': return JAUNE;
    case '4': return VERT;
    case '5': return ORANGE;
    case '6': return VIOLET;
    case '7': return TEXTE2;
    default: return def;
    }
}

// s pointe sur '{' : renvoie le pointeur juste apres l'accolade fermante
static const char *apres_groupe(const char *s, const char *fin)
{
    if (s >= fin || *s != '{') return s;
    int prof = 0;
    for (const char *p = s; p < fin; p++) {
        if (*p == '{') prof++;
        else if (*p == '}') { prof--; if (prof == 0) return p + 1; }
    }
    return fin;
}

struct Ctx { int taille; uint16_t c; uint8_t a; };

static Boite rendre(const char *s, const char *fin, int niv, float x, float yb, bool dessin, const Ctx &cx);

static Boite texte_brut(const char *s, int n, int niv, float x, float yb, bool dessin, const Ctx &cx)
{
    char buf[96];
    if (n > 95) n = 95;
    memcpy(buf, s, n);
    buf[n] = 0;
    const PoliceAA &p = *police(cx.taille, niv);
    Boite b = { (float)largeur_texte(buf, p), (float)p.ascendante * 0.78f, (float)p.descendante * 0.6f };
    if (dessin) texte((int)lroundf(x), (int)lroundf(yb) - p.ascendante, buf, p, cx.c, cx.a);
    return b;
}

static float axe_math(int taille, int niv) { return police(taille, niv)->ascendante * 0.30f; }

static Boite rendre(const char *s, const char *fin, int niv, float x, float yb, bool dessin, const Ctx &cx)
{
    Boite tot = { 0, (float)police(cx.taille, niv)->ascendante * 0.7f, 0 };
    const char *p = s;
    while (p < fin) {
        Boite b = { 0, 0, 0 };
        float bx = x + tot.w;
        if (*p == '^' || *p == '_') {
            bool haut = *p == '^';
            const char *g = p + 1, *g2 = apres_groupe(g, fin);
            const char *c0 = g + 1, *c1 = g2 - 1;
            if (*g != '{') { c0 = g; c1 = g + 1; g2 = g + 1; }
            Boite c = rendre(c0, c1, niv + 1, 0, 0, false, cx);
            float asc0 = police(cx.taille, niv)->ascendante;
            float dy = haut ? -asc0 * 0.42f : asc0 * 0.28f;
            if (dessin) rendre(c0, c1, niv + 1, bx + 0.5f, yb + dy, true, cx);
            b.w = c.w + 1;
            b.asc = haut ? c.asc - dy : 0;
            b.desc = haut ? 0 : c.desc + dy;
            p = g2;
        } else if (*p == '\\' && p + 1 < fin) {
            char cmd = p[1];
            const char *g = p + 2;
            if (cmd == 'c') { g++; }                    // \cN{...}
            const char *g2 = apres_groupe(g, fin);
            const char *c0 = g + 1, *c1 = g2 - 1;
            if (cmd == 'f') {
                const char *h = g2, *h2 = apres_groupe(h, fin);
                Boite num = rendre(c0, c1, niv, 0, 0, false, cx);
                Boite den = rendre(h + 1, h2 - 1, niv, 0, 0, false, cx);
                float w = (num.w > den.w ? num.w : den.w) + 6;
                float ax = axe_math(cx.taille, niv);
                float by = yb - ax;
                if (dessin) {
                    rendre(c0, c1, niv, bx + (w - num.w) / 2, by - 2.5f - num.desc, true, cx);
                    rendre(h + 1, h2 - 1, niv, bx + (w - den.w) / 2, by + 3 + den.asc, true, cx);
                    trait(bx + 1, by, bx + w - 1, by, 1.2f, cx.c, cx.a);
                }
                b.w = w + 2;
                b.asc = ax + 2.5f + num.asc + num.desc;
                b.desc = den.asc + den.desc + 3 - ax;
                p = h2;
            } else if (cmd == 'r') {
                Boite c = rendre(c0, c1, niv, 0, 0, false, cx);
                if (dessin) {
                    float top = yb - c.asc - 2.5f;
                    trait(bx, yb - c.asc * 0.35f, bx + 2.5f, yb - c.asc * 0.45f, 1.2f, cx.c, cx.a);
                    trait(bx + 2.5f, yb - c.asc * 0.45f, bx + 5, yb + c.desc, 1.3f, cx.c, cx.a);
                    trait(bx + 5, yb + c.desc, bx + 8.5f, top, 1.2f, cx.c, cx.a);
                    trait(bx + 8.5f, top, bx + 10 + c.w, top, 1.2f, cx.c, cx.a);
                    rendre(c0, c1, niv, bx + 9.5f, yb, true, cx);
                }
                b.w = c.w + 12;
                b.asc = c.asc + 3.5f;
                b.desc = c.desc;
                p = g2;
            } else if (cmd == 'm') {
                // decoupe en cellules
                const char *deb[16], *fins[16];
                int lig[16], col_[16], n = 0, l = 0, k = 0, prof = 0;
                const char *d = c0;
                for (const char *q_ = c0; q_ <= c1 && n < 16; q_++) {
                    if (q_ < c1 && *q_ == '{') prof++;
                    if (q_ < c1 && *q_ == '}') prof--;
                    if (q_ == c1 || (prof == 0 && (*q_ == ',' || *q_ == ';') && !(q_ > c0 && q_[-1] == '\\'))) {
                        deb[n] = d; fins[n] = q_; lig[n] = l; col_[n] = k; n++;
                        if (q_ < c1 && *q_ == ';') { l++; k = 0; } else k++;
                        d = q_ + 1;
                    }
                }
                int nl = l + 1, nc = 0;
                for (int i = 0; i < n; i++) if (col_[i] + 1 > nc) nc = col_[i] + 1;
                float cw[4] = { 0, 0, 0, 0 }, la[4] = { 0, 0, 0, 0 }, ld[4] = { 0, 0, 0, 0 };
                Boite bc[16];
                for (int i = 0; i < n; i++) {
                    bc[i] = rendre(deb[i], fins[i], niv, 0, 0, false, cx);
                    int cc = col_[i] < 4 ? col_[i] : 3, ll = lig[i] < 4 ? lig[i] : 3;
                    if (bc[i].w > cw[cc]) cw[cc] = bc[i].w;
                    if (bc[i].asc > la[ll]) la[ll] = bc[i].asc;
                    if (bc[i].desc > ld[ll]) ld[ll] = bc[i].desc;
                }
                float ht = 0;
                for (int i = 0; i < nl && i < 4; i++) ht += la[i] + ld[i] + (i ? 4 : 0);
                float wt = 8;
                for (int i = 0; i < nc && i < 4; i++) wt += cw[i] + (i ? 9 : 0);
                float ax = axe_math(cx.taille, niv);
                float top = yb - ax - ht / 2;
                if (dessin) {
                    float yy = top;
                    for (int r = 0; r < nl && r < 4; r++) {
                        float base = yy + la[r];
                        float xx = bx + 5;
                        for (int c = 0; c < nc && c < 4; c++) {
                            for (int i = 0; i < n; i++)
                                if (lig[i] == r && col_[i] == c)
                                    rendre(deb[i], fins[i], niv, xx + (cw[c] - bc[i].w) / 2, base, true, cx);
                            xx += cw[c] + 9;
                        }
                        yy += la[r] + ld[r] + 4;
                    }
                    float y0 = top - 2, y1 = top + ht + 2;
                    trait(bx + 1.5f, y0, bx + 1.5f, y1, 1.3f, cx.c, cx.a);
                    trait(bx + 1.5f, y0, bx + 4.5f, y0, 1.3f, cx.c, cx.a);
                    trait(bx + 1.5f, y1, bx + 4.5f, y1, 1.3f, cx.c, cx.a);
                    float xr = bx + wt + 1;
                    trait(xr, y0, xr, y1, 1.3f, cx.c, cx.a);
                    trait(xr - 3, y0, xr, y0, 1.3f, cx.c, cx.a);
                    trait(xr - 3, y1, xr, y1, 1.3f, cx.c, cx.a);
                }
                b.w = wt + 5;
                b.asc = yb - (top - 2);
                b.desc = (top + ht + 2) - yb;
                p = g2;
            } else if (cmd == 'c') {
                Ctx c2 = cx;
                c2.c = couleur_code(p[2], cx.c);
                b = rendre(c0, c1, niv, bx, yb, dessin, c2);
                p = g2;
            } else {
                b = texte_brut(p + 1, 1, niv, bx, yb, dessin, cx);
                p += 2;
            }
        } else {
            const char *q_ = p;
            while (q_ < fin && *q_ != '^' && *q_ != '_' && *q_ != '\\') q_++;
            // l'exposant s'accroche au dernier caractere : on coupe juste avant
            b = texte_brut(p, (int)(q_ - p), niv, bx, yb, dessin, cx);
            p = q_;
        }
        tot.w += b.w;
        if (b.asc > tot.asc) tot.asc = b.asc;
        if (b.desc > tot.desc) tot.desc = b.desc;
    }
    return tot;
}

void mesure_formule(const char *s, int taille, float &w, float &asc, float &desc)
{
    Ctx cx = { taille, TEXTE, 255 };
    Boite b = rendre(s, s + strlen(s), 0, 0, 0, false, cx);
    w = b.w; asc = b.asc; desc = b.desc;
}

int largeur_formule(const char *s, int taille)
{
    Ctx cx = { taille, TEXTE, 255 };
    return (int)ceilf(rendre(s, s + strlen(s), 0, 0, 0, false, cx).w);
}

int formule(float x, float y, const char *s, int taille, uint16_t c, uint8_t a, int align)
{
    Ctx cx = { taille, c, a };
    const char *fin = s + strlen(s);
    Boite b = rendre(s, fin, 0, 0, 0, false, cx);
    if (align == CENTRE) x -= b.w / 2;
    else if (align == DROITE) x -= b.w;
    rendre(s, fin, 0, x, y, true, cx);
    return (int)ceilf(b.w);
}

}  // namespace ui
