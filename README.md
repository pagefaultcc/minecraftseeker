# ServerSeeker

ServerSeeker is a multi-threaded scanner for Minecraft servers. It connects to a PostgreSQL database, reads a list of target IPs from a file, and pings each server to check whether any players are currently online. When players are found, the results are logged to the database along with a timestamp.

The main use case for this tool is **sniping** — finding servers with active players as quickly as possible.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Configuration](#configuration)
- [Usage](#usage)
- [How It Works](#how-it-works)
- [API](#api)
- [Roadmap / TODO](#roadmap--todo)
- [Author](#author)

---

## Overview

Given a text file containing a list of server IPs, ServerSeeker will:

1. Load the IP list from the provided file.
2. Spin up a configurable number of worker threads.
3. Ping each server using the Minecraft server list ping protocol.
4. Check the response for online player counts.
5. If players are detected, log the server (IP, player count, timestamp) to the connected database.

This allows large IP lists to be scanned quickly in parallel, rather than checking servers one at a time.

## Features

- Multi-threaded scanning with a user-defined thread count
- Reads target IPs from a simple text file
- Pings servers using the standard Minecraft protocol
- Detects and logs servers with online players
- Stores results in a PostgreSQL database with timestamps

## Requirements

- A PostgreSQL database (accessible via URI connection string)
- A text file containing the list of IPs to scan (one IP per line)

### Dependencies

This project uses [vcpkg](https://vcpkg.io/) for dependency management. Required packages:

- `spdlog`
- `libpqxx`
- `nlohmann-json`
- `boost`
- `cpp-httplib`
- `args`

## Configuration

Database connection settings are defined in `CONFIGURATION.h`.

Currently, only a **PostgreSQL URI connection string** is supported for the database configuration. Example format:

```
postgres://username:password@host:port/database
```

Edit `CONFIGURATION.h` and set your connection URI before building/running the program.

## Usage

```
./serverseeker <thread_count> <ips.txt>
```

| Argument       | Description                                      |
|----------------|---------------------------------------------------|
| `thread_count` | Number of threads to use for scanning             |
| `ips.txt`      | Path to a file containing the list of IPs to scan |

### Example

```
./serverseeker 256 ips.txt
```
If you dont have a IP list yet, you can use a sample from [here](http://20.79.8.185/minecraft_servers.txt).
This runs the scanner with 256 threads against the IP list in `ips.txt`.

- *Warning:* **If you get `Too many open files.` error, you need to set `ulimit -n 65535` to bypass limits.**
- *Note:* **After some testing, i found 512 threads is a sweet spot for gigabit connection.**

## How It Works

1. **Input parsing** — the IP list file is read and split into individual targets.
2. **Thread pool** — the specified number of threads is spawned to process the IP list concurrently.
3. **Server ping** — each thread pings its assigned IP(s) on the Minecraft server port to retrieve server status (including online player count).
4. **Player check** — if the response indicates one or more players online, the server is flagged as a hit.
5. **Logging** — hits are written to the configured PostgreSQL database, including the IP and the timestamp of detection.

## API

ServerSeeker runs a lightweight built-in API server on port `1337`, used to monitor the program while it is running.

| Endpoint              | Method | Description                                      |
|-----------------------|--------|---------------------------------------------------|
| `/api/v1/heartbeat`   | GET    | Returns a simple status check confirming the program is running |
| `/api/v1/status`      | GET    | Returns the current number of active jobs        |

The API server starts automatically alongside the scanner and listens on all network interfaces (`0.0.0.0:1337`).

This is an early version of the API and will continue to be expanded with more endpoints and monitoring data over time.

## Roadmap / TODO

- [ ] Support custom port scanning (currently hardcoded to the default Minecraft port `25565`)
- [ ] Expand API with more endpoints and detailed monitoring data

## Author

Made with love by **kenanwastaken**.
