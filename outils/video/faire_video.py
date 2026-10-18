#!/usr/bin/env python3
"""
faire_video.py - video de presentation du projet "Ordinateur quantique" (1920x1080, 30 i/s, ~3 min).

Dessin vectoriel avec pycairo, encodage H.264 avec ffmpeg. Les ecrans de la
console viennent du simulateur PC (outils/simulateur : meme code que la carte),
avec la position du doigt (appuis.txt) pour montrer chaque toucher.

  pip install pycairo numpy pillow scipy
  python3 narration.py <fr_FR-siwis-medium.onnx> voix          # voix off d'abord
  python3 faire_video.py <dossier captures du simulateur> voix sortie
  python3 faire_video.py <dossier captures du simulateur> voix sortie --miniature
  python3 faire_video.py <dossier captures du simulateur> voix sortie --test   (vues fixes)

Sorties : ordinateur_quantique.mp4 (avec voix et musique), ordinateur_quantique.fr.srt,
miniature_youtube.png
"""
import json
import math
import os
import random
import subprocess
import sys
import wave

import cairo
import numpy as np
from PIL import Image

W, H, FPS = 1920, 1080, 30
POLICE = "Inter"
POLICE_TITRE = "Inter Display"
POLICE_MATH = "DejaVu Sans"      # a les crochets ⟨ ⟩ des kets
GITHUB = "github.com/Edunts5962/Esp32-ordinateur-quantique"


def c(r, g, b): return (r / 255, g / 255, b / 255)


FOND_HAUT, FOND_BAS = c(4, 4, 20), c(18, 6, 42)
PANNEAU, PANNEAU2, BORD = c(18, 18, 54), c(32, 30, 84), c(70, 60, 150)
TEXTE, TEXTE2, TEXTE3 = c(240, 242, 255), c(166, 170, 220), c(104, 106, 160)
CYAN, BLEU, VIOLET, MAGENTA = c(40, 232, 255), c(70, 120, 255), c(150, 90, 255), c(255, 60, 210)
JAUNE, ORANGE, ROUGE, VERT, ROSE = c(255, 214, 64), c(255, 140, 60), c(255, 70, 100), c(60, 255, 160), c(255, 120, 200)


def sat(x): return 0.0 if x < 0 else (1.0 if x > 1 else x)
def sortie(x): x = sat(x); return 1 - (1 - x) ** 3
def app(t, t0, d=0.6): return sortie((t - t0) / d)


# ================================================================== dessin
def texte(ctx, s, x, y, taille, coul=TEXTE, a=1.0, gras=True, align="g", famille=POLICE):
    ctx.select_font_face(famille, cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD if gras else cairo.FONT_WEIGHT_NORMAL)
    ctx.set_font_size(taille)
    e = ctx.text_extents(s)
    if align == "c": x -= e.x_advance / 2
    elif align == "d": x -= e.x_advance
    ctx.move_to(x, y)
    ctx.set_source_rgba(*coul, a)
    ctx.show_text(s)
    return e.x_advance


def bloc(ctx, s, x, y, larg, taille, coul=TEXTE2, a=1.0, gras=False, interligne=1.4):
    ctx.select_font_face(POLICE, cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD if gras else cairo.FONT_WEIGHT_NORMAL)
    ctx.set_font_size(taille)
    mots, ligne, yy = s.split(" "), "", y
    for m in mots:
        essai = (ligne + " " + m).strip()
        if ctx.text_extents(essai).x_advance > larg and ligne:
            texte(ctx, ligne, x, yy, taille, coul, a, gras)
            yy += taille * interligne
            ligne = m
        else:
            ligne = essai
    if ligne:
        texte(ctx, ligne, x, yy, taille, coul, a, gras)
        yy += taille * interligne
    return yy


def rrect(ctx, x, y, w, h, r):
    ctx.new_path()
    ctx.arc(x + w - r, y + r, r, -math.pi / 2, 0)
    ctx.arc(x + w - r, y + h - r, r, 0, math.pi / 2)
    ctx.arc(x + r, y + h - r, r, math.pi / 2, math.pi)
    ctx.arc(x + r, y + r, r, math.pi, 3 * math.pi / 2)
    ctx.close_path()


def halo(ctx, x, y, r, coul, a=1.0):
    g = cairo.RadialGradient(x, y, 0, x, y, r)
    g.add_color_stop_rgba(0, *coul, 0.55 * a)
    g.add_color_stop_rgba(0.4, *coul, 0.18 * a)
    g.add_color_stop_rgba(1, *coul, 0)
    ctx.set_source(g)
    ctx.arc(x, y, r, 0, 2 * math.pi)
    ctx.fill()


def disque(ctx, x, y, r, coul, a=1.0):
    ctx.new_path()
    ctx.arc(x, y, r, 0, 2 * math.pi)
    ctx.set_source_rgba(*coul, a)
    ctx.fill()


def trait(ctx, x0, y0, x1, y1, ep, coul, a=1.0, pointille=None):
    ctx.new_path()
    ctx.move_to(x0, y0)
    ctx.line_to(x1, y1)
    ctx.set_line_width(ep)
    ctx.set_line_cap(cairo.LINE_CAP_ROUND)
    ctx.set_dash(pointille or [])
    ctx.set_source_rgba(*coul, a)
    ctx.stroke()
    ctx.set_dash([])


def pastille(ctx, s, x, y, taille, coul, a=1.0):
    ctx.select_font_face(POLICE, cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
    ctx.set_font_size(taille)
    l = ctx.text_extents(s).x_advance + taille * 1.4
    h = taille * 1.7
    rrect(ctx, x, y - h * 0.72, l, h, h / 2)
    ctx.set_source_rgba(*[v * 0.35 for v in coul], 0.9 * a)
    ctx.fill_preserve()
    ctx.set_source_rgba(*coul, a)
    ctx.set_line_width(2)
    ctx.stroke()
    texte(ctx, s, x + taille * 0.7, y, taille, TEXTE, a)
    return l


# ---- fond pre-calcule : degrade, halos, etoiles
_FOND = None
_ETOILES = [(random.Random(i).random() * W, random.Random(i + 999).random() * H,
             random.Random(i + 5).random() * 2 + 0.5, random.Random(i + 7).random() * 6.28) for i in range(220)]


def preparer_fond():
    global _FOND
    _FOND = cairo.ImageSurface(cairo.FORMAT_ARGB32, W, H)
    ctx = cairo.Context(_FOND)
    g = cairo.LinearGradient(0, 0, 0, H)
    g.add_color_stop_rgb(0, *FOND_HAUT)
    g.add_color_stop_rgb(1, *FOND_BAS)
    ctx.set_source(g)
    ctx.paint()
    halo(ctx, 1600, 160, 700, VIOLET, 0.35)
    halo(ctx, 200, 900, 650, BLEU, 0.3)
    halo(ctx, 960, 1200, 600, MAGENTA, 0.22)


def fond(ctx, T):
    ctx.set_source_surface(_FOND, 0, 0)
    ctx.paint()
    for (x, y, r, ph) in _ETOILES:
        xx = (x - T * 12 * r) % W
        a = 0.25 + 0.35 * (0.5 + 0.5 * math.sin(T * 1.3 + ph))
        disque(ctx, xx, y, r * 0.8, TEXTE, a)


def atome(ctx, cx, cy, R, T, a=1.0):
    for i, col in enumerate((CYAN, MAGENTA, VIOLET)):
        ang = i * math.pi / 3 + T * 0.4
        ctx.save()
        ctx.translate(cx, cy)
        ctx.rotate(ang)
        ctx.scale(1, 0.38)
        ctx.new_path()
        ctx.arc(0, 0, R, 0, 2 * math.pi)
        ctx.restore()
        ctx.set_source_rgba(*col, 0.8 * a)
        ctx.set_line_width(R * 0.03)
        ctx.stroke()
        u = T * (2.2 + i * 0.6) + i * 2
        ex, ey = R * math.cos(u), R * 0.38 * math.sin(u)
        x = cx + ex * math.cos(ang) - ey * math.sin(ang)
        y = cy + ex * math.sin(ang) + ey * math.cos(ang)
        halo(ctx, x, y, R * 0.3, col, a)
        disque(ctx, x, y, R * 0.05, TEXTE, a)
    halo(ctx, cx, cy, R * 0.5, JAUNE, 0.8 * a)
    disque(ctx, cx, cy, R * 0.08, JAUNE, a)


def sphere(ctx, cx, cy, r, th, ph, coul, a=1.0, lacet=0.5, tangage=0.32):
    def proj(x, y, z):
        cl, sl, ct, st = math.cos(lacet), math.sin(lacet), math.cos(tangage), math.sin(tangage)
        x1, y1 = x * cl - y * sl, x * sl + y * cl
        return cx + r * x1, cy - r * (z * ct + y1 * st), y1 * ct - z * st
    halo(ctx, cx, cy, r * 1.4, coul, 0.5 * a)
    disque(ctx, cx, cy, r, c(14, 14, 44), 0.85 * a)
    ctx.new_path(); ctx.arc(cx, cy, r, 0, 2 * math.pi)
    ctx.set_source_rgba(*BORD, a); ctx.set_line_width(3); ctx.stroke()
    for ax in range(3):
        pts = []
        for i in range(61):
            u = i * 2 * math.pi / 60
            p = [(math.cos(u), math.sin(u), 0), (math.cos(u), 0, math.sin(u)), (0, math.cos(u), math.sin(u))][ax]
            pts.append(proj(*p))
        for i in range(60):
            (x0, y0, p0), (x1, y1, p1) = pts[i], pts[i + 1]
            trait(ctx, x0, y0, x1, y1, 2, BORD, (0.7 if p0 + p1 > 0 else 0.25) * a)
    x, y, _ = proj(math.sin(th) * math.cos(ph), math.sin(th) * math.sin(ph), math.cos(th))
    trait(ctx, cx, cy, x, y, 14, coul, 0.25 * a)
    trait(ctx, cx, cy, x, y, 5, coul, a)
    halo(ctx, x, y, r * 0.25, coul, a)
    disque(ctx, x, y, r * 0.06, TEXTE, a)
    tx, ty, _ = proj(0, 0, 1.25)
    texte(ctx, "|0⟩", tx, ty, r * 0.2, CYAN, a, True, "c", POLICE_MATH)
    tx, ty, _ = proj(0, 0, -1.3)
    texte(ctx, "|1⟩", tx, ty + r * 0.15, r * 0.2, MAGENTA, a, True, "c", POLICE_MATH)


# ================================================================== images du simulateur
DOSSIER_SIM = "."
_CACHE = {}


def surface_ppm(chemin):
    if chemin in _CACHE:
        return _CACHE[chemin]
    im = Image.open(chemin).convert("RGB")
    a = np.asarray(im)
    bgra = np.zeros((a.shape[0], a.shape[1], 4), dtype=np.uint8)
    bgra[..., 0], bgra[..., 1], bgra[..., 2], bgra[..., 3] = a[..., 2], a[..., 1], a[..., 0], 255
    s = cairo.ImageSurface.create_for_data(bytearray(bgra.tobytes()), cairo.FORMAT_ARGB32, a.shape[1], a.shape[0])
    if len(_CACHE) > 400:
        _CACHE.clear()
    _CACHE[chemin] = s
    return s


def longueur_seq(nom):
    d = os.path.join(DOSSIER_SIM, "seq_" + nom)
    return len([f for f in os.listdir(d) if f.endswith(".ppm")])


def appuis(nom):
    p = os.path.join(DOSSIER_SIM, "seq_" + nom, "appuis.txt")
    if not os.path.exists(p):
        return []
    return [tuple(int(v) for v in l.split()) for l in open(p) if l.strip()]


_APPUIS = {}


def image_liste(liste, t):
    """liste : [("seq", nom) ou ("cap", nom, duree)] -> (chemin ppm, doigt ou None)"""
    k = 0.0
    for e in liste:
        if e[0] == "seq":
            n = longueur_seq(e[1])
            d = n / FPS
            if t < k + d or e is liste[-1]:
                i = min(n - 1, max(0, int((t - k) * FPS)))
                if e[1] not in _APPUIS:
                    _APPUIS[e[1]] = appuis(e[1])
                ap = _APPUIS[e[1]]
                doigt = ap[i] if i < len(ap) and ap[i][0] else None
                return os.path.join(DOSSIER_SIM, "seq_" + e[1], f"{i:04d}.ppm"), doigt
        else:
            d = e[2]
            if t < k + d or e is liste[-1]:
                return os.path.join(DOSSIER_SIM, e[1] + ".ppm"), None
        k += d
    return None, None


def duree_liste(liste):
    return sum(longueur_seq(e[1]) / FPS if e[0] == "seq" else e[2] for e in liste)


def console(ctx, x, y, echelle, chemin, doigt, a=1.0, T=0.0):
    """La carte ES3C28P : circuit imprime, ecran, et l'image de la console."""
    lw, lh = 320 * echelle, 240 * echelle
    m = 26 * echelle / 3
    # le circuit imprime
    rrect(ctx, x - 2.2 * m, y - 2.2 * m, lw + 4.4 * m, lh + 6.4 * m, 4 * m)
    ctx.set_source_rgba(*c(20, 16, 40), a)
    ctx.fill_preserve()
    ctx.set_source_rgba(*VIOLET, 0.8 * a)
    ctx.set_line_width(3)
    ctx.stroke()
    halo(ctx, x + lw / 2, y + lh / 2, lw * 0.75, CYAN, 0.25 * a)
    rrect(ctx, x - 0.6 * m, y - 0.6 * m, lw + 1.2 * m, lh + 1.2 * m, 1.2 * m)
    ctx.set_source_rgba(0, 0, 0, a)
    ctx.fill()
    if chemin:
        s = surface_ppm(chemin)
        ctx.save()
        ctx.translate(x, y)
        ctx.scale(echelle, echelle)
        ctx.set_source_surface(s, 0, 0)
        ctx.get_source().set_filter(cairo.FILTER_GOOD)
        ctx.paint_with_alpha(a)
        ctx.restore()
    texte(ctx, "ES3C28P · ESP32-S3", x + lw / 2, y + lh + 3.2 * m, 2.2 * m, TEXTE3, a, True, "c")
    if doigt:
        fx, fy = x + doigt[1] * echelle, y + doigt[2] * echelle
        halo(ctx, fx, fy, 22 * echelle, BLANC, 0.9 * a)
        ctx.new_path(); ctx.arc(fx, fy, 9 * echelle, 0, 2 * math.pi)
        ctx.set_source_rgba(1, 1, 1, 0.75 * a); ctx.set_line_width(3); ctx.stroke()


BLANC = (1, 1, 1)


# ================================================================== scenes
def panneau_gauche(ctx, t, num, titre, lignes, coul, larg=700):
    """Titre de chapitre et points cles, a gauche de la console."""
    a = app(t, 0.1, 0.5)
    pastille(ctx, f"{num:02d}", 90, 200, 26, coul, a)
    y = bloc(ctx, titre, 90, 290, larg, 62, TEXTE, a, True, 1.12)
    y += 20
    for i, l in enumerate(lignes):
        k = app(t, 0.8 + i * 0.7, 0.5)
        if k <= 0:
            continue
        if l.startswith("="):           # une formule, en grand
            halo(ctx, 90 + 250, y + 10, 260, coul, 0.25 * k)
            texte(ctx, l[1:], 90, y + 40, 56, coul, k, True, "g", POLICE_MATH)
            y += 100
        else:
            disque(ctx, 102, y + 2, 7, coul, k)
            y = bloc(ctx, l, 128, y + 12, larg - 40, 36, TEXTE2, k, False, 1.3) + 10


def scene_intro(ctx, t, D, T):
    atome(ctx, 960, 400, 330, T, app(t, 0, 1.2))
    k = app(t, 0.6, 1)
    sphere(ctx, 960, 400, 150, 1.0 + 0.5 * math.sin(t * 0.9), t * 1.6, CYAN, k, 0.5 + t * 0.3)
    k2 = app(t, 1.5, 0.8)
    texte(ctx, "UN ORDINATEUR QUANTIQUE", 960, 840, 104, TEXTE, k2, True, "c", POLICE_TITRE)
    texte(ctx, "dans la main, avec un ESP32", 960, 930, 56, JAUNE, app(t, 2.3, 0.8), True, "c", POLICE_TITRE)
    texte(ctx, "5 qubits · 32 amplitudes · un écran tactile", 960, 1010, 34, TEXTE2, app(t, 3.2, 0.8), False, "c")


def scene_outro(ctx, t, D, T):
    atome(ctx, 960, 300, 200, T, 0.9)
    texte(ctx, "Tout est sur GitHub", 960, 600, 84, TEXTE, app(t, 0.2), True, "c", POLICE_TITRE)
    texte(ctx, GITHUB, 960, 690, 48, CYAN, app(t, 0.6), True, "c")
    y = 800
    for i, (s, col) in enumerate([("firmware prêt à flasher depuis le navigateur", VERT),
                                   ("code source · simulateur PC · TP pour la classe", JAUNE),
                                   ("saisons précédentes : vitesse de la lumière · vitesse du son · Fourier & Laplace", TEXTE2)]):
        texte(ctx, s, 960, y + i * 60, 36, col, app(t, 1.2 + i * 0.5), False, "c")
    if t > D - 2.5:
        texte(ctx, "Merci d'avoir regardé !", 960, 1040, 40, MAGENTA, app(t, D - 2.5), True, "c")


def scene_verif(ctx, t, D, T):
    panneau_gauche(ctx, t, 11, "Vérifié avec Qiskit", [
        "Les 12 algorithmes et 30 circuits au hasard, exportés en OpenQASM",
        "Mêmes probabilités qu'IBM, à 2·10⁻⁵ près",
        "Un TP en 4 parties : superposition, intrication, Grover, vrai QPU"], VERT, 760)
    x, y, w, h = 960, 170, 880, 760
    a = app(t, 0.3)
    rrect(ctx, x, y, w, h, 24)
    ctx.set_source_rgba(*c(8, 8, 22), 0.95 * a)
    ctx.fill_preserve()
    ctx.set_source_rgba(*VERT, 0.6 * a)
    ctx.set_line_width(2)
    ctx.stroke()
    lignes = ["$ python3 verifier_qasm.py qa",
              "OK  Pile ou face quantique        1e-16", "OK  Interférence (Mach-Zehnder)   4e-16",
              "OK  État de Bell (intrication)    1e-16", "OK  GHZ : 3 qubits intriqués      1e-16",
              "OK  Deutsch-Jozsa                 4e-16", "OK  Bernstein-Vazirani            6e-16",
              "OK  Grover (2 qubits)             9e-16", "OK  Grover (3 qubits)             5e-07",
              "OK  Téléportation quantique       6e-17", "OK  QFT : trouver une période     4e-16",
              "OK  Estimation de phase (T)       1e-06", "OK  … 30 circuits au hasard",
              "Tout concorde avec Qiskit."]
    n = int((t - 0.6) * 4)
    for i, l in enumerate(lignes[:max(0, n)]):
        col = VERT if l.startswith("OK") or l.startswith("Tout") else TEXTE2
        texte(ctx, l, x + 40, y + 70 + i * 48, 30, col, 1.0, l.startswith("Tout"), "g", "DejaVu Sans Mono")


# chaque scene : (titre, couleur, lignes du panneau, liste d'images du simulateur ou fonction)
SCENES = [
    ("intro", None, None, scene_intro),
    ("La console", CYAN, ["Une carte ES3C28P : ESP32-S3, écran tactile 2,8\"", "Rien à câbler",
                          "5 qubits simulés exactement", "Un vrai hasard : le bruit de la radio"],
     [("seq", "demarrage"), ("cap", "c01_circuit_bell", 3.0)]),
    ("Un qubit, c'est un vecteur", JAUNE, ["=|ψ⟩ = α|0⟩ + β|1⟩", "Deux amplitudes complexes",
                                           "Un point de la sphère de Bloch"],
     [("cap", "t02_tuto", 4.0), ("seq", "tuto_03"), ("seq", "tuto_03")]),
    ("Construire un circuit", CYAN, ["Une porte H, deux CNOT, trois mesures", "Lecture pas à pas",
                                     "La mesure fait s'effondrer l'état"],
     [("seq", "construire"), ("seq", "ghz_lecture")]),
    ("L'intrication", ROSE, ["=(|00⟩ + |11⟩)/√2", "Toujours 00 ou 11", "Jamais l'un sans l'autre"],
     [("seq", "bell_pas_a_pas"), ("seq", "bell_pas_a_pas")]),
    ("Les portes sont des rotations", VIOLET, ["H, X, Y, Z, S, T, Rx, Ry, Rz", "Chaque porte tourne la flèche",
                                               "La mesure la fait sauter vers un pôle"],
     [("seq", "bac_a_sable")]),
    ("La téléportation", VERT, ["L'état de q0 réapparaît sur q2", "Grâce à une paire intriquée",
                                "Suivie en direct sur la sphère"],
     [("seq", "sphere_teleportation")]),
    ("Mesurer, c'est répéter", JAUNE, ["Grover, 1 000 tirs : 101 sort à 94,5 %",
                                       "Avec le bruit d'un vrai QPU : ≈ 50 %", "Un seul tir : le hasard décide"],
     [("seq", "tirs_grover"), ("cap", "m02_grover_histogramme", 3.0), ("cap", "m03_grover_bruit_qpu", 3.5),
      ("seq", "un_tir")]),
    ("Les algorithmes et leur utilité", MAGENTA, ["Shor, Grover, QFT, chimie, optimisation…",
                                                  "À quoi ça sert, le gain espéré", "Où on en est vraiment aujourd'hui"],
     [("seq", "algo_01"), ("seq", "algo_02"), ("seq", "algo_05"), ("seq", "algo_10"), ("seq", "algo_13")]),
    ("Les types d'équations", CYAN, ["Linéaire, paramétrique, matricielle", "Euler, Schrödinger, valeurs propres",
                                     "La transformée de Fourier quantique"],
     [("seq", "tuto_04"), ("seq", "tuto_05"), ("seq", "tuto_09"), ("seq", "tuto_11"), ("seq", "tuto_14")]),
    ("Sur un vrai ordinateur quantique", MAGENTA, ["Le circuit en OpenQASM 2.0, en QR code",
                                                   "Collé dans IBM Quantum Composer", "Exécuté gratuitement sur un vrai QPU"],
     [("cap", "q01_qpu_lien", 3.5), ("cap", "q02_qpu_circuit", 4.0), ("cap", "q03_qr_plein_ecran", 4.5)]),
    ("vérif", None, None, scene_verif),
    ("outro", None, None, scene_outro),
]

DOSSIER_VOIX = "voix"
DEBUT, ECART, FIN = 0.8, 0.45, 1.0


def chronologie():
    repl = json.load(open(os.path.join(DOSSIER_VOIX, "narration.json")))
    durees, departs = [], []
    t0 = 0.0
    for i, sc in enumerate(SCENES):
        rs = [r for r in repl if r["scene"] == i]
        dv = DEBUT + sum(r["duree"] for r in rs) + ECART * max(0, len(rs) - 1) + FIN
        if i == 0:
            dv += 2.0                         # le titre s'installe avant la voix
        dl = duree_liste(sc[3]) + 0.4 if isinstance(sc[3], list) else 0
        d = max(dv, dl, 4.0)
        tt = t0 + DEBUT + (2.0 if i == 0 else 0)
        for r in rs:
            departs.append((tt, r))
            tt += r["duree"] + ECART
        durees.append(d)
        t0 += d
    return durees, departs


def rendre_image(ctx, i, t, D, T, total):
    fond(ctx, T)
    titre, coul, lignes, contenu = SCENES[i]
    # fondu entre scenes
    if callable(contenu):
        contenu(ctx, t, D, T)
    else:
        panneau_gauche(ctx, t, i, titre, lignes, coul)
        chemin, doigt = image_liste(contenu, max(0, t - 0.3))
        a = app(t, 0, 0.5)
        console(ctx, 900, 170, 2.9, chemin, doigt, a, T)
    # barre de progression
    ctx.set_source_rgba(*PANNEAU2, 0.8)
    ctx.rectangle(0, H - 8, W, 8)
    ctx.fill()
    g = cairo.LinearGradient(0, 0, W, 0)
    g.add_color_stop_rgb(0, *CYAN)
    g.add_color_stop_rgb(1, *MAGENTA)
    ctx.set_source(g)
    ctx.rectangle(0, H - 8, W * T / total, 8)
    ctx.fill()
    # fondu au noir aux transitions
    f = min(sat(t / 0.35), sat((D - t) / 0.35))
    if f < 1:
        ctx.set_source_rgba(0, 0, 0, 1 - f)
        ctx.paint()


def rendre_video(sortie_dir):
    durees, _ = chronologie()
    total = sum(durees)
    muette = os.path.join(sortie_dir, "muette.mp4")
    ff = subprocess.Popen(["ffmpeg", "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "bgra",
                           "-s", f"{W}x{H}", "-r", str(FPS), "-i", "-", "-c:v", "libx264", "-preset", "medium",
                           "-crf", "19", "-pix_fmt", "yuv420p", muette], stdin=subprocess.PIPE)
    surf = cairo.ImageSurface(cairo.FORMAT_ARGB32, W, H)
    ctx = cairo.Context(surf)
    T0 = 0.0
    for i, D in enumerate(durees):
        n = int(round(D * FPS))
        for k in range(n):
            t = k / FPS
            rendre_image(ctx, i, t, D, T0 + t, total)
            surf.flush()
            ff.stdin.write(bytes(surf.get_data()))
        T0 += n / FPS
        print(f"scene {i:2d} : {D:5.1f} s")
    ff.stdin.close()
    ff.wait()
    return muette, total


# ================================================================== son
SR = 44100


def musique(duree):
    """Nappe douce et scintillante (accords Ré m - Si♭ - Fa - Do), stereo."""
    n = int(duree * SR)
    t = np.arange(n) / SR
    out = np.zeros((n, 2))
    accords = [(146.83, 174.61, 220.0, 293.66), (116.54, 146.83, 174.61, 233.08),
               (130.81, 174.61, 220.0, 261.63), (130.81, 164.81, 196.0, 261.63)]
    pas = 8.0
    for k in range(int(duree / pas) + 2):
        acc = accords[k % 4]
        t0 = k * pas - 1.5
        i0, i1 = max(0, int(t0 * SR)), min(n, int((t0 + pas + 3.0) * SR))
        if i0 >= i1:
            continue
        tt = t[i0:i1] - t0
        env = np.clip(np.minimum(1, tt / 2.5) * np.minimum(1, (pas + 3.0 - tt) / 2.5), 0, 1) ** 2
        for j, f in enumerate(acc):
            pan = 0.3 + 0.4 * (j / 3)
            o = np.sin(2 * np.pi * f * tt) + np.sin(2 * np.pi * (f + 0.2) * tt) + 0.25 * np.sin(4 * np.pi * f * tt)
            out[i0:i1, 0] += o * env * (1 - pan)
            out[i0:i1, 1] += o * env * pan
        # arpege cristallin (les "mesures")
        for m in range(8):
            ta = m * 1.0
            fa = acc[m % 4] * 4
            ia = int((t0 + 1.5 + ta) * SR)
            if 0 <= ia < n:
                L_ = min(n - ia, int(1.2 * SR))
                ts = np.arange(L_) / SR
                note = np.sin(2 * np.pi * fa * ts) * np.exp(-ts * 4) * 0.25
                out[ia:ia + L_, m % 2] += note
    rev = out.copy()
    for d, g in ((0.29, 0.35), (0.47, 0.25), (0.71, 0.15)):
        k = int(d * SR)
        rev[k:] += out[:-k] * g
    out = rev
    out *= np.clip(np.minimum(1, t / 3.0) * np.minimum(1, (duree - t) / 4.0), 0, 1)[:, None]
    return out / (np.abs(out).max() + 1e-9)


def piste_son(sortie_dir, total):
    from scipy.signal import resample_poly
    _, departs = chronologie()
    n = int(total * SR) + SR
    voix = np.zeros(n)
    for t0, r in departs:
        with wave.open(os.path.join(DOSSIER_VOIX, r["fichier"])) as w:
            sr = w.getframerate()
            x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float64) / 32768
        x = resample_poly(x, SR, sr)
        i0 = int(t0 * SR)
        voix[i0:i0 + len(x)] += x[: n - i0]
    voix *= 0.89 / (np.abs(voix).max() + 1e-9)

    def glisse(x, k):
        cs = np.cumsum(np.concatenate(([0.0], x)))
        y = (cs[k:] - cs[:-k]) / k
        return np.concatenate((np.full(k // 2, y[0]), y, np.full(len(x) - len(y) - k // 2, y[-1])))
    pres = np.clip(glisse((np.abs(voix) > 0.02).astype(float), SR // 4) * 3, 0, 1)
    pres = glisse(pres, int(SR * 0.6))
    mix = musique(n / SR)[:n] * (0.17 * (1 - 0.55 * pres))[:, None] + voix[:, None]
    mix /= max(1.0, np.abs(mix).max() / 0.97)
    chemin = os.path.join(sortie_dir, "piste_son.wav")
    with wave.open(chemin, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((mix * 32767).astype(np.int16).tobytes())

    def hms(x):
        return f"{int(x // 3600):02d}:{int(x % 3600 // 60):02d}:{int(x % 60):02d},{int(round((x % 1) * 1000)) % 1000:03d}"
    with open(os.path.join(sortie_dir, "ordinateur_quantique.fr.srt"), "w", encoding="utf-8") as f:
        for k, (t0, r) in enumerate(departs, 1):
            f.write(f"{k}\n{hms(t0)} --> {hms(t0 + r['duree'] + 0.2)}\n{r['sous_titre']}\n\n")
    return chemin


def chapitres():
    durees, _ = chronologie()
    noms = ["Introduction", "La console", "Un qubit, c'est un vecteur", "Construire un circuit", "L'intrication",
            "Les portes sont des rotations", "La téléportation", "Mesurer, c'est répéter",
            "Les algorithmes et leur utilité", "Les types d'équations", "Sur un vrai ordinateur quantique",
            "Vérifié avec Qiskit, et le TP", "Fin"]
    t = 0.0
    for nom, d in zip(noms, durees):
        print(f"{int(t // 60)}:{int(t % 60):02d} {nom}")
        t += d


def miniature(sortie_dir):
    surf = cairo.ImageSurface(cairo.FORMAT_ARGB32, W, H)
    ctx = cairo.Context(surf)
    fond(ctx, 2.0)
    atome(ctx, 1450, 560, 430, 1.3, 0.75)
    console(ctx, 1010, 260, 2.6, os.path.join(DOSSIER_SIM, "b01_sphere_teleportation.ppm"), None, 1.0)
    texte(ctx, "ORDINATEUR", 70, 230, 150, TEXTE, 1, True, "g", POLICE_TITRE)
    texte(ctx, "QUANTIQUE", 70, 390, 150, CYAN, 1, True, "g", POLICE_TITRE)
    halo(ctx, 330, 560, 260, MAGENTA, 0.5)
    texte(ctx, "5 qubits", 70, 600, 130, MAGENTA, 1, True, "g", POLICE_TITRE)
    texte(ctx, "dans un ESP32 !", 70, 760, 110, JAUNE, 1, True, "g", POLICE_TITRE)
    pastille(ctx, "+ un VRAI QPU", 80, 900, 52, VERT, 1)
    petite = cairo.ImageSurface(cairo.FORMAT_ARGB32, 1280, 720)
    c2 = cairo.Context(petite)
    c2.scale(1280 / W, 720 / H)
    c2.set_source_surface(surf, 0, 0)
    c2.get_source().set_filter(cairo.FILTER_BEST)
    c2.paint()
    chemin = os.path.join(sortie_dir, "miniature_youtube.png")
    petite.write_to_png(chemin)
    print("miniature :", chemin)


if __name__ == "__main__":
    DOSSIER_SIM, DOSSIER_VOIX, sortie_dir = sys.argv[1], sys.argv[2], sys.argv[3]
    os.makedirs(sortie_dir, exist_ok=True)
    preparer_fond()
    args = sys.argv[4:]
    if "--miniature" in args:
        miniature(sortie_dir)
    elif "--test" in args:
        durees, _ = chronologie()
        surf = cairo.ImageSurface(cairo.FORMAT_ARGB32, W, H)
        ctx = cairo.Context(surf)
        for i, D in enumerate(durees):
            t = D * 0.6
            rendre_image(ctx, i, t, D, sum(durees[:i]) + t, sum(durees))
            surf.write_to_png(os.path.join(sortie_dir, f"test_{i:02d}.png"))
        chapitres()
    elif "--chapitres" in args:
        chapitres()
    else:
        muette, total = rendre_video(sortie_dir)
        son = piste_son(sortie_dir, total)
        finale = os.path.join(sortie_dir, "ordinateur_quantique.mp4")
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", muette, "-i", son, "-c:v", "copy",
                        "-c:a", "aac", "-b:a", "192k", "-shortest", "-movflags", "+faststart", finale], check=True)
        os.remove(muette)
        print(f"video : {finale} ({total:.0f} s)")
        chapitres()
