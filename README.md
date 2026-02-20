spacefn-evdev
=============

This is a little tool to implement
[the SpaceFn keyboard layout](https://geekhack.org/index.php?topic=51069.0)
on Linux.

I wanted to try SpaceFn on my laptop, obviously with a built-in keyboard.
The only previous Linux implementation I could find
[requires recompiling the Xorg input driver](http://www.ljosa.com/~ljosa/software/spacefn-xorg/),
which is an impressive effort but is tricky to compile and means restarting my X server every time I want to make a change.


## Requirements


- libevdev
    and its headers or -dev packages on some systems
- uinput
    `/dev/uinput` must be present and you must have permission to read and write it

You also need permission to read `/dev/input/eventXX`.

On my system all the requisite permissions are granted by making myself a member of the `input` group.
You can also just run the program as root.

## Building

```
./build.sh
```

The compiled binary will be at `./spacefn` (symlink to `build/spacefn`).

## Installation

```
sudo ./install.sh
```

This will:
1. Install udev rules for uinput and input device access
2. Copy config examples to `~/.config/spacefn/`
3. Show instructions for adding your user to the `input` group

After installation, add your user to the `input` group and log out/in:
```
sudo usermod -aG input $USER
```

## Running

```
./spacefn ~/.config/spacefn/default.cfg
```

Find your keyboard device:
```
ls /dev/input/by-id/
```

## Customising

Edit config files in `configs/` directory.
