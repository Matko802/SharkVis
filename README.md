# SharkVis

Audio visualizer, inspired by
[cava](https://github.com/karlstav/cava). Written in C, with more features without any "bloat"

## Features

- PulseAudio / PipeWire support
- smoothness adjust, noise reduction
- autosensitivity, manual sensitivity control, adjustable cutoff frequencies
- TUI settings
- color customization

## Building

Dependencies: a C11 compiler, `make`, `pkg-config`, and the PulseAudio
development headers (`libpulse-simple`).

```sh
make
sudo make install   
```
You can also override the prefix:

```sh
make PREFIX=$HOME/.local install
```

## Usage

```sh
sharkvis                
sharkvis -p config.conf 
sharkvis -h             
```

Keys:

| Key               | Action                          |
| ----------------- | ------------------------------- |
| `g`               | open settings panel             |
| `q` / `Ctrl-C`    | quit                            |

The config file is looked up in `$SHRKVIS_CONFIG`, then
`~/.config/SHRKVis/config`, then `./config`. Example:

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

SHRKVis ships with its own flake, so you can pull it straight from GitHub.

### As a flake input

```nix
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    shrkvis = {
      url = "github:Matko802/SHRKVis";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { nixpkgs, shrkvis, ... }: {
    packages.x86_64-linux.default = shrkvis.packages.x86_64-linux.default;
  };
}
```

### As an overlay

The flake also exposes `overlays.default`, so you can enable it with
`nixpkgs.overlays = [ shrkvis.overlays.default ];` and get `pkgs.SHRKVis`.

A full NixOS example that pulls the flake in as both an overlay and a package:

```nix
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    shrkvis = {
      url = "github:Matko802/SHRKVis";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    { nixpkgs, shrkvis, ... }:
    let
      system = "x86_64-linux";
    in
    {
      nixosConfigurations.myhost = nixpkgs.lib.nixosSystem {
        inherit system;
        modules = [
          {
            nixpkgs.overlays = [ shrkvis.overlays.default ];
            environment.systemPackages = [ shrkvis.packages.${system}.default ];
          }
        ];
      };
    };
}
```

### Standalone build from source

```sh
nix build github:Matko802/SHRKVis
nix run github:Matko802/SHRKVis
```

### Development

```sh
nix develop github:Matko802/SHRKVis   # shell with build dependencies
```

## License

This project is currently unlicensed. Contact the author if you wish to
redistribute or reuse it.
