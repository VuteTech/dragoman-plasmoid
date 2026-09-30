# Offline Translator for Plasma

A Plasma 6 panel and desktop widget that translates text without an
internet connection. It is a front end to
[Dragomand](https://dragomand.l10n-bg.dev), the per-user service that
runs Mozilla's Firefox translation models on your own computer, and talks
to it through [libdragoman-qt](https://github.com/VuteTech/libdragoman-qt).

- Click the icon in the panel, choose the two languages and type or paste
  text: it is translated after a short pause, or with Ctrl+Return when
  translating while typing is off. Empty lines and the layout of the text
  are kept.
- Before translating, the widget checks which of the two languages the
  text is in. When it is in the target language, the languages are
  swapped, and a message says so and offers to swap them back.
- A language pair that is not installed yet is never downloaded behind
  your back: the widget says so and offers an Install button, with
  progress and a way to cancel.
- The translation can be copied, or opened in
  [Krakoman](https://github.com/VuteTech/krakoman), the full translation
  application, when it is installed.
- The last languages and the "Translate while typing" switch are
  remembered; the settings page sets them too, and can turn off the
  automatic swap.

The widget does not start the translation service by itself: nothing is
asked of it before you open the popup.

## Build

Needs CMake 3.24, extra-cmake-modules, Qt 6.8, KDE Frameworks 6.13
(Config, CoreAddons, I18n), libplasma 6.3 and libdragoman-qt:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build
sudo cmake --install build
```

To build against an uninstalled libdragoman-qt, pass
`-DDragomanQt_DIR=<libdragoman-qt build>/buildtree`.

The autotests run the translation logic against the fake daemon of
libdragoman-qt on a private bus; they never touch your session bus.
`cmake --build build --target clang-format` applies the KDE code style.

## Try it without installing

`plasmoidviewer` (from plasma-sdk) loads the widget straight from the
build directory:

```sh
QT_PLUGIN_PATH=$PWD/build/bin plasmoidviewer -a dev.l10n_bg.dragomand.translator
# as in a panel:
QT_PLUGIN_PATH=$PWD/build/bin plasmoidviewer -a dev.l10n_bg.dragomand.translator \
    -f horizontal -l bottomedge
```

After `cmake --install`, add "Offline Translator" to a panel or the
desktop from the widget explorer. Plasma loads widgets when it starts, so
log out and in again after installing a new version.

## Translations

`scripts/update-translations.sh` extracts the strings into
`po/plasma_applet_dev.l10n_bg.dragomand.translator.pot` and merges them
into every `po/<language>/plasma_applet_dev.l10n_bg.dragomand.translator.po`.

## License

GPL-3.0-or-later. The project follows the [REUSE](https://reuse.software)
specification.
