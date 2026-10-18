#!/usr/bin/env python3
"""
narration.py - voix off francaise de la video (Piper TTS, voix SIWIS).

Voix : Piper fr_FR-siwis-medium (jeu de donnees SIWIS, licence CC-BY 4.0),
version publiee par le projet sherpa-onnx (vits-piper-fr_FR-siwis-medium).

Les textes sont ecrits pour etre BIEN PRONONCES (sigles epeles :
"E S P trente-deux"), les sous-titres utilisent l'ecriture normale.

  pip install piper-tts numpy
  python3 narration.py <modele.onnx> <dossier_sortie>     -> wav + narration.json
"""
import json
import os
import sys
import wave

# (scene, texte prononce, sous-titre ou None si identique)
REPLIQUES = [
    (0, "Un ordinateur quantique… dans la main.", None),
    (0, "Cinq qubits, trente-deux amplitudes complexes, et un écran tactile : aujourd'hui, on programme un ordinateur quantique sur une carte E S P trente-deux.",
     "Cinq qubits, 32 amplitudes complexes, et un écran tactile : aujourd'hui, on programme un ordinateur quantique sur une carte ESP32."),

    (1, "La console, c'est la carte E S trois C vingt-huit P des saisons précédentes : une puce E S P trente-deux S trois, un écran tactile, un haut-parleur. Rien à câbler.",
     "La console, c'est la carte ES3C28P des saisons précédentes : une puce ESP32-S3, un écran tactile, un haut-parleur. Rien à câbler."),
    (1, "Elle calcule exactement l'état de cinq qubits, et tire ses mesures au hasard, grâce au bruit de sa propre radio.", None),

    (2, "Un bit vaut zéro ou un. Un qubit, lui, est un vecteur : un mélange de zéro et de un, avec deux amplitudes, alpha et bêta.", None),
    (2, "Chaque état d'un qubit est un point d'une sphère : la sphère de Bloch.", None),

    (3, "On construit un circuit au doigt : une porte H, deux portes C NOT, et trois mesures.",
     "On construit un circuit au doigt : une porte H, deux portes CNOT, et trois mesures."),
    (3, "La lecture pas à pas applique chaque porte en douceur. À la mesure, l'état s'effondre : trois zéros ou trois uns, jamais un mélange.", None),

    (4, "Voici l'état de Bell. Après H, le premier qubit est en superposition. Après la porte C NOT, les deux qubits sont intriqués : on mesure zéro zéro, ou un un, jamais l'un sans l'autre.",
     "Voici l'état de Bell. Après H, le premier qubit est en superposition. Après la porte CNOT, les deux qubits sont intriqués : on mesure 00 ou 11, jamais l'un sans l'autre."),

    (5, "Dans le bac à sable, chaque porte est une rotation : la flèche tourne autour de son axe, qui s'allume en rose.", None),
    (5, "Et la mesure la fait sauter vers un pôle, au hasard.", None),

    (6, "La téléportation quantique : grâce à l'intrication, l'état du premier qubit réapparaît sur le troisième. La sphère le montre en direct.", None),

    (7, "Une mesure quantique ne donne qu'un résultat au hasard : il faut répéter le circuit. Mille tirs de l'algorithme de Grover : la bonne réponse, un zéro un, sort dans quatre-vingt-quatorze pour cent des cas.",
     "Une mesure quantique ne donne qu'un résultat au hasard : il faut répéter le circuit. 1 000 tirs de l'algorithme de Grover : la bonne réponse, 101, sort dans 94 % des cas."),
    (7, "Un seul tir ? C'est le hasard qui décide.", None),

    (8, "Quinze fiches animées présentent les algorithmes connus : Shor pour la cryptographie, Grover pour la recherche, la chimie, l'optimisation, la cryptographie quantique…", None),
    (8, "Chacune dit à quoi ça sert, le gain espéré, et où on en est vraiment aujourd'hui.", None),

    (9, "Un tutoriel explique aussi les différents types d'équations du calcul quantique : la formule d'Euler, les matrices, l'intrication, l'équation de Schrödinger, et la transformée de Fourier quantique.", None),

    (10, "Et pour passer au vrai : la carte écrit le circuit en open Q A S M, et l'affiche en QR code.",
     "Et pour passer au vrai : la carte écrit le circuit en OpenQASM, et l'affiche en QR code."),
    (10, "On le colle dans le Composer d'I B M, et on l'exécute gratuitement sur un véritable processeur quantique.",
     "On le colle dans le Composer d'IBM, et on l'exécute gratuitement sur un véritable processeur quantique."),

    (11, "Les circuits exportés ont été vérifiés avec Qiskit, la bibliothèque d'I B M : mêmes probabilités, au cent-millième près.",
     "Les circuits exportés ont été vérifiés avec Qiskit, la bibliothèque d'IBM : mêmes probabilités, au cent-millième près."),
    (11, "Et un T P, en quatre parties, accompagne le projet : superposition, intrication, Grover, et le bruit d'un vrai processeur.",
     "Et un TP, en quatre parties, accompagne le projet : superposition, intrication, Grover, et le bruit d'un vrai processeur."),

    (12, "Tout est sur guitt heub : le firmware prêt à flasher depuis le navigateur, le code, et le tutoriel.", None),
    (12, "Merci d'avoir regardé !", None),
]

_PROPRE = [("guitt heub", "GitHub")]


# mots que la voix prononce mal : on les ecrit "comme ils se disent"
_VOIX = [("qubits", "kiou-bittes"), ("qubit", "kiou-bitte"), ("Qiskit", "Kisse-kitte"),
         ("QR code", "Q R code"), ("Schrödinger", "Chreu-dinguère")]


def pour_la_voix(s):
    for a, b in _VOIX:
        s = s.replace(a, b)
    return s


def propre(s):
    for a, b in _PROPRE:
        s = s.replace(a, b)
    return s


VITESSE = 1.05      # > 1 : plus lent (length_scale de Piper)


def synthetiser(modele, sortie):
    from piper import PiperVoice
    from piper.config import SynthesisConfig
    voix = PiperVoice.load(modele)
    cfg = SynthesisConfig(length_scale=VITESSE)
    infos = []
    for i, (sc, dit, st) in enumerate(REPLIQUES):
        chemin = os.path.join(sortie, f"r{i:02d}.wav")
        with wave.open(chemin, "wb") as w:
            voix.synthesize_wav(pour_la_voix(dit), w, syn_config=cfg)
        with wave.open(chemin) as w:
            duree = w.getnframes() / w.getframerate()
        infos.append({"scene": sc, "dit": dit, "sous_titre": propre(st or dit),
                      "fichier": os.path.basename(chemin), "duree": round(duree, 3)})
        print(f"{i:02d} scene {sc:2d}  {duree:5.2f} s  {dit[:60]}")
    json.dump(infos, open(os.path.join(sortie, "narration.json"), "w"), ensure_ascii=False, indent=1)


if __name__ == "__main__":
    os.makedirs(sys.argv[2], exist_ok=True)
    synthetiser(sys.argv[1], sys.argv[2])
