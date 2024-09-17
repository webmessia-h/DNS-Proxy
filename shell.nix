{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    uthash
    libev
    dnsperf
    valgrind
    dig
  ];
  shellHook = ''
    export C_INCLUDE_PATH=${pkgs.lib.makeSearchPathOutput "dev" "include" 
      [ pkgs.libev pkgs.uthash pkgs.dnsperf pkgs.valgrind pkgs.dig]}
    # Add alias for running the test script with queries.txt as the argument
    alias test="bash ./assets/tests/test.sh ./assets/tests/queries.txt"
  '';
}
