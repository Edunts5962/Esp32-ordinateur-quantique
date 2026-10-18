# TP : programmer un ordinateur quantique (5 qubits sur un ESP32)

**Durée :** 2 × 1 h 30 · **Niveau :** lycée (terminale, spécialité physique ou NSI) à licence
**Matériel :** une console ES3C28P par binôme (programme « Ordinateur quantique »), un téléphone, et en option un compte gratuit IBM Quantum.

## Objectifs

- Manipuler la superposition, la phase, l'interférence et l'intrication.
- Relier les **types d'équations** (linéaire, quadratique, matricielle, probabiliste…) à ce qu'on observe.
- Comprendre la nature **statistique** de la mesure : un tir ne suffit pas.
- Exécuter un circuit sur un **vrai** processeur quantique et interpréter ses erreurs.

Notations : les résultats s'écrivent avec q0 **à droite**, comme sur la carte et chez IBM (« 01 » veut dire q1 = 0 et q0 = 1).

---

## Partie 1 : un qubit (30 min)

**1.1 Pile ou face quantique.** Onglet Algos › fiche « Hasard », ou Circuit : un qubit, une porte H, une mesure M.

1. Lancez la lecture pas à pas (▶). Dans quel état est le qubit après H ? Que montre l'onglet Sphère ?
2. Faites 10 fois « 1 tir » (onglet Mesures) et notez les résultats. Puis « Lancer 100 tirs », puis 1 000.
3. Combien de « 0 » attendiez-vous sur 1 000 tirs ? L'écart observé est-il normal ? (L'écart-type d'une loi binomiale vaut √(N p (1 − p)), soit environ 16 ici.)

**1.2 La sphère de Bloch (Sphère › Bac à sable).**

4. Appliquez H, puis encore H. Où revient la flèche ? Écrivez l'équation matricielle H·H = ? (Tuto, page 6.)
5. Appliquez H, puis S quatre fois. Que fait S ? Les probabilités P(0) et P(1) changent-elles ? Que change S, alors ?
6. Partez de |0⟩. Combien de fois faut-il appliquer T pour obtenir le même effet que Z ? Justifiez avec les angles (T = π/4, Z = π).

**1.3 L'interférence (Algos › « Interférence »).** Circuit H, Z, H, M.

7. Prévoyez le résultat sans Z, puis avec Z, et vérifiez.
8. Pourquoi Z, qui ne change aucune probabilité, change-t-il pourtant le résultat final ? Utilisez l'idée que ½ + ½ = 1 et ½ − ½ = 0 (Tuto, page 14).

*Valeurs attendues : 1 000 tirs de H donnent 500 ± 30 « 0 » ; H H = I ; S ne change pas P(0) mais tourne la phase de 90° ; Z = T⁴ ; H Z H donne 1 à 100 %, H H donne 0 à 100 %.*

---

## Partie 2 : deux qubits et l'intrication (30 min)

**2.1 Construire l'état de Bell** (onglet Circuit, « Vider », 2 qubits) : H sur q0, puis dans la colonne suivante ● sur q0 et X sur q1, puis M sur les deux.

9. Lisez le circuit pas à pas en observant les barres d'amplitude. Écrivez l'état après chaque colonne.
10. Lancez 1 000 tirs. Quels résultats n'apparaissent **jamais** ?
11. Onglet Sphère : que vaut |r| pour q0 et pour q1 ? Comparez avec le Tuto, page 10 (« équation sans solution »). Montrez qu'on ne peut pas trouver a₀, a₁, b₀, b₁ tels que (a₀|0⟩ + a₁|1⟩) ⊗ (b₀|0⟩ + b₁|1⟩) = (|00⟩ + |11⟩)/√2.

**2.2 GHZ.** Ajoutez un troisième qubit et un CNOT q1 → q2.

12. Quels résultats obtient-on ? Combien de nombres faudrait-il pour décrire 3 qubits ? Et 50 ?

*Valeurs attendues : 00 et 11 à 50 % chacun, jamais 01 ni 10 ; |r| = 0 pour les deux qubits ; 000 et 111 ; 2³ = 8 amplitudes, et 2⁵⁰ ≈ 10¹⁵.*

---

## Partie 3 : un algorithme, Grover (30 min)

Algos › fiche « Grover » › Essayer (3 qubits, cible 101).

13. Pendant la lecture, notez la probabilité de 101 après la 1ʳᵉ itération (colonne 9) puis après la 2ᵉ (colonne 17). Comparez avec la formule P = sin²((2k + 1)θ), où sin θ = 1/√8 (Tuto, page 14).
14. Supprimez la 2ᵉ itération (colonnes 10 à 17). Quelle probabilité obtenez-vous ? Que se passerait-il avec une 3ᵉ itération ?
15. Combien d'essais faudrait-il en moyenne pour trouver la cible en ouvrant les 8 « tiroirs » au hasard ? Et avec Grover, pour N = 1 000 000 de tiroirs ?

*Valeurs attendues : 78,1 % puis 94,5 % ; une 3ᵉ itération ferait redescendre P à environ 33 % ; 4 essais en moyenne sans quantique ; environ (π/4)·√N ≈ 785 itérations de Grover pour un million.*

---

## Partie 4 : le bruit et un vrai ordinateur quantique (1 h)

**4.1 Le bruit sur la carte.** Mesures › bouton « Bruit QPU ».

16. Relancez 1 000 tirs de Bell, puis de Grover à 3 qubits. Notez la probabilité du bon résultat dans chaque cas.
17. Pourquoi Grover (46 portes) souffre-t-il bien plus du bruit que Bell (2 portes) ? Estimez la probabilité qu'aucune des 46 portes ne se trompe si chacune a 2 % d'erreur : 0,98⁴⁶ ≈ ?

**4.2 Sur un vrai processeur quantique.** Mesures › Vrai QPU (voir le README, § 6).

18. Créez un compte IBM Quantum (offre gratuite « Open »). Scannez le QR du Composer, puis celui du circuit de Bell. Collez le texte dans le Code Editor et lancez-le avec 1 000 tirs.
19. Recopiez l'histogramme obtenu. Quelle part de résultats « impossibles » (01, 10) observez-vous ? Comparez avec la carte en mode « Bruit QPU ».
20. Le Composer affiche aussi le **circuit transpilé**. Combien de portes contient-il ? Pourquoi est-il différent du vôtre ? (Indice : la puce ne sait faire que quelques portes natives, et tous ses qubits ne sont pas reliés entre eux.)
21. Facultatif : lancez Grover à 3 qubits sur le vrai QPU. Le résultat 101 reste-t-il majoritaire ?

*Valeurs attendues : Bell reste vers 48 % / 48 % avec quelques % d'erreurs ; Grover tombe à 50 % environ avec le bruit QPU ; 0,98⁴⁶ ≈ 0,39 ; sur un vrai QPU, Bell donne typiquement 1 à 10 % de 01 et 10, selon la machine et le jour.*

---

## Pour aller plus loin

- **Tuto** : pour chaque page, classez l'équation (linéaire, quadratique, matricielle, différentielle…) et dites ce qu'elle « contraint » ou « prédit ».
- **Téléportation** (Algos) : suivez q2 sur la sphère pendant la lecture. À quelle colonne q2 prend-il l'état du message ?
- **Estimation de phase** : modifiez le circuit pour estimer la phase de S au lieu de T. Quel résultat doit-on mesurer ?
- **Le hasard** : réglez Hasard › Graine fixe, faites 1 000 tirs deux fois et comparez. Repassez en « Matériel ». Qu'est-ce qu'un vrai hasard ?
