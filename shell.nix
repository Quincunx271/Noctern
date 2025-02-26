{ pkgs ? import <nixpkgs> { } }:
with pkgs;

mkShell {
  buildInputs = [
    cmake
    gcc13
    ninja
    llvmPackages_19.clang-tools
    llvmPackages_19.libcxxClang

    spdlog
    fmt
    boost
    flex
    catch2
  ];
}
