{
  description = "A programming language experiment";

  inputs = {
    nixpkgs.url = "nixpkgs/nixos-24.11";
  };

  outputs =
    {
      self,
      nixpkgs,
      ...
    }@inputs:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forEachSystem =
        f:
        nixpkgs.lib.genAttrs supportedSystems (
          system:
          f {
            pkgs = import nixpkgs { inherit system; };
          }
        );
      libraries =
        pkgs: with pkgs; [
          fmt
          catch2
        ];
    in
    {
      devShells = forEachSystem (
        { pkgs }:
        {
          default = pkgs.mkShell {
            packages =
              with pkgs;
              [
                cmake
                gcc13
                ninja
                llvmPackages_19.clang-tools
                clang_19

                ccache

                gdb
              ]
              ++ libraries pkgs;
          };
        }
      );
    };
}
