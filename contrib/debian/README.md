
Debian
====================
This directory contains files used to package bwscoind/bwscoin-qt
for Debian-based Linux systems. If you compile bwscoind/bwscoin-qt yourself, there are some useful files here.

## bwscoin: URI support ##


bwscoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install bwscoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your bwscoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/bwscoin128.png` to `/usr/share/pixmaps`

bwscoin-qt.protocol (KDE)

