{ pkgs }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    glibc.static
    clang
    gnumake
    clang-tools
  ];
}
