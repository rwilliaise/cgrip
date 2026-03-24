{
  description = "AmbientCG asset ripper.";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-25.11";

  outputs = { self, nixpkgs }:
    let

      lastModifiedDate = self.lastModifiedDate or self.lastModified or "19700101";
      version = builtins.substring 0 8 lastModifiedDate;

      supportedSystems = [ "x86_64-linux" "x86_64-darwin" "aarch64-linux" "aarch64-darwin" ];

      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; overlays = [ self.overlays.default ]; });
    in

    {
      overlays.default = final: prev: {
        cgrip = with final; stdenv.mkDerivation {
          pname = "cgrip";
          inherit version;
          src = ./.;
          nativeBuildInputs = [
            pkg-config
            autoreconfHook
          ];
          buildInputs = [
            libarchive
            curlFull
          ];
        };

      };

      packages = forAllSystems (system:
        rec {
          inherit (nixpkgsFor.${system}) cgrip;
          default = cgrip;
        });

    };
}
