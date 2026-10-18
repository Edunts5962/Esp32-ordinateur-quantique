// ecran_qpu.cpp - lancer le circuit sur un VRAI ordinateur quantique.
//
// La carte ecrit le circuit en OpenQASM 2.0 (le langage qu'accepte IBM Quantum
// Composer) et l'affiche en QR code : on le scanne avec un telephone, on le colle
// dans l'editeur de code du Composer, et on l'execute sur un processeur
// quantique reel (offre gratuite "Open" d'IBM). Le fichier .qasm est aussi
// ecrit sur la carte SD (Composer : File > Upload .qasm file).
#include "ui_commun.h"
#include "plateforme.h"
#include "qrcodegen.h"
#include <stdio.h>
#include <string.h>

namespace ui {

static const char *URL_COMPOSER = "https://quantum.cloud.ibm.com/composer";
static const int VMAX = 25;
static uint8_t qr_tmp[qrcodegen_BUFFER_LEN_FOR_VERSION(VMAX)];
static uint8_t qr_lien[qrcodegen_BUFFER_LEN_FOR_VERSION(VMAX)];
static uint8_t qr_circ[qrcodegen_BUFFER_LEN_FOR_VERSION(VMAX)];
static bool lien_ok = false, circ_ok = false, exportable = true;
static char qasm[1400];
static uint32_t empreinte = 0;
static int plein_ecran = 0;           // 0 non, 1 lien, 2 circuit
static int etape = 0;
static float t_ecrit = -10;

static uint32_t hacher(const void *p, int n)
{
    const uint8_t *b = (const uint8_t *)p;
    uint32_t h = 2166136261u;
    for (int i = 0; i < n; i++) { h ^= b[i]; h *= 16777619u; }
    return h;
}

static void preparer()
{
    if (!lien_ok)
        lien_ok = qrcodegen_encodeText(URL_COMPOSER, qr_tmp, qr_lien, qrcodegen_Ecc_MEDIUM, 1, VMAX,
                                       qrcodegen_Mask_AUTO, true);
    uint32_t h = hacher(&circ, sizeof circ);
    if (h == empreinte) return;
    empreinte = h;
    exportable = q::vers_qasm(circ, qasm, sizeof qasm);
    circ_ok = qrcodegen_encodeText(qasm, qr_tmp, qr_circ, qrcodegen_Ecc_LOW, 1, VMAX, qrcodegen_Mask_AUTO, true);
}

// dessine un QR code (modules carres de s pixels), avec sa marge blanche
static void dessiner_qr(const uint8_t *qr, int x, int y, int s)
{
    int n = qrcodegen_getSize(qr);
    rect(x, y, (n + 8) * s, (n + 8) * s, TEXTE);
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++)
            if (qrcodegen_getModule(qr, i, j)) rect(x + (i + 4) * s, y + (j + 4) * s, s, s, NOIR_BLEU);
}

static int taille_qr(const uint8_t *qr, int place)
{
    int n = qrcodegen_getSize(qr) + 8;
    int s = place / n;
    return s < 1 ? 1 : s;
}

// QR plein ecran (par-dessus les barres) : appele a la fin de l'image
static void qr_plein_ecran()
{
    const uint8_t *qr = plein_ecran == 1 ? qr_lien : qr_circ;
    rect(0, 0, L, H, NOIR_BLEU, 235);
    int s = taille_qr(qr, H - 6);
    int n = (qrcodegen_getSize(qr) + 8) * s;
    int x = 8, y = (H - n) / 2;
    dessiner_qr(qr, x, y, s);
    int tx = x + n + 10;
    texte(tx, 20, plein_ecran == 1 ? "IBM Quantum" : "Le circuit", POLICE_16, JAUNE);
    texte(tx, 40, plein_ecran == 1 ? "Composer" : "OpenQASM 2.0", POLICE_13, TEXTE);
    paragraphe(tx, 66, L - tx - 6,
               plein_ecran == 1 ? "Scanne avec l'appareil photo du téléphone, puis connecte-toi (compte gratuit)."
                                : "Scanne, copie le texte, puis colle-le dans le Code Editor du Composer.",
               POLICE_11, TEXTE2, 255, 14);
    texte(tx, H - 22, "Touche pour revenir", POLICE_11, TEXTE3);
    if (tc.tap) { plein_ecran = 0; tc.tap = false; }
}

void ecran_qpu(bool &ouvert)
{
    preparer();
    if (plein_ecran) {
        sans_decoupe();
        qr_plein_ecran();
        return;
    }
    // les 4 etapes, a gauche ; le QR de l'etape a droite
    static const char *TITRES[4] = { "1. Un compte gratuit", "2. Ouvrir le Composer", "3. Coller le circuit", "4. Lancer et comparer" };
    static const char *TEXTES[4] = {
        "IBM Quantum Platform, offre « Open » : jusqu'à 10 minutes de vrai processeur quantique par période de 28 jours. Il suffit d'un e-mail.",
        "Scanne le QR : il ouvre IBM Quantum Composer. Ouvre l'éditeur : View › Panels › Code Editor.",
        "Scanne le QR du circuit et colle le texte dans l'éditeur (OpenQASM 2.0). Ou : File › Upload .qasm file depuis la carte SD.",
        "Run circuit › choisis un QPU › 1 000 tirs. Compare avec l'onglet Histogramme, réglé sur « Bruit QPU »." };
    int x0 = 6, y0 = ZY0 + 28;
    panneau(x0 - 2, y0 - 2, 176, ZY1 - y0 - 2, 150);
    for (int i = 0; i < 4; i++) {
        int y = y0 + 2 + i * 18;
        bool sel = i == etape;
        if (sel) {
            rect_arrondi(x0 + 2, y - 1, 168, 17, 6, PANNEAU2);
            halo(x0 + 10.0f, y + 7.0f, 9, JAUNE, 80);
        }
        texte(x0 + 8, y + 1, TITRES[i], POLICE_11, sel ? JAUNE : TEXTE2);
        if (tap_dans(x0, y - 1, 172, 18)) { etape = i; tc.tap = false; }
    }
    paragraphe(x0 + 6, y0 + 78, 162, TEXTES[etape], POLICE_11, TEXTE, 255, 13);

    // QR a droite : le lien (etapes 1-2) ou le circuit (etapes 3-4)
    int qx = 188, qy = ZY0 + 28, place = 108;
    bool lien = etape <= 1;
    const uint8_t *qr = lien ? qr_lien : qr_circ;
    bool ok = lien ? lien_ok : circ_ok;
    if (ok) {
        int s = taille_qr(qr, place);
        int n = (qrcodegen_getSize(qr) + 8) * s;
        halo(qx + n / 2.0f, qy + n / 2.0f, n * 0.7f, lien ? CYAN : MAGENTA, (uint8_t)(40 + 30 * pulse(0.6f)));
        dessiner_qr(qr, qx + (place - n) / 2, qy, s);
        texte(qx + place / 2, qy + n + 3, lien ? "lien · toucher : agrandir" : "circuit · toucher : agrandir",
              POLICE_11, TEXTE2, 255, CENTRE);
        if (tap_dans(qx, qy, place, n)) { plein_ecran = lien ? 1 : 2; tc.tap = false; }
    } else {
        paragraphe(qx, qy + 20, place, "Circuit trop long pour un QR code : utilise le fichier .qasm de la carte SD.",
                   POLICE_11, ORANGE, 255, 13);
    }
    if (!exportable) texte(6, ZY1 - 14, "Attention : une porte à 3 contrôles n'existe pas en OpenQASM 2", POLICE_11, ORANGE);

    // boutons : ecrire le .qasm sur la carte SD, charger un algorithme
    int y = ZY1 - 27;
    if (exportable && petit_bouton(qx - 2, y, 64, 22, "SD .qasm", VERT, plat_sd())) {
        char nom[48];
        int j = 0;
        for (const char *p = circ.nom; *p && j < 30; p++) {
            unsigned char c = (unsigned char)*p;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) nom[j++] = (char)c;
            else if (c == ' ' || c == '-') nom[j++] = '_';
        }
        nom[j] = 0;
        if (!j) strcpy(nom, "circuit");
        strcat(nom, ".qasm");
        if (plat_ecrire_fichier(nom, qasm)) {
            char b[80];
            snprintf(b, sizeof b, "Écrit : /quantique/%s", nom);
            toast(b, VERT);
            t_ecrit = t;
        } else {
            toast("Carte SD absente ou pleine", ORANGE);
        }
    }
    if (petit_bouton(qx + 66, y, 62, 22, "Algos", CYAN)) { ouvert = false; aller_onglet(O_ALGOS); }
    (void)t_ecrit;
}

// appele par interface.cpp apres les barres, pour le QR plein ecran
bool qpu_plein_ecran_actif() { return plein_ecran != 0; }

}  // namespace ui
