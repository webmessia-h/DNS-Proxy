#!/usr/bin/env bash

check_dnsperf() {
    if ! command -v dnsperf &> /dev/null; then
        echo "dnsperf is not installed."
        echo "Please install dnsperf using your package manager."
        echo "On Debian/Ubuntu: sudo apt-get install dnsperf"
        echo "On Red Hat/CentOS: sudo yum install dnsperf"
        echo "On Fedora: sudo dnf install dnsperf"
        echo "On Arch Linux: sudo pacman -S dnsperf"
        exit 1
    else
        return
    fi
}

check_dnsperf

# sends queries from queries.txt to standart address for 60 seconds
# acting like 4 clients and using 4 threads
# verbose, shows warnings
dnsperf -s 127.0.0.1 -p 53 -d queries.txt -l 60 -Q 20000 -T 4 -c 4 -b 40 -q 1000 -v -W
