# Publier le projet sur GitHub

Le dépôt s'appelle **`Esp32-ordinateur-quantique`**, sur le compte `Edunts5962`. Tous les liens du README, de la vidéo et de la description YouTube pointent vers cette adresse.

## 1. Créer le dépôt

1. Sur github.com : **+ › New repository**.
2. Nom : `Esp32-ordinateur-quantique`. Description :
   *Un ordinateur quantique de 5 qubits sur un ESP32-S3 à écran tactile : circuits, sphère de Bloch, algorithmes, tutoriels, et export vers un vrai QPU IBM.*
3. Public, **sans** README ni licence (ils sont déjà dans le dossier). Puis **Create repository**.

## 2. Envoyer les fichiers

### Méthode A : depuis le navigateur (sans rien installer)

1. Décompresser `Esp32-ordinateur-quantique.zip`.
2. Sur la page du dépôt vide : **uploading an existing file**.
3. **Glisser le contenu du dossier** (pas le dossier lui-même) : `README.md`, `LICENSE`, et les dossiers `firmware`, `binaires`, `images`, `animations`, `outils`…

   ⚠️ Il faut **glisser les dossiers**, et non sélectionner les fichiers un par un : sinon tous les fichiers se retrouvent à la racine, et les images du README ne s'affichent plus. Chrome et Edge gardent l'arborescence quand on glisse un dossier.
4. Message : « Version 1.0 », puis **Commit changes**.
5. Vérifier que la page du dépôt affiche bien les GIF du README.

### Méthode B : avec git

```
cd Esp32-ordinateur-quantique
git init -b main
git add .
git commit -m "Version 1.0 : ordinateur quantique sur ESP32"
git remote add origin https://github.com/Edunts5962/Esp32-ordinateur-quantique.git
git push -u origin main
```

## 3. Créer la « Release » (vidéo et firmware)

1. Sur le dépôt : **Releases › Create a new release**.
2. Tag : `v1.0`. Titre : *Ordinateur quantique v1.0*.
3. Glisser dans « Attach binaries » :
   - `ordinateur_quantique.mp4` (la vidéo) et `ordinateur_quantique.fr.srt` ;
   - `binaires/quantique_console_complet.bin` (le firmware, aussi présent dans le dépôt).
4. Texte de la release, par exemple :

```
Première version.
- Firmware complet à flasher à l'adresse 0x0 (voir TUTO_INSTALLATION.md)
- Vidéo de présentation (3 min) et sous-titres français
- Vérifié : 12 algorithmes + 30 circuits au hasard identiques à Qiskit (IBM)
```

5. **Publish release**.

## 4. Finitions

- **About** (roue dentée, à droite) : ajouter les sujets (topics)
  `esp32` `esp32-s3` `quantum-computing` `qubit` `bloch-sphere` `openqasm` `qiskit` `arduino` `physique` `education`.
- Dans le README des saisons précédentes, ajouter un lien vers cette saison.
- Sur YouTube : titre, description, chapitres et tags dans [`video_youtube.md`](video_youtube.md). Le lien GitHub est déjà dans la description.

## Ce qu'il ne faut pas publier

Rien de secret dans ce projet : pas de clé, pas de mot de passe. Si vous ajoutez un jour un compte IBM à la carte, gardez la clé sur la carte SD, jamais sur GitHub.
