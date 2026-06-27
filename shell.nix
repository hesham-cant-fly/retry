{ pkgs }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    clang
    gnumake
    clang-tools
  ];
}
