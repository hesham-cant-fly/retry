{ pkgs }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    glibc
    glibc.static
    clang
    gnumake
    clang-tools
  ];
}
