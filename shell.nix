{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    uthash
    libev
    dnsperf
    dig
    valgrind
    tcpdump
  ];
  shellHook = ''
    export C_INCLUDE_PATH=${pkgs.lib.makeSearchPathOutput "dev" "include" [ pkgs.libev pkgs.uthash pkgs.dnsperf pkgs.dig pkgs.valgrind pkgs.tcpdump ]}
    # Add alias for running the test script with queries.txt as the argument
    alias test="bash ./assets/tests/test.sh ./assets/tests/queries.txt"
  '';
}
