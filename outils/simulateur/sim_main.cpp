// sim_main.cpp - la console "Ordinateur quantique" sur PC.
//
// Exactement le meme code que la carte ES3C28P (ecrans, simulateur quantique,
// dessin, polices), avec un faux materiel. Rejoue un scenario tactile complet,
// enregistre des captures (.ppm) et des sequences d'images pour les GIF et la video.
//
//   cd outils/simulateur
//   g++ -std=c++17 -O2 -I../../firmware/quantique_console sim_main.cpp (suite)
//       ../../firmware/quantique_console/{gfx,polices,quantique,formule,interface,ecran_*}.cpp (suite)
//       ../../firmware/quantique_console/qrcodegen.c -o sim
//   ./sim captures                 # captures + sequences (captures/seq_<nom>/0000.ppm ...)
#include "gfx.h"
#include "interface.h"
#include "ui_commun.h"
#include "plateforme.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <random>
#include <string>
#include <vector>
#include <sys/stat.h>

static uint32_t g_ms = 0;
static std::map<std::string, std::vector<uint8_t>> g_nvs;
static std::mt19937 g_rng(2026);

uint32_t plat_ms() { return g_ms; }
void plat_luminosite(int) {}
uint32_t plat_alea_materiel() { return g_rng(); }
const char *plat_alea_source() { return "simulateur PC"; }
bool plat_son_ok() { return false; }
const char *plat_son_etat() { return "pas de son (PC)"; }
void plat_bip(float, int, int) {}
bool plat_sd() { return true; }
void plat_csv(const char *l) { FILE *f = fopen("quantique.csv", "a"); if (f) { fprintf(f, "%s\n", l); fclose(f); } }
bool plat_ecrire_fichier(const char *nom, const char *texte)
{
    mkdir("sd_quantique", 0755);
    std::string c = std::string("sd_quantique/") + nom;
    FILE *f = fopen(c.c_str(), "w");
    if (!f) return false;
    fputs(texte, f);
    fclose(f);
    return true;
}
bool plat_nvs_lire(const char *k, void *b, size_t n)
{
    auto it = g_nvs.find(k);
    if (it == g_nvs.end() || it->second.size() != n) return false;
    memcpy(b, it->second.data(), n);
    return true;
}
void plat_nvs_ecrire(const char *k, const void *b, size_t n)
{
    g_nvs[k] = std::vector<uint8_t>((const uint8_t *)b, (const uint8_t *)b + n);
}
uint32_t plat_memoire_libre() { return 172 * 1024; }

// ---------------------------------------------------------------------------
static uint16_t fb[320 * 240], fond[320 * 240];
static std::string g_dossier = ".";
static std::string g_seq;            // sequence en cours d'enregistrement
static int g_seq_n = 0;
static bool g_appui = false;
static int g_x = 0, g_y = 0;
static FILE *g_appuis = nullptr;     // positions du doigt pour la video

static void ppm(const std::string &chemin)
{
    FILE *f = fopen(chemin.c_str(), "wb");
    if (!f) return;
    fprintf(f, "P6\n320 240\n255\n");
    for (int i = 0; i < 320 * 240; i++) {
        uint16_t c = fb[i];
        uint8_t r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
        r |= r >> 5; g |= g >> 6; b |= b >> 5;
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
}

static void image()
{
    ui::toucher(g_appui, g_x, g_y);
    gfx::fb = fb;
    ui::image(g_ms);
    g_ms += 33;
    if (!g_seq.empty()) {
        char n[32];
        snprintf(n, sizeof n, "/%04d.ppm", g_seq_n++);
        ppm(g_dossier + "/seq_" + g_seq + n);
        if (g_appuis) fprintf(g_appuis, "%d %d %d\n", g_appui ? 1 : 0, g_x, g_y);
    }
}

static void attendre(float s) { int n = (int)(s * 30 + 0.5f); for (int i = 0; i < n; i++) image(); }

static void capture(const char *nom)
{
    ppm(g_dossier + "/" + nom + ".ppm");
    printf("capture %-28s t=%6.2f s\n", nom, g_ms / 1000.0);
}

static void sequence(const char *nom)
{
    if (g_appuis) { fclose(g_appuis); g_appuis = nullptr; }
    g_seq = nom ? nom : "";
    g_seq_n = 0;
    if (nom) {
        std::string d = g_dossier + "/seq_" + nom;
        mkdir(d.c_str(), 0755);
        g_appuis = fopen((d + "/appuis.txt").c_str(), "w");
    }
}

static void toucher(int x, int y, float tenir = 0.1f)
{
    g_x = x; g_y = y;
    g_appui = true;
    attendre(tenir);
    g_appui = false;
    image();
}

static void glisser(int x0, int y0, int x1, int y1, float duree = 0.4f)
{
    int n = (int)(duree * 30);
    g_appui = true;
    for (int i = 0; i <= n; i++) {
        g_x = x0 + (x1 - x0) * i / n;
        g_y = y0 + (y1 - y0) * i / n;
        image();
    }
    g_appui = false;
    image();
}

// onglets : 6 cases de 53 px en bas
static void onglet(int o) { toucher(o * 320 / 6 + 26, 222); attendre(0.4f); }

// palette de l'ecran Circuit : 15 outils
static void outil(int i) { float w = 314.0f / 15; toucher((int)(3 + i * w + w / 2), 40); }
// case (colonne, qubit) du circuit, nq qubits
static void case_circuit(int col, int k, int nq)
{
    static const float E[6] = { 22, 24, 22, 20, 18, 16 };
    float e = E[nq];
    int x = 44 + col * 22 + 11, y = (int)(27 + 27 + 2 + e * (k + 0.5f));
    toucher(x, y);
}

int main(int argc, char **argv)
{
    if (argc > 1) g_dossier = argv[1];
    mkdir(g_dossier.c_str(), 0755);
    remove("quantique.csv");
    ui::init(fond);
    gfx::init(fb);

    // ---- demarrage
    sequence("demarrage");
    attendre(2.6f);
    capture("e00_demarrage");
    attendre(1.2f);
    sequence(nullptr);

    // ---- circuit : l'etat de Bell, pas a pas
    capture("c01_circuit_bell");
    sequence("bell_pas_a_pas");
    toucher(19, 27 + 27 + 2 + 48 + 4 + 11);            // ▶ (sous un circuit de 2 qubits)
    attendre(1.0f);
    capture("c02_bell_superposition");
    attendre(1.1f);
    capture("c03_bell_cnot");
    attendre(1.5f);
    capture("c04_bell_mesure");
    attendre(1.0f);
    sequence(nullptr);

    // ---- construire un circuit a la main : GHZ sur 3 qubits
    sequence("construire");
    toucher(188 + 27, 27 + 27 + 2 + 48 + 4 + 11);      // Vider
    toucher(188 + 27, 27 + 27 + 2 + 48 + 4 + 11);      // Sûr ?
    attendre(0.3f);
    toucher(163 + 10, 27 + 27 + 2 + 48 + 4 + 11);      // + qubit
    attendre(0.3f);
    outil(0); case_circuit(0, 0, 3);                    // H sur q0
    outil(11); case_circuit(1, 0, 3); case_circuit(2, 1, 3);   // controles
    outil(1); case_circuit(1, 1, 3); case_circuit(2, 2, 3);    // X (cibles)
    outil(13); case_circuit(3, 0, 3); case_circuit(3, 1, 3); case_circuit(3, 2, 3);
    outil(13);                                          // deselectionne
    attendre(0.6f);
    capture("c05_ghz_construit");
    sequence(nullptr);
    sequence("ghz_lecture");
    toucher(19, 27 + 27 + 2 + 60 + 4 + 3 + 11);
    attendre(4.5f);
    capture("c06_ghz_fin");
    sequence(nullptr);

    // ---- inspecteur d'une porte avec angle
    q::charger_algo(ui::circ, 8);                        // teleportation
    ui::circuit_modifie();
    attendre(0.4f);
    capture("c07_teleportation");
    case_circuit(0, 0, 3);                               // Ry : inspecteur
    attendre(0.5f);
    capture("c08_inspecteur_angle");
    toucher(160, 228 - 60);                              // ailleurs : ferme
    toucher(5, 5);
    attendre(0.3f);

    // ---- sphere de Bloch : teleportation pas a pas, q2
    onglet(1);
    toucher(6 + 2 * 34 + 15, 27 + 28 + 9);               // q2
    sequence("sphere_teleportation");
    toucher(21, 203 - 26 + 11);                          // ▶
    attendre(9.5f);
    capture("b01_sphere_teleportation");
    sequence(nullptr);
    toucher(6 + 0 * 34 + 15, 27 + 28 + 9);               // q0
    toucher(64, 203 - 26 + 11);                          // pas
    q::charger_algo(ui::circ, 2);
    ui::circuit_modifie();
    attendre(0.5f);
    capture("b02_sphere_bell_intrique");
    // bac a sable
    toucher(205, 27 + 13);
    attendre(0.4f);
    sequence("bac_a_sable");
    for (int i : { 0, 4, 5, 6, 0, 9 }) { toucher(6 + i * 31 + 14, 203 - 14); attendre(1.0f); if (i == 5) capture("b03_bac_a_sable_rotation"); }
    attendre(0.6f);
    capture("b04_bac_a_sable_mesure");
    sequence(nullptr);
    toucher(115, 27 + 13);

    // ---- mesures : Grover 3 qubits, 1000 tirs
    q::charger_algo(ui::circ, 7);
    ui::circuit_modifie();
    onglet(0);
    attendre(0.5f);
    capture("c09_grover3");
    onglet(2);
    sequence("tirs_grover");
    toucher(68, 203 - 14);                               // Lancer
    attendre(0.5f);
    capture("m01_tirs_en_cours");
    attendre(1.6f);
    capture("m02_grover_histogramme");
    sequence(nullptr);
    toucher(198 + 58, 203 - 14);                         // bruit faible
    toucher(198 + 58, 203 - 14);                         // bruit QPU
    attendre(2.8f);
    toucher(68, 203 - 14);
    attendre(2.2f);
    capture("m03_grover_bruit_qpu");
    toucher(198 + 58, 203 - 14);                         // parfait
    attendre(0.3f);
    sequence("un_tir");
    toucher(164, 203 - 14);                              // 1 tir
    attendre(1.4f);
    capture("m04_un_tir");
    attendre(1.0f);
    sequence(nullptr);
    // amplitudes
    toucher(20 + 280 / 2, 27 + 13);
    attendre(0.5f);
    capture("m05_amplitudes_grover");
    q::charger_algo(ui::circ, 9);                        // QFT
    ui::circuit_modifie();
    // ... la QFT avant la mesure : on prend l'etat juste avant les mesures
    ui::lecture_jouer();                                 // l'etat juste avant les mesures
    for (int k = 0; k < 8; k++) ui::lecture_pas(1);
    onglet(2);
    attendre(0.2f);
    capture("m06_amplitudes_qft");
    attendre(3);

    // ---- vrai QPU
    q::charger_algo(ui::circ, 2);
    ui::circuit_modifie();
    toucher(20 + 280 * 5 / 6, 27 + 13);
    attendre(0.6f);
    capture("q01_qpu_lien");
    toucher(10, 27 + 28 + 2 + 2 * 18 + 6);               // etape 3
    attendre(0.4f);
    capture("q02_qpu_circuit");
    toucher(188 + 59, 80);                               // agrandir
    attendre(0.4f);
    capture("q03_qr_plein_ecran");
    toucher(160, 120);
    toucher(188 + 30, 203 - 16);                         // SD .qasm
    attendre(0.2f);
    capture("q04_qasm_sd");
    attendre(0.5f);
    toucher(20 + 280 / 6, 27 + 13);                      // retour histogramme

    // ---- algorithmes : toutes les fiches
    onglet(3);
    for (int p = 0; p < 15; p++) {
        ui::algos_aller(p);
        char seq[32];
        snprintf(seq, sizeof seq, "algo_%02d", p);
        sequence(p == 1 || p == 2 || p == 5 || p == 7 || p == 10 || p == 13 ? seq : nullptr);
        attendre(p == 1 ? 3.4f : 2.2f);
        char nom[32];
        snprintf(nom, sizeof nom, "a%02d_algo", p + 1);
        capture(nom);
        attendre(p == 1 ? 1.6f : 0.6f);
        sequence(nullptr);
    }

    // ---- tuto des equations
    onglet(4);
    for (int p = 0; p < 15; p++) {
        ui::tuto_aller(p);
        char seq[32];
        snprintf(seq, sizeof seq, "tuto_%02d", p);
        sequence(p == 3 || p == 4 || p == 5 || p == 9 || p == 11 || p == 14 ? seq : nullptr);
        attendre(2.4f);
        char nom[32];
        snprintf(nom, sizeof nom, "t%02d_tuto", p + 1);
        capture(nom);
        attendre(0.8f);
        sequence(nullptr);
    }
    // essayer depuis le tuto (intrication -> Bell)
    ui::tuto_aller(9);
    attendre(0.5f);
    toucher(71, 203 - 11);
    attendre(2.5f);
    capture("c10_essayer_bell");

    // ---- reglages
    onglet(5);
    capture("r01_reglages_calcul");
    toucher(60 + 150, 27 + 13);
    attendre(0.4f);
    capture("r02_reglages_appareil");
    printf("termine : %.1f s simulees\n", g_ms / 1000.0);
    return 0;
}
