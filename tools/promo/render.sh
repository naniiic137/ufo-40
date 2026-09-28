#!/bin/sh
# UFO 40 - renders the promo video from real play, run from the repo root:
#
#   FFMPEG=/path/to/ffmpeg sh tools/promo/render.sh
#
# 1. the headless runner plays tools/promo/clips/*.ufs (one per cartridge)
#    and boot/library/endcard.ufs, writing every frame as a PNG and the
#    sound effects as a WAV (record_start / wav_start), captions included;
#    music.ufs renders the montage track, cards.ufs the vertical video's cards
# 2. cuts.txt picks the shots; they are joined into a lossless 320x180 master
# 3. the master is scaled up by whole numbers (nearest neighbour) into
#      OUT/ufo40-promo.mp4           1920x1080, 60 fps, H.264 + AAC
#      OUT/ufo40-promo-vertical.mp4  1080x1920, the game at x3 between cards
#      OUT/ufo40-promo.gif           640x360, a silent preview
#
# OUT defaults to build/promo. One ffmpeg at a time, two threads.
set -e
cd "$(dirname "$0")/../.."
FF=${FFMPEG:-ffmpeg}
OUT=${OUT:-build/promo}
TH="-threads 2"
HL=./build/ufo40_headless
[ -f "$HL.exe" ] && HL=$HL.exe

make headless
rm -rf "$OUT/clips" "$OUT/save" "$OUT/run"
mkdir -p "$OUT/run"
games=$(ls tools/promo/clips/*.ufs | wc -l | tr -d ' ')

# ---- 1. play everything ----------------------------------------------------
for s in tools/promo/clips/*.ufs tools/promo/boot.ufs tools/promo/library.ufs \
         tools/promo/endcard.ufs tools/promo/music.ufs tools/promo/cards.ufs; do
    n=$(basename "$s" .ufs)
    sed "s/@GAMES@/$games/g" "$s" > "$OUT/run/$n.ufs"
    "$HL" --script "$OUT/run/$n.ufs" --save-dir "$OUT/save/$n" --out "$OUT/clips" > "$OUT/run/$n.log" ||
        { cat "$OUT/run/$n.log"; echo "render: $s failed"; exit 1; }
done
echo "played $games cartridges"

# ---- 2. the edit: a lossless 320x180 master with the sound effects ----------
args=""; filt=""; cat=""; i=0; total=0; first_len=0; montage=0
while read -r clip first frames rest; do
    case "$clip" in '' | \#*) continue ;; esac
    args="$args -framerate 60 -start_number $first -i $OUT/clips/$clip/%06d.png -i $OUT/clips/$clip.wav"
    filt="$filt[$((2 * i)):v]trim=end_frame=$frames,setpts=PTS-STARTPTS[v$i];"
    filt="$filt[$((2 * i + 1)):a]atrim=start_sample=$((first * 800)):end_sample=$(((first + frames) * 800)),asetpts=PTS-STARTPTS[a$i];"
    cat="$cat[v$i][a$i]"
    [ $i -eq 0 ] && first_len=$frames
    [ $i -eq 1 ] && montage=$((first_len + frames))
    i=$((i + 1)); total=$((total + frames))
done < tools/promo/cuts.txt
# shellcheck disable=SC2086
"$FF" -y -v error $args -filter_complex "$filt${cat}concat=n=$i:v=1:a=1[v][a]" \
    -map "[v]" -map "[a]" -c:v ffv1 -c:a pcm_s16le $TH "$OUT/master.mkv"
secs=$(awk "BEGIN { printf \"%.3f\", $total / 60 }")
echo "master: $i shots, $total frames ($secs s)"

# ---- 3. the sound: effects under the montage track, which starts after the
# first shot (the boot jingle) and fades out over the end card's last 1.5 s
delay=$((first_len * 1000 / 60))
fade=$(awk "BEGIN { print $total / 60 - 1.5 }")
mix="[0:a]volume=1.25[fx];[1:a]volume=0.8,adelay=$delay|$delay,apad[mu];[fx][mu]amix=inputs=2:duration=first:normalize=0,afade=t=out:st=$fade:d=1.5,alimiter=limit=0.89[a]"
enc="-c:v libx264 -preset slow -tune animation -crf 16 -profile:v high -pix_fmt yuv420p -r 60 -c:a aac -b:a 192k -ar 48000 -movflags +faststart"

"$FF" -y -v error -i "$OUT/master.mkv" -i "$OUT/clips/music.wav" \
    -filter_complex "[0:v]scale=1920:1080:flags=neighbor[v];$mix" \
    -map "[v]" -map "[a]" $enc $TH "$OUT/ufo40-promo.mp4"
echo "wrote $OUT/ufo40-promo.mp4"

# vertical: the header card, the game and the footer card, each x3, on ink
"$FF" -y -v error -i "$OUT/master.mkv" -i "$OUT/clips/music.wav" \
    -i "$OUT/clips/card_top.png" -i "$OUT/clips/card_bottom.png" \
    -filter_complex "color=c=0x0e0b16:s=1080x1920:r=60[bg];[0:v]scale=960:540:flags=neighbor[g];[2:v]scale=960:540:flags=neighbor[t];[3:v]scale=960:540:flags=neighbor[b];[bg][t]overlay=60:90[x];[x][g]overlay=60:690:shortest=1[y];[y][b]overlay=60:1290[v];$mix" \
    -map "[v]" -map "[a]" $enc $TH "$OUT/ufo40-promo-vertical.mp4"
echo "wrote $OUT/ufo40-promo-vertical.mp4"

# GIF: the start of the montage (after the first two shots), then the end
# card; 30 fps, x2, no dithering
m0=$montage; m1=$((m0 + ${GIF_FRAMES:-768})); e1=$total; e0=$((total - 150))
"$FF" -y -v error -i "$OUT/master.mkv" -filter_complex \
    "[0:v]split[p][q];[p]trim=start_frame=$m0:end_frame=$m1,setpts=PTS-STARTPTS[m];[q]trim=start_frame=$e0:end_frame=$e1,setpts=PTS-STARTPTS[e];[m][e]concat=n=2:v=1:a=0,fps=30,scale=640:360:flags=neighbor,split[s0][s1];[s0]palettegen=max_colors=64:stats_mode=full[pal];[s1][pal]paletteuse=dither=none" \
    $TH "$OUT/ufo40-promo.gif"
echo "wrote $OUT/ufo40-promo.gif"
