# proct

## Description

`proct` reports the running time of a Linux process by PID or name, with optional human-readable output.

## Build

```sh
make
```

## Installation

```sh
sudo make install
```

Default install path is `/usr/local/bin`. Override with `make install PREFIX=/custom/path`.

## Usage

```sh
proct [-h] -p <pid>
proct [-h] -n <process_name>
```

- `-p <pid>`: Specify the process ID.
- `-n <process_name>`: Specify the process name.
- `-h`: Output time as hours, minutes, and seconds.

## Examples

Show uptime in seconds for PID 1234:
```sh
proct -p 1234
```

Show uptime in hours/minutes/seconds for process named "bash":
```sh
proct -h -n bash
```

## Uninstall

```sh
sudo make uninstall
```

Creators: João & Raidboy
