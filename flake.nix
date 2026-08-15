{
  description = "SharkVis - terminal audio spectrum analyzer";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f (nixpkgs.legacyPackages.${system}));

      sharkvis =
        { pkgs }:
        pkgs.stdenv.mkDerivation {
          pname = "SharkVis";
          version = "0.1.0";
          src = ./.;
          nativeBuildInputs = [ pkgs.pkg-config ];
          buildInputs = [ pkgs.libpulseaudio ];
          makeFlags = [ "PREFIX=$(out)" ];
          meta = {
            mainProgram = "SharkVis";
            description = "Terminal audio spectrum analyzer";
            homepage = "https://github.com/Matko802/SharkVis";
            platforms = pkgs.lib.platforms.linux;
          };
        };

      overlay = final: _prev: {
        SharkVis = sharkvis { pkgs = final; };
      };
    in
    {
      packages = forAllSystems (pkgs: {
        default = sharkvis { inherit pkgs; };
        SharkVis = sharkvis { inherit pkgs; };
      });

      overlays.default = overlay;

      devShells = forAllSystems (pkgs:
        pkgs.mkShell {
          buildInputs = [ pkgs.gcc pkgs.pkg-config pkgs.libpulseaudio ];
        });
    };
}
