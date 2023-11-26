# Firewall Configuration

A C code repository for a firewall management system, encompassing server-client interactions, rule processing, and activity logging.

A firewall rule is specified in the form of:

`<IPAddresses> <Ports>`

where `<IPAddresses>` is either a single IP address of the form `xxx.xxx.xxx.xxx` (where `xxx` is a number between 0 and 255), or `<IPAddress1>-<IPAddress2>`. Similarly, `<Ports>` is either a single port number (a number between 0 and 65535), or `<Port1>-<Port2>`, where `<Port1>` and `<Port2>` are ports and `<Port1>` is smaller than `<Port2>`.

The code comes with a Python 3 GUI to facilitate user-friendly and error-minimized input, and a separate CLI client for direct command line interactions.

## Prerequisites

1. **Python**: Version 3.12.0 or higher.
2. **GCC**: For compiling the C server and client.
3. **Pip**: Python package manager, for installing Python dependencies.

Additional Python Packages: `tkinter`, `Pillow`, `customtkinter`, `requests`. These can be installed using `pip`.

## Build and Running

* Download the source tree and change to the source directory.
    ```bash
    git clone https://github.com/TallowCatch/Firewall-Configuration.git
    cd Firewall-Configuration
    ```
* To simultaneously start the server and the GUI, run:
    ```bash
    make run-all
    ```
    This will launch the server in the background and then open the GUI. Closing the GUI will automatically shut down the server.
  
  *  To only compile the server and client for CLI use, run:
    ```bash
    make
    ```

## CLI Client Usage

The client can be used for direct command-line interactions with the firewall server. The client supports the following commands:

* Adding a rule:
    ```bash
    ./client <serverHost> <serverPort> A <rule>
    ```
* Checking an IP address and port:
    ```bash
    ./client <serverHost> <serverPort> C <IPAddress> <port>
    ```
* Deleting a rule:
    ```bash
    ./client <serverHost> <serverPort> D <rule>
    ```
* Showing the current firewall rules:
    ```bash
    ./client <serverHost> <serverPort> L
    ```
* Any other usage will return an error message.

## Cleanup

* Use `make clean` to remove the compiled server and client files.
