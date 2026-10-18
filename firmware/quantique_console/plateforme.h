// plateforme.h - ce que le coeur du programme demande au materiel.
// Implemente par quantique_console.ino (carte ES3C28P) et par le simulateur PC
// (outils/simulateur/sim_main.cpp). Le reste du code (ecrans, simulateur
// quantique, dessin) est identique sur les deux.
#pragma once
#include <stdint.h>
#include <stddef.h>

uint32_t plat_ms();                       // millisecondes depuis le demarrage
void plat_luminosite(int pct);            // retroeclairage 5..100 %

// hasard : generateur materiel de l'ESP32 (bruit de la radio), ou pseudo-aleatoire
uint32_t plat_alea_materiel();
const char *plat_alea_source();           // texte court pour l'ecran

// son (codec ES8311) : un petit bip de f Hz pendant ms millisecondes (non bloquant)
bool plat_son_ok();
const char *plat_son_etat();
void plat_bip(float f, int ms, int volume);   // volume 0..2

// carte SD
bool plat_sd();
void plat_csv(const char *ligne);                          // ajoute une ligne a quantique.csv
bool plat_ecrire_fichier(const char *nom, const char *texte);  // /quantique/<nom>

bool plat_nvs_lire(const char *cle, void *buf, size_t n);
void plat_nvs_ecrire(const char *cle, const void *buf, size_t n);
uint32_t plat_memoire_libre();            // octets de memoire interne libres
