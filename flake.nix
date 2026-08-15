{
  description = "SHRKVis - terminal audio spectrum analyzer";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f (nixpkgs.legacyPackages.${system}));

      shrkvis =
        { pkgs }:
        pkgs.stdenv.mkDerivation {
          pname = "SHRKVis";
          version = "0.1.0";
          src = ./.;
          nativeBuildInputs = [ pkgs.pkg-config ];
          buildInputs = [ pkgs.libpulseaudio ];
          makeFlags = [ "PREFIX=$(out)" ];
          meta = {
            mainProgram = "SHRKVis";
            description = "Terminal audio spectrum analyzer";
            homepage = "https://github.com/Matko802/SHRKVis";
            platforms = pkgs.lib.platforms.linux;
          };
        };

      overlay = final: _prev: {
        SHRKVis = shrkvis { pkgs = final; };
      };
    in
    {
      packages = forAllSystems (pkgs: {
        default = shrkvis { inherit pkgs; };
        SHRKVis = shrkvis { inherit pkgs; };
      });

      overlays.default = overlay;
    };
}
