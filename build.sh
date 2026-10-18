#!/bin/sh
# compile le simulateur PC de la console (meme code que la carte)
F=../../firmware/quantique_console
g++ -std=c++17 -O2 -I$F sim_main.cpp \
    $F/gfx.cpp $F/polices.cpp $F/quantique.cpp $F/formule.cpp $F/interface.cpp \
    $F/ecran_circuit.cpp $F/ecran_bloch.cpp $F/ecran_mesures.cpp $F/ecran_qpu.cpp \
    $F/ecran_algos.cpp $F/ecran_tuto.cpp -x c $F/qrcodegen.c -o sim
