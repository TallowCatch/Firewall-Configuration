# Firewall Configuration
A C code repository for a firewall management system, encompassing server-client interactions, rule processing, and activity logging. 

A firewall rule is specified in the form of

<_IPAddresses_> <_Ports_>

where <_IPAddresses_> is either a single IP address, which has the form xxx.xxx.xxx.xxx, where xxx is a number between 0 and 255, or <_IPAddress1_>-<_IPAddress2_>. Similarly, <_Ports_>is either a single port number, which is a number between
0 and 65535, or <_Ports1_>-<_Ports2_>, where <_Ports1_> and <_Ports2_> are ports and <_Ports1_> is smaller than <_Ports2_>.

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

