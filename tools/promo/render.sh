#!/bin/sh
# UFO 40 - renders the promo video from real play, run from the repo root:
#
#   FFMPEG=/path/to/ffmpeg sh tools/promo/render.sh
#
# 1. the headless runner plays tools/promo/clips/*.ufs (one per cartridge)
#    and boot/library/ohnew.ufs, writing every frame as a PNG and the sound
#    effects as a WAV (record_start / wav_start); the clips' overlay_text
#    lines are left out of the recordings and become promo_fx's moving
#    captions; music.ufs renders the montage track, cards.ufs the vertical
#    video's cards
# 2. promo_fx (tools/promo/fx.c) plays the edit in cuts.txt, drawing the
#    transitions, punches, shakes, captions, the grid and the end card at the
#    output size (nearest neighbour, so every game pixel stays sharp), and
#    pipes the frames into one ffmpeg per output:
#      OUT/ufo40-promo.mp4           1920x1080, 60 fps, H.264 + AAC
#      OUT/ufo40-promo-nolink.mp4    the same, the end card without the link
#      OUT/ufo40-promo-vertical.mp4  1080x1920, the game at x3 between cards
#      OUT/ufo40-promo.gif           640x360, 30 fps, silent, loops
# The sound: the effects under the montage track (from the first frame),
# faded out over the last 1.5 s and brought to about -14 LUFS.
#
# OUT defaults to build/promo; CRT=0 leaves out the scanlines and vignette.
# One ffmpeg at a time, two threads.
set -e
cd "$(dirname "$0")/../.."
FF=${FFMPEG:-ffmpeg}
OUT=${OUT:-build/promo}
CRT=${CRT:-1}
TH="-threads 2"
HL=./build/ufo40_headless
[ -f "$HL.exe" ] && HL=$HL.exe
FX=./build/promo_fx
[ -f "$FX.exe" ] && FX=$FX.exe

make headless promo_fx
rm -rf "$OUT/clips" "$OUT/save" "$OUT/run"
mkdir -p "$OUT/run"

# ---- 1. play everything ----------------------------------------------------
play() {
    n=$(basename "$1" .ufs)
    sed -e "s/@GAMES@/$games/g" -e '/^overlay_text/d' "$1" > "$OUT/run/$n.ufs"
    "$HL" --script "$OUT/run/$n.ufs" --save-dir "$OUT/save/$n" --out "$OUT/clips" > "$OUT/run/$n.log" ||
        { cat "$OUT/run/$n.log"; echo "render: $1 failed"; exit 1; }
}
games=0
for s in tools/promo/clips/*.ufs tools/promo/boot.ufs tools/promo/library.ufs \
         tools/promo/ohnew.ufs tools/promo/music.ufs; do
    play "$s"
done
FXA="-clips $OUT/clips -cuts tools/promo/cuts.txt -scripts tools/promo/clips"
info=$("$FX" $FXA -info)
echo "$info" | head -1
games=$(echo "$info" | head -1 | sed 's/.*, \([0-9]*\) games$/\1/')
total=$(echo "$info" | head -1 | sed 's/^[0-9]* shots, \([0-9]*\) frames.*/\1/')
# the cards keep their words (overlay_text is what they are made of)
sed "s/@GAMES@/$games/g" tools/promo/cards.ufs > "$OUT/run/cards.ufs"
"$HL" --script "$OUT/run/cards.ufs" --save-dir "$OUT/save/cards" --out "$OUT/clips" > "$OUT/run/cards.log"

# ---- 2. the sound -------------------------------------------------------------
"$FX" $FXA -wav "$OUT/sfx.wav"
fade=$(awk "BEGIN { print $total / 60 - 1.5 }")
"$FF" -y -v error -i "$OUT/sfx.wav" -i "$OUT/clips/music.wav" -filter_complex \
    "[0:a]volume=1.25[fx];[1:a]volume=0.8[mu];[fx][mu]amix=inputs=2:duration=first:normalize=0,afade=t=out:st=$fade:d=1.5[a]" \
    -map "[a]" -c:a pcm_s16le $TH "$OUT/mix.wav"
lufs=$("$FF" -hide_banner -i "$OUT/mix.wav" -af ebur128 -f null - 2>&1 | sed -n 's/^ *I: *\(-*[0-9.]*\) LUFS/\1/p' | tail -1)
gain=$(awk "BEGIN { print -14 - ($lufs) }")
"$FF" -y -v error -i "$OUT/mix.wav" -af "volume=${gain}dB,alimiter=limit=0.84:level=false" \
    -c:a pcm_s16le $TH "$OUT/audio.wav"
echo "sound: $lufs LUFS, $gain dB"

# ---- 3. the videos ------------------------------------------------------------
enc="-c:v libx264 -preset medium -tune animation -crf 20 -profile:v high -pix_fmt yuv420p -r 60 -c:a aac -b:a 192k -ar 48000 -movflags +faststart"
video() { # video OUTFILE W H FX-OPTIONS...
    o=$1 w=$2 h=$3
    shift 3
    "$FX" $FXA -crt "$CRT" "$@" |
        "$FF" -y -v error -f rawvideo -pix_fmt rgb24 -s "${w}x$h" -r 60 -i - -i "$OUT/audio.wav" \
            -map 0:v -map 1:a $enc -shortest $TH "$o"
    echo "wrote $o"
}
video "$OUT/ufo40-promo.mp4" 1920 1080 -size 1920 1080 6 0 0
video "$OUT/ufo40-promo-nolink.mp4" 1920 1080 -size 1920 1080 6 0 0 -end nolink
# vertical: the header card, the game and the footer card, each x3, on ink
video "$OUT/ufo40-promo-vertical.mp4" 1080 1920 -size 1080 1920 3 60 690 \
    -cards "$OUT/clips/card_top.png" "$OUT/clips/card_bottom.png"

# ---- 4. the GIF: the loudest stretch - BOOMTOWN's blasts, the rapid-fire run,
# TIN TROOP's victory, the grid and the end card - at 30 fps, x2, no dither.
# It starts just past BOOMTOWN's white flash (a better first frame) and
# loops back into it.
start() { echo "$info" | awk -v n="$1" -v k="$2" '$2 == n { c++; if (c == k) print $1 }'; }
g0=$(start 10 1); g1=$(start 13 1); g2=$(start 04 1); g3=$(start 06 1); g4=$(start grid 1)
e0=$(start boot 2)
"$FX" $FXA -crt 0 -size 640 360 2 0 0 -step 2 \
    -range $((g0 + 2)) $((g0 + 96)) -range "$g1" "$g2" -range "$g3" $((g3 + 96)) -range "$g4" $((e0 + ${GIF_END:-180})) |
    "$FF" -y -v error -f rawvideo -pix_fmt rgb24 -s 640x360 -r 30 -i - -filter_complex \
        "split[s0][s1];[s0]palettegen=max_colors=128:stats_mode=full[pal];[s1][pal]paletteuse=dither=none" \
        -loop 0 $TH "$OUT/ufo40-promo.gif"
echo "wrote $OUT/ufo40-promo.gif"
