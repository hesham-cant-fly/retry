{
  description = "retry - Re-execute a command until it succeeds";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "retry";
          version = "0.1.0";
          src = ./.;
          buildPhase = ''
            make release
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp retry $out/bin/
          '';
          meta = {
            description = "Re-execute a command until it succeeds";
            license = pkgs.lib.licenses.mit;
            platforms = pkgs.lib.platforms.linux;
          };
        };
        devShells.default = import ./shell.nix { inherit pkgs; };
      }
    );
}
