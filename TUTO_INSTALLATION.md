# Tutoriel : installer l'ordinateur quantique sur l'ES3C28P (sans rien compiler)

Durée : environ 5 minutes. Il ne faut **ni Arduino, ni Python** : seulement un ordinateur avec **Chrome ou Edge** et un câble USB.

## Ce qu'il faut

| | |
|---|---|
| La carte **ES3C28P** (ESP32-S3, écran 2,8" tactile) | celle des saisons précédentes |
| Un câble **USB-C** qui transporte les données (pas seulement la charge) | pour flasher |
| Facultatif : une carte **micro-SD** (FAT32) | résultats des tirs et fichiers `.qasm` |
| Facultatif : un téléphone et un compte gratuit **IBM Quantum** | pour le vrai ordinateur quantique |

Rien d'autre : pas de Wi-Fi à configurer, pas de clé. La radio de la carte s'allume seule, uniquement pour fournir un vrai hasard ; elle ne se connecte à aucun réseau.

## Étape 1 : télécharger le firmware

Sur la page GitHub du projet, ouvrir le dossier **`binaires/`**, cliquer sur **`quantique_console_complet.bin`**, puis sur le bouton de téléchargement (« Download raw file »).

## Étape 2 : flasher la carte depuis le navigateur

1. Brancher la carte en USB.
2. Ouvrir **https://espressif.github.io/esptool-js/** dans Chrome ou Edge.
3. Cliquer **Connect**, choisir le port de la carte (« USB JTAG/serial debug unit » ou COMx), puis valider.
   Si rien ne se connecte : garder le bouton **BOOT** appuyé, appuyer sur **RESET**, relâcher BOOT, puis recommencer « Connect ».
4. Dans **Flash Address**, laisser **0x0**, choisir le fichier `quantique_console_complet.bin`, puis cliquer **Program**. Attendre la fin (moins d'une minute).
5. Appuyer sur **RESET**.

## Étape 3 : premier démarrage

- Si la carte n'a jamais été étalonnée (par la console NES ou un projet précédent), l'écran demande de toucher le centre de **trois croix**.
- L'écran de démarrage indique la source du hasard. On doit lire « bruit radio (vrai hasard) ».
- L'état de Bell est déjà chargé dans l'onglet **Circuit** : toucher ▶.

## Étape 4 (facultative) : la carte micro-SD

Insérer une carte micro-SD formatée en FAT32 **avant** d'allumer la console. Elle crée :

```
/quantique.csv            une ligne par série de tirs (circuit, bruit, comptes)
/quantique/Bell.qasm      les circuits exportés (Mesures › Vrai QPU › SD .qasm)
```

## Étape 5 (facultative) : un vrai ordinateur quantique

1. Créer un compte sur **IBM Quantum Platform** (offre gratuite « Open »).
2. Sur la carte, aller dans **Mesures › Vrai QPU** et suivre les 4 étapes affichées. Toucher un QR code l'affiche en plein écran.
3. Dans le Composer : *View › Panels › Code Editor*, coller le circuit, *Run circuit*, choisir un processeur, 1 000 tirs.

## En cas de problème

| Symptôme | Que faire |
|---|---|
| L'écran reste noir | Le firmware suppose l'écran de l'ES3C28P (ILI9341, SPI 10-13, DC 46). Pour une autre carte, compiler le croquis avec Arduino IDE (voir le README, § 4). |
| Le tactile est décalé | Effacer l'étalonnage puis redémarrer : les trois croix réapparaissent. |
| Pas de port à la connexion | Essayer un autre câble USB (certains ne font que la charge), puis la manœuvre BOOT + RESET. |
| Pas de son | Réglages › Appareil › Son « Clics », volume Fort. |
