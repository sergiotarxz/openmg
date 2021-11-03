# mangareader

## Installing the app.

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
