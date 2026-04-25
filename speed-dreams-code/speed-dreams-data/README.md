# Speed Dreams

![Speed Dreams logo](/data/data/img/sd-logo.png)

Speed Dreams is a free and open source motorsport simulator. Originally
a fork of the [TORCS](https://torcs.sourceforge.net/) project,
it has evolved into a higher level of maturity, featuring realistic physics
with tens of high-quality cars and tracks to choose from.

> **This repository only contains the base assets.**
> **End users are expected to download the**
> **[pre-built packages](#pre-built-packages)**.
>
> Developers interested in the engine source code should refer to
> https://forge.a-lec.org/speed-dreams/speed-dreams-code/

![In-game screenshot](./doc/readme/collage.jpg)

## Pre-built packages

Speed Dreams binaries are available from the
[official website](https://www.speed-dreams.net/en/downloads).

## Building from source

> Note that no build step is required, since this is a pure CMake project
> i.e., no programming languages are defined.

```
cmake -B build -DCMAKE_INSTALL_PREFIX=<install-dir>
cmake --install build/
```

### Dependencies

#### Debian/Ubuntu

```
sudo apt install git cmake make
```

## Translations

Speed Dreams can be translated to other languages without any changes to its
game engine, or programming skills required.

Read [the relevant documentation](./doc/TRANSLATING.md) for further reference.

## License

By default, Speed Dreams code is licensed under the GPLv2-or-later license,
as specified by the [`LICENSE`](./LICENSE) file, whereas non-functional data
is licensed under the [Free Art License](http://artlibre.org/) by default.

However, some sections of the code and some other assets are distributed under
various free (as in freedom) licenses. Please read their license files
located in their respective directories for further reference.

## Trademark disclaimer

Microsoft Windows is a registered trademark of Microsoft Corporation.

Ubuntu is a registered trademark of Canonical Ltd.
