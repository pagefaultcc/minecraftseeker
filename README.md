# MinecraftSeeker

MinecraftSeeker is a high-performance multi-threaded scanner for Minecraft servers. It scans a list of target IPs, pings each server using the Minecraft server list ping protocol, checks for online players, provides real-time progress metrics (masscan-style), serves an HTTP monitoring API, and exports discovered servers to JSON/text files or a PostgreSQL database.

The main use case for this tool is **sniping** — finding servers with active players as quickly as possible.

---

## Features

- **High-speed multi-threaded scanning** with user-defined worker thread count.
- **Custom Minecraft target port** via `-p` / `--port` (default: `25565`).
- **Loop mode** via `-l` / `--loop` to continuously loop through IP list.
- **Real-time status display** (like masscan) showing rate (pings/s), percentage progress, servers found, players found, and ETA.
- **Multiple output formats** via `-o` / `--output` supporting both `.json` and `.txt`.
- **Optional PostgreSQL database logging** via `-db` / `--database-uri` CLI argument.
- **Built-in HTTP monitoring API** with customizable port via `-mp` / `--monitoring-port` (default: `1337`).
- **Cross-platform build support** for Linux and Windows x64.

---

## Command Line Options

```
Usage: serverseeker [options] <ips.txt>

Options:
  -t,  --threads <num>            Number of worker threads (default: 256)
  -f,  --file <path>              Path to file containing IP list
  -p,  --port <port>              Minecraft server port (default: 25565)
  -mp, --monitoring-port <port>   API monitoring port (default: 1337)
  -o,  --output <path>            Output file path (.txt or .json)
  -db, --database-uri <uri>       PostgreSQL connection URI
  -l,  --loop                     Loop the IP list continuously
  -h,  --help                     Display this help menu
```

### Examples

```bash
# Basic scan with 256 threads
./serverseeker -t 256 -f ips.txt

# Scan custom Minecraft port and export hits to JSON
./serverseeker -t 512 -f ips.txt -p 25565 -o results.json

# Scan with custom monitoring API port and continuous loop
./serverseeker -t 256 -f ips.txt -o results.txt -mp 8080 -l

# Scan and log directly to PostgreSQL database
./serverseeker -t 512 -f ips.txt -db postgres://username:password@localhost:5432/database

# Legacy positional syntax
./serverseeker 256 ips.txt
```

---

## Status Output

During scanning, ServerSeeker outputs real-time status updates every second:

```
[13:46:20] [info ] Status: 15.20% | Rate: 512.4 pings/s | Pinged: 3550/23356 | Found: 42 | Players: 18 | ETA: 00:00:38
```

In continuous loop mode (`-l`):

```
[13:46:20] [info ] Status: [Loop] | Rate: 512.4 pings/s | Pinged: 48920 | Found: 380 | Players: 120 | Active: 256/256
```

---

## Monitoring API

ServerSeeker includes an embedded HTTP monitoring API server listening on `0.0.0.0:<monitoring_port>` (default port `1337`):

| Endpoint            | Method | Description |
|---------------------|--------|-------------|
| `/api/v1/heartbeat` | GET    | Service heartbeat, version, and uptime |
| `/api/v1/status`    | GET    | Detailed scan status, rate, thread assignments, and stats |
| `/api/v1/metrics`   | GET    | Real-time metrics (pings/sec, active threads, queue length) |
| `/api/v1/hits`      | GET    | List of recent servers found with online players |

---

## Building

### Requirements
- CMake (>= 3.20)
- C++20 compiler (GCC, Clang, or MSVC)
- [vcpkg](https://vcpkg.io/)

### Linux / macOS
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Windows (MSVC)
```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET="x64-windows"
cmake --build build --config Release
```

---

## Author

Made with love by **kenanwastaken** & **pagefaultcc**.
