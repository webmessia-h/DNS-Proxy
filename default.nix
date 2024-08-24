{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.libev
    pkgs.uthash
    pkgs.pkg-config
  ];
}

