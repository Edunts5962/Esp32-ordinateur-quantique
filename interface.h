// interface.h - ecrans animes et tactile de la console "Ordinateur quantique"
#pragma once
#include <stdint.h>

#define OQ_VERSION "v1.0"

namespace ui {

// fond : tampon L*H facultatif (PSRAM) pour le fond pre-calcule, ou nullptr
void init(uint16_t *fond);

// a chaque image : etat du doigt (coordonnees ecran 320x240)
void toucher(bool appui, int x, int y);

// dessine une image complete dans gfx::fb
void image(uint32_t ms);

// pour le simulateur et la video : aller directement a un ecran
void aller(int onglet, int page = -1);
void passer_demarrage();

}  // namespace ui
