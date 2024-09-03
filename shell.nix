{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    uthash
    libev
    dnsperf
  ];
  shellHook = ''
    export C_INCLUDE_PATH=${pkgs.lib.makeSearchPathOutput "dev" "include" [ pkgs.libev pkgs.uthash pkgs.dnsperf ]}
  '';
}
