{ pkgs ? import <nixpkgs> { } }:

let
  unstable = import (fetchTarball https://nixos.org/channels/nixos-unstable/nixexprs.tar.xz) { };  # Specify the desired GCC version
  gcc = unstable.gcc14;
  # Specify the desired Clang version
  clang = unstable.clang_19;
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    clang
    uthash
    libev
    dnsperf
    valgrind
    dig
  ];
  shellHook = ''
    export CC=${gcc}/bin/gcc
    export CXX=${gcc}/bin/g++
    export C_INCLUDE_PATH=${pkgs.lib.makeSearchPathOutput "dev" "include"
      [ pkgs.libev pkgs.uthash pkgs.dnsperf pkgs.valgrind pkgs.dig]}:${gcc}/lib/gcc/${pkgs.stdenv.targetPlatform.config}/${gcc.version}/include
    export CPLUS_INCLUDE_PATH=$C_INCLUDE_PATH
    export PATH="${gcc}/bin:${clang}/bin:$PATH"
    # Add alias for running the test script with queries.txt as the argument
    alias test="bash ./assets/tests/test.sh ./assets/tests/queries.txt"
    echo "GCC version: $(gcc --version | head -n1)"
    echo "Clang version: $(clang --version | head -n1)"
  '';
}
