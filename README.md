# OpenMG 

OpenMG is a GTK4 + Libadwaita manga reader written in C which uses `readmng` as its backend.

## Demostration

![Demostration vídeo of the manga reader.](https://gitea.sergiotarxz.freemyip.com/sergiotarxz/mangareader/raw/branch/main/demostration.gif)

## Installing the app.

First fine tune the options in `me.sergiotarxz.openmg.json` for
meson you want to have, for example preview images, complete list is
on `meson_options.txt`

```shell
flatpak --user remote-add --if-not-exists gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo 
flatpak install org.gnome.Sdk//master
flatpak install org.gnome.Platform//master
flatpak-builder --install --user build me.sergiotarxz.openmg.yml me.sergiotarxz.openmg

```

## Running the app

```shell
flatpak run me.sergiotarxz.openmg
```

## Donations welcome:

btc: `bc1q0apxdedrm5vjn3zr0hxswnruk2x2uecwqrusmj`
