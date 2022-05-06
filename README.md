# OpenMG 

OpenMG is a GTK4 + Libadwaita manga reader written in C which uses `readmng` as its backend.

[![Please do not theme this app](https://stopthemingmy.app/badge.svg)](https://stopthemingmy.app)

## Demostration

![Demostration vídeo of the manga reader.](https://gitea.sergiotarxz.freemyip.com/sergiotarxz/mangareader/raw/branch/main/demostration.gif)

## Installing the app.

These are the installation methods supported currently.

### Flatpak

Download from https://gitea.sergiotarxz.freemyip.com/sergiotarxz/mangareader/releases the latest `openmg-x86_64-(version).flatpak` and run:

```shell
flatpak install openmg-x86_64-(version).flatpak
```

Beware that not being in Flathub yet you will have to come here again
to get updates.

### Gentoo

```shell
sudo eselect repository enable sergiotarxz
echo 'app-misc/openmg ~amd64' | sudo tee -a /etc/portage/package.accept_keywords/zz-autounmask
sudo emerge -a openmg --autounmask
```

If the installation ask you for a package masked for ~amd64 you can run
`sudo etc-update`, upgrade the `package.accept_keywords` config file
and try again the latest command of the installation instructions.


## Build from source

### Flatpak

First fine tune the options in `me.sergiotarxz.openmg.json` for
meson you want to have, for example preview images, complete list is
on `meson_options.txt`

```shell
flatpak --user remote-add --if-not-exists gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo 
flatpak --user install org.gnome.Sdk//master
flatpak --user install org.gnome.Platform//master
flatpak-builder --install --user build me.sergiotarxz.openmg.json me.sergiotarxz.openmg

```

### Native

```shell
meson build
meson compile -C build
sudo meson install -C build
```

## Running the app

If using flatpak:

```shell
flatpak run me.sergiotarxz.openmg
```

If native installated:

```shell
openmg
```

## Donations welcome:

btc: `bc1q0apxdedrm5vjn3zr0hxswnruk2x2uecwqrusmj`
