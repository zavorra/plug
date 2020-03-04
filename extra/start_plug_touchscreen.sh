PLUGDIR="/home/giorgio/workspace/Mustang/plug.git/"

DISPLAY=:0
Xephyr -screen 800x480 :1 &
sleep 1
DISPLAY=:1
#xfwm4 -- :1 &
#openbox &
#awesome &
lxsession &
sleep 1
#xfce4-panel &
devilspie -d $PLUGDIR/plug.ds &
sleep 1
rxvt -title "log" -g 160x25+0+440 -e $PLUGDIR/build/src/plug &
sleep 1
