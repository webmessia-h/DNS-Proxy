#!/usr/bin/env sh

TARGET_IP="127.0.0.1"  # Replace with your proxy's IP
TARGET_PORT="53"       # Replace with your proxy's port
QUERIES=1000           # Number of queries to send
DOMAINS=("microsoft.com" "google.com" "github.com")

for ((i=1; i<=QUERIES; i++))
do
    for domain in "${DOMAINS[@]}"
    do
        dig @$TARGET_IP -p $TARGET_PORT $domain > /dev/null 2>&1 &
    done
    
    if (( i % 100 == 0 )); then
        echo "Sent $i queries"
    fi
done

wait
echo "Test completed. Sent $QUERIES queries for each domain."
