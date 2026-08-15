# SharkVis

Linux only audio visualizer made in C 

Inspired by [cava](https://github.com/karlstav/cava) and [cli-visualizer](https://github.com/PosixAlchemist/cli-visualizer)

## Features

- PulseAudio / PipeWire support
- smoothness adjust, noise reduction
- autosensitivity, manual sensitivity control, adjustable cutoff frequencies
- TUI settings
- color customization

## Building

SharkVis is a plain C11 program built with `make` and only one runtime
dependency (PulseAudio / PipeWire audio). It works on any Linux distro.

### 1. Install the build dependencies

You need a C11 compiler (`gcc` or `clang`), `make`, `pkg-config`, and the
PulseAudio development headers (`libpulse-simple`). Install them with your
distro's package manager:

| Distro family      | Command                                                                 |
| ------------------ | ----------------------------------------------------------------------- |
| Debian / Ubuntu    | `sudo apt install build-essential pkg-config libpulse-dev`              |
| Arch / Manjaro     | `sudo pacman -S base-devel libpulse`                                    |
| Fedora             | `sudo dnf install gcc make pkgconf-pkg-config pulseaudio-libs-devel`    |
| openSUSE           | `sudo zypper install gcc make pkg-config libpulse-devel`                |
| Void               | `sudo xbps-install base-devel pkg-config pulseaudio-devel`              |
| Alpine             | `sudo apk add build-base pkgconfig libpulse-dev`                        |
| Gentoo             | `sudo emerge -av sys-devel/gcc sys-devel/make sys-devel/pkgconf media-libs/libpulse` |

On PipeWire systems (the default on Fedora, openSUSE, and Arch since 2023)
the PulseAudio compatibility layer is provided by the same `libpulse` /
`libpulse-dev` packages, so nothing extra is needed.

You can verify the headers are present before building:

```sh
pkg-config --exists libpulse-simple && echo "ok"
```

### 2. Build and install

```sh
make
sudo make install        # installs to /usr/local/bin/sharkvis
```

To install somewhere else, override the prefix:

```sh
make PREFIX=$HOME/.local install
```

For packaging (rpm/deb/arch), install into a staging directory:

```sh
make DESTDIR=$pkgdir PREFIX=/usr install
```

### 3. Run it

```sh
sharkvis
```

To use the default PulseAudio sink (what's currently playing), just launch it.
It also works as a monitor of your microphone / line-in source; see the config
below to change the input. Audio must be playing on the system for the
visualizer to move.

## Usage

```sh
sharkvis
sharkvis -p config.conf
sharkvis -h
```

Keys:

| Key               | Action                          |
| ----------------- | ------------------------------- |
| `g`               | open settings                   |
| `q` / `Ctrl-C`    | quit                            |

The config file is looked up in `$SHARKVIS_CONFIG`, then
`~/.config/SharkVis/config`, then `./config`. Settings changed in the panel
are saved automatically when you close the panel or quit. Example:

```ini
[general]
bars = 40            ; 0 = auto fit to terminal width
bar_width = 2
bar_spacing = 1
framerate = 60
sensitivity = 100
autosens = 1
lower_cutoff_freq = 50
higher_cutoff_freq = 8000

[smoothing]
noise_reduction = 0.2

[input]
source = auto
sample_rate = 48000
channels = 2

[color]
gradient = 0
```

## Nix flakes

SharkVis ships with its own flake, so you can pull it straight from GitHub.

### As a flake input

```nix
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    sharkvis = {
      url = "github:Matko802/SharkVis";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { nixpkgs, sharkvis, ... }: {
    packages.x86_64-linux.default = sharkvis.packages.x86_64-linux.default;
  };
}
```

### As an overlay

The flake also exposes `overlays.default`, so you can enable it with
`nixpkgs.overlays = [ sharkvis.overlays.default ];` and get `pkgs.sharkvis`.

A full NixOS example that pulls the flake in as both an overlay and a package:

```nix
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    sharkvis = {
      url = "github:Matko802/SharkVis";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    { nixpkgs, sharkvis, ... }:
    let
      system = "x86_64-linux";
    in
    {
      nixosConfigurations.myhost = nixpkgs.lib.nixosSystem {
        inherit system;
        modules = [
          {
            nixpkgs.overlays = [ sharkvis.overlays.default ];
            environment.systemPackages = [ sharkvis.packages.${system}.default ];
          }
        ];
      };
    };
}
```

### Standalone build from source

```sh
nix build github:Matko802/SharkVis
nix run github:Matko802/SharkVis
```

### Development

```sh
nix develop github:Matko802/SharkVis  
```

## License

This project is currently unlicensed. Contact the author if you wish to
redistribute or reuse it.
