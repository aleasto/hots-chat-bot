#!/bin/bash
Xvfb $DISPLAY -screen 0 1024x768x16 &
sleep 1

fluxbox &
sleep 1

if [ -n "$VNCPASSWD" ]; then
    x11vnc -storepasswd "$VNCPASSWD" /etc/x11vnc.pass
    x11vnc -display $DISPLAY -bg -forever -rfbauth /etc/x11vnc.pass -quiet -listen 0.0.0.0 -xkb &
else
    x11vnc -display $DISPLAY -bg -forever -nopw -quiet -listen 0.0.0.0 -xkb &
fi

sleep 2
echo "##############################################"
echo "Run 'vncviewer localhost:5900' to connect"
echo "##############################################"
sleep 4

"$@"

while pgrep wineserver; do
    sleep 2
done

killall Xvfb
rm -rf /tmp/.X*
