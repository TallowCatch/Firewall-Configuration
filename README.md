# Firewall Configuration
A C code repository for a firewall management system, encompassing server-client interactions, rule processing, and activity logging. A firewall rule is specified in the form of

<IPAddresses> <ports>

where <IPAddresses> is either a single IP address, which has the form xxx.xxx.xxx.xxx, where xxx is a number between 0 and 255, or <IPAddress1>-<IPAddress2>. Similarly, <ports> is either a single port number, which is a number between
0 and 65535, or <port1>-<port2>, where <port1> and <port2> are ports and <port1> is smaller than <port2>.

The code comes with a python3 GUI to avoid rigorous yet error-prone input.

## Prerequisites

1. Python (version 3.12)
2. GCC
3. Pip (package manager)
  
## Build

* Download the source tree, change to the source directory
* Run 'make' for command-line utility
* To compile GUI, run 'make GUI'

## Installation

