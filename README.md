# Plug

Software for Fender Mustang Amps. 

This repository is a fork of [Offa's rework](https://github.com/offa/plug) of [piorekf's Plug](https://bitbucket.org/piorekf/plug/).

## Goals:

- Add support for Mustang V2 I/II effects and amps using [snhirsch /
mustang-midi-bridge ](https://github.com/snhirsch/mustang-midi-bridge) as a reference.

- Have it running correctly on a raspberry pi

- Provide a more compact interface to fit on a cheap LCD

## Current status:

- Presets with V2-only Effects and Amps can be created, loaded and saved from both amplifier and files. It's now time for some serious bug hunting...

- Working fine on a raspberry pi 3, but I had to increment the timeout for USB transfers. 

- The UI has been changed to fit an 800x480 touch screen. Using [devilspie](https://wiki.gnome.org/action/show/Projects/DevilsPie?action=show&redirect=DevilsPie). 
    - The Main window and the Amp are opened on workspace 1
    - effect windows 1 and 2 on workspace 2, 
    - effect windows 3 and 4 on workspace 3

- It is now possible to toggle the tuner from the application (Ctrl-t)

- Effect toggling is now performed using the dedicated message instead of setting a null effect.

Some major work will be needed to read from the amp on a separate thread as in [snhirsch /
mustang-midi-bridge ](https://github.com/snhirsch/mustang-midi-bridge). It would allow to use the amp dials and buttons, and it would redice the chance to have unflushed messages from the amp.

## Building

Building and Testing is done through CMake:

```
mkdir build && cd build
cmake ..
make
make unittest
```


## Installation

CMake will install the application and *udev* rule (`50-mustang.rules`) using:

```
make install
```

The *udev* rule allows the USB access without *root* for the users of the `plugdev` group.

If you want to run on a small screen (e.g. on a Raspberry pi) edit the stuff in the *extra/\_config* folder and copy it  on $HOME/.config 

## Documentation

Visit the [Plug Website](https://bitbucket.org/piorekf/plug/) for documentation and technical details.


## Credits

Thanks to [piorekf and all Plug contributors](https://bitbucket.org/piorekf/plug/).
Thanks to [Offa's rework](https://github.com/offa/plug) 

## License

**GNU General Public License (GPL)**

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.



