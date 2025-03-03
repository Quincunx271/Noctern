{
  description = "A programming language experiment";

  inputs = {
    nixpkgs.url = "nixpkgs/nixos-24.11";

    utils.url = "github:numtide/flake-utils";
  };

  outputs =
    { self, nixpkgs, ... }@inputs:
    inputs.utils.lib.eachSystem
      [
        "x86_64-linux"
        "i686-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ]
      (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;

            overlays = [ ];
          };
        in
        {
          devShells.default = pkgs.mkShell rec {
            name = "noctern";

            packages = with pkgs; [
              cmake
              gcc13
              ninja
              llvmPackages_19.clang-tools
              llvmPackages_19.libcxxClang
              gdb

              spdlog
              fmt
              boost
              flex
              catch2
            ];
          };

          packages.default = pkgs.callPackage ./default.nix { };
        }
      );
}
