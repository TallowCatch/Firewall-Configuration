# Firewall Configuration

A C code repository for a firewall management system, encompassing server-client interactions, rule processing, and activity logging. 

A firewall rule is specified in the form of:

`<IPAddresses> <Ports>`

where `<IPAddresses>` is either a single IP address of the form `xxx.xxx.xxx.xxx` (where `xxx` is a number between 0 and 255), or `<IPAddress1>-<IPAddress2>`. Similarly, `<Ports>` is either a single port number (a number between 0 and 65535), or `<Port1>-<Port2>`, where `<Port1>` and `<Port2>` are ports and `<Port1>` is smaller than `<Port2>`.

The code comes with a Python 3 GUI to facilitate user-friendly and error-minimized input.

## Prerequisites

1. **Python**: Version 3.12 or higher.
2. **GCC**: For compiling the C server.
3. **Pip**: Python package manager, for installing Python dependencies.

Additional Python Packages: `tkinter`, `Pillow`, `customtkinter`, `requests`. These can be installed using `pip`.

## Build and Running

* Download the source tree and change to the source directory.
    ```bash
    git clone https://your-repository-url.git
    cd your-repository-directory
    ```
* To compile the server, run:
    ```bash
    make
    ```
* To start the GUI, run:
    ```bash
    make run-gui
    ```

## Cleanup

* Use `make clean` to remove the compiled server files.

## Running the Application

* After building, start the server by executing `./server`.
* Run the GUI using the `make run-gui` command.
