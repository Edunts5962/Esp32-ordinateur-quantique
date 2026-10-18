#!/bin/sh
# Fabrique les GIF du README a partir des sequences du simulateur.
#   cd outils/simulateur && ./build.sh && ./sim cap && cd .. && ./faire_gifs.sh simulateur/cap ../animations
SRC=$1
DST=$2
mkdir -p "$DST"
gif() {        # gif <nom> <sequence> [<sequence> ...]
    nom=$1; shift
    tmp=$(mktemp -d)
    i=0
    for s in "$@"; do
        for f in "$SRC"/seq_$s/*.ppm; do
            cp "$f" "$tmp/$(printf %05d $i).ppm"; i=$((i + 1))
        done
    done
    ffmpeg -loglevel error -y -framerate 30 -i "$tmp/%05d.ppm" \
        -vf "fps=15,scale=480:360:flags=lanczos,split[a][b];[a]palettegen=max_colors=192[p];[b][p]paletteuse=dither=sierra2_4a" \
        "$DST/$nom.gif"
    rm -rf "$tmp"
    echo "$DST/$nom.gif"
}
gif anim_circuit construire ghz_lecture
gif anim_bell bell_pas_a_pas
gif anim_sphere bac_a_sable
gif anim_teleportation sphere_teleportation
gif anim_mesures tirs_grover un_tir
gif anim_algos algo_01 algo_02 algo_10
gif anim_tuto tuto_04 tuto_09 tuto_11
