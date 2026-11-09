#!/bin/bash
# Headless emulator helper. Usage:
#   emu.sh start-x                      start Xvfb on :99
#   emu.sh ngpc <rom.ngp|.ngc>          run Mednafen NGPC core
#   emu.sh amiga [config.fs-uae]        run FS-UAE (default A500 + AROS ROM, hd = amiga/hd)
#   emu.sh shot <ngpc|amiga> <out.png>  screenshot that emulator's window
#   emu.sh key <ngpc|amiga> <xdotool key...>   send keys (e.g. Return, Up, x)
#   emu.sh stop                         kill emulators and Xvfb
export DISPLAY=:99 SDL_AUDIODRIVER=dummy
mkdir -p "$(cd "$(dirname "$0")/.." && pwd)/logs"
B="$(cd "$(dirname "$0")/.." && pwd)"
win(){ case $1 in ngpc) xdotool search --class mednafen | head -1;; amiga) xdotool search --name FS-UAE | head -1;; esac; }
case $1 in
  start-x) pgrep -x Xvfb >/dev/null || nohup Xvfb :99 -screen 0 1280x960x24 >$B/logs/xvfb.log 2>&1 & ;;
  ngpc)  pkill -x mednafen; nohup /usr/games/mednafen -sound 0 -video.driver sdl -ngp.xscale 4 -ngp.yscale 4 "$2" >$B/logs/mednafen.log 2>&1 & ;;
  amiga) pkill -x fs-uae; nohup fs-uae "${2:-$B/amiga/cfg/a500.fs-uae}" --hard_drive_0="$B/amiga/hd" --screenshots_output_dir="$B/shots" >$B/logs/fs-uae.log 2>&1 & ;;
  shot)  w=$(win $2); xdotool windowraise "$w"; sleep 0.3; import -window "$w" "$3" ;;
  key)   w=$(win $2); shift 2; xdotool windowfocus --sync "$w" key --delay 80 "$@" ;;
  stop)  pkill -x mednafen; pkill -x fs-uae; pkill -x Xvfb ;;
esac
