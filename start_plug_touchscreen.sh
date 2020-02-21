PLUGDIR="/home/giorgio/workspace/Mustang/plug.git/"

DISPLAY=:0
Xephyr -screen 800x480 :1 &
sleep 1
DISPLAY=:1
xfwm4 -- :1 &
sleep 1
xfce4-panel &
sleep 1
rxvt -title "log" -g 160x25+0+440 -e $PLUGDIR/build/src/plug &
sleep 1
echo "1"
MAIN_WIN=$(xdotool search --name --onlyvisible PLUG )
FX1_WIN=$(xdotool search --name FX1)
FX2_WIN=$(xdotool search --name FX2)
FX3_WIN=$(xdotool search --name FX3)
FX4_WIN=$(xdotool search --name FX4)
AMP_WIN=$(xdotool search --name Amplif)

