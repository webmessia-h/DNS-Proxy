# Task #1: DNS Proxy Server

## Description

Write a DNS proxy server with a domains blacklist feature to filter unwanted host names from being resolved.

## Functional Requirements

1. **Configuration File**:

   - The proxy server reads its parameters during startup from the configuration file.
   - The configuration file contains the following parameters:
     - IP address of upstream DNS server.
     - List of domain names to filter resolving ("blacklist").
     - Type of DNS proxy server's response for blacklisted domains (not found, refused, resolve to a pre-configured IP address).

2. **UDP Protocol**:

   - DNS proxy server uses UDP protocol for processing client's requests and for interaction with upstream DNS server.

3. **Request Handling**:
   - If a domain name from a client's request is not found in the blacklist, then the proxy server forwards the request to the upstream DNS server, waits for a response, and sends it back to the client.
   - If a domain name from a client's request is found in the blacklist, then the proxy server responds with a response defined in the configuration file.

## Non-Functional Requirements

1. **Programming Language**:
   - C.
2. **Third-Party Libraries**:

   - It is allowed to use third-party libraries or code. If you use one, then you must comply with its license terms and conditions.
   - The only exception is that it is not allowed to use third-party libraries or code that implement DNS parsing and constructing DNS packets.

3. **Discretionary Requirements**:
   - All other requirements and restrictions are up to your discretion.
