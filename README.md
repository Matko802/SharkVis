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

`make` required for building

```sh
git clone https://github.com/Matko802/SharkVis.git
cd SharkVis
make deps      
make           
sudo make install
```

Prefer not to touch your system? On NixOS or any distro with Nix installed:

```sh
nix run github:Matko802/SharkVis
```

To install somewhere else instead of `/usr/local`:

```sh
make PREFIX=$HOME/.local install
```

### Run it

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
