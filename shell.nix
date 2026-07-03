{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
  buildInputs = with pkgs; [
    glibc
    glibc.static
    clang
    gnumake
    clang-tools
  ];
}
