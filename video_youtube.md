# Vidéo YouTube : un ordinateur quantique dans un ESP32

Fichiers produits par `outils/video/` (à mettre dans la page **Releases** du dépôt) :
- `ordinateur_quantique.mp4` : la vidéo à téléverser (1920×1080, 30 i/s, environ 3 min 10 s, voix off et musique) ;
- `ordinateur_quantique.fr.srt` : les sous-titres français (YouTube › Sous-titres › Importer un fichier) ;
- `miniature_youtube.png` : la miniature (1280×720).

## Titre

**J'ai mis un ordinateur quantique de 5 qubits dans un ESP32 (et je l'ai comparé à un vrai !)**

Variantes :
- *Programmer un ordinateur quantique sur une carte ESP32 : superposition, intrication, Grover*
- *5 qubits sur un écran tactile : l'informatique quantique enfin visible (ESP32-S3)*
- *De la sphère de Bloch à un vrai processeur quantique IBM, avec une carte à écran ESP32*

## Description (à coller)

```
Un ordinateur quantique… dans la main ! Une carte ESP32-S3 à écran tactile
(ES3C28P) simule exactement 5 qubits (32 amplitudes complexes). On pose les
portes au doigt et on voit le calcul se dérouler : superposition, interférence,
intrication, effondrement à la mesure. Le hasard vient du bruit de la radio
de la puce.

⚛️ SUR LA CARTE
- Éditeur de circuits : H, X, Y, Z, S, T, Rx, Ry, Rz, CNOT, Toffoli, SWAP, mesure
- Lecture pas à pas animée : chaque porte s'applique en douceur
- Sphère de Bloch 3D de chaque qubit, et un bac à sable
- Des centaines de tirs, l'histogramme, le bruit d'un vrai processeur
- 12 algorithmes prêts : Bell, GHZ, Deutsch-Jozsa, Bernstein-Vazirani, Grover,
  téléportation, transformée de Fourier quantique, estimation de phase…

📚 DEUX TUTORIELS ANIMÉS
- Les algorithmes quantiques connus (Shor, Grover, chimie, optimisation,
  cryptographie quantique…) : à quoi ils servent et où on en est vraiment
- Les différents types d'équations du calcul quantique : linéaire,
  paramétrique, matricielle, différentielle (Schrödinger), valeurs propres…

🖥️ SUR UN VRAI ORDINATEUR QUANTIQUE
La carte écrit le circuit en OpenQASM 2.0 et l'affiche en QR code : on le
colle dans IBM Quantum Composer et on l'exécute gratuitement sur un vrai
processeur quantique (offre Open d'IBM). Les exports ont été vérifiés avec
Qiskit.

📂 Code, firmware prêt à flasher depuis le navigateur, TP pour étudiants,
simulateur PC :
https://github.com/Edunts5962/Esp32-ordinateur-quantique

Les saisons précédentes :
⚡ La vitesse de la lumière avec deux ESP32 (Wi-Fi FTM) :
https://github.com/Edunts5962/Esp32-vitesse-de-la-lumi-re-
🔊 La vitesse du son par écho et l'effet Doppler :
https://github.com/Edunts5962/Esp32-vitesse-du-son
🎵 Fourier & Laplace en vrai :
https://github.com/Edunts5962/Esp32-Fourier-Laplace
🎨 Le moteur graphique utilisé :
https://github.com/Edunts5962/Esp32-gfx-moteur-graphique

Chapitres
0:00 Introduction
0:15 La console
0:34 Un qubit, c'est un vecteur
0:49 Construire un circuit
1:05 L'intrication
1:19 Les portes sont des rotations
1:31 La téléportation
1:41 Mesurer, c'est répéter
1:57 Les algorithmes et leur utilité
2:14 Les types d'équations
2:31 Sur un vrai ordinateur quantique
2:45 Vérifié avec Qiskit, et le TP
3:03 Fin

Matériel : une carte ES3C28P (ESP32-S3, écran 2,8" tactile, haut-parleur).
Rien à câbler. Arduino IDE, paquet ESP32 3.x — ou firmware à flasher depuis
Chrome, sans rien installer.
Voix de synthèse : Piper, voix SIWIS (CC-BY 4.0). Police Inter (SIL OFL).
Les écrans de la démonstration viennent du simulateur PC (même code que la
carte). Projet non affilié à IBM.

#ESP32 #quantique #ordinateurquantique #qubit #Grover #Qiskit #physique #DIY #Arduino
```

## Tags

ESP32, ESP32-S3, ordinateur quantique, informatique quantique, qubit, sphère de Bloch, superposition, intrication, état de Bell, algorithme de Grover, algorithme de Shor, transformée de Fourier quantique, téléportation quantique, OpenQASM, IBM Quantum, Qiskit, physique quantique, équation de Schrödinger, TP, DIY, Arduino, ES3C28P

## Paramètres conseillés

- Catégorie : Science et technologie ; langue : français.
- Sous-titres : importer `ordinateur_quantique.fr.srt`.
- Écran de fin (20 dernières secondes) : la vidéo « Fourier & Laplace » et la playlist de la série.
- Commentaire épinglé : « Vous avez lancé un circuit sur un vrai QPU ? Postez votre histogramme de Bell ! »

## Refaire la vidéo

```
cd outils/simulateur && ./build.sh && ./sim cap          # écrans de la démonstration
cd ../video
pip install piper-tts pycairo numpy scipy pillow
python3 narration.py fr_FR-siwis-medium.onnx voix         # voix off (Piper, voix SIWIS)
python3 faire_video.py ../simulateur/cap voix sortie      # MP4 + .srt
python3 faire_video.py ../simulateur/cap voix sortie --miniature
```

La voix française vient du projet sherpa-onnx :
`vits-piper-fr_FR-siwis-medium` (dans les « tts-models » de ses publications GitHub).
