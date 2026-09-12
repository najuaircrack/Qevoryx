# Qevoryx

High-performance raw-socket packet generator with modular architecture, IP spoofing, and pluggable protocol strategies.

## Features

- **TCP SYN packets** — randomized source IPs, optional MSS options, full checksum computation
- **UDP packets** — randomized payloads with unrolled 8-byte fill, variable payload sizes
- **Mixed mode** — alternating TCP/UDP across worker threads
- **IP spoofing** — generates spoofed source IPs from major cloud provider CIDR ranges (Cloudflare, AWS, Google, Azure, OVH, DigitalOcean), padded to power-of-2 for bitmask indexing
- **Modular architecture** — strategy pattern for packet types, pluggable transports, pluggable monitors
- **Performance-optimized** — xoshiro256** PRNG, CPU affinity pinning, stack-allocated cache-line aligned packets, no-memset hot paths, thread-local PPS counters, yield-based rate limiting
- **Real-time monitoring** — live PPS, total packets, max PPS tracking
- **Configurable** — CLI arguments, extensible to config files

## Requirements

- Linux (requires raw sockets)
- Root privileges (`sudo`)
- g++ with C++17 support
- pthread

## Building

```bash
make
```

Compiler flags: `-O3 -march=native -mtune=native -funroll-loops -flto -Wall -Wextra -pthread`

## Usage

```bash
sudo ./qevoryx <target_ip> <port> [threads] [mode] [rate]
```

### Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `target_ip` | (required) | Target IP address |
| `port` | (required) | Target port |
| `threads` | 1000 | Number of worker threads (max 10000) |
| `mode` | 0 | 0=Mixed, 1=TCP, 2=UDP |
| `rate` | 0 | Packets per second limit per thread (0=unlimited) |

### Examples

```bash
# Mixed TCP+UDP, 5000 threads
sudo ./qevoryx 192.168.1.100 25565 5000 0 0

# TCP SYN only, 10000 threads
sudo ./qevoryx 192.168.1.100 80 10000 1 0

# UDP flood with per-thread 1M PPS limit
sudo ./qevoryx 192.168.1.100 53 5000 2 1000000
```

### Stopping

Press `Ctrl+C` for graceful shutdown.

## Architecture

```
Qevoryx/
├── Makefile
├── LICENSE
├── include/
│   ├── common/          constants, types (PacketBuffer, PacketStatistics)
│   ├── config/          Config struct, CLI parser
│   ├── packet/          PacketStrategy interface, TCP/UDP strategies, factory
│   ├── protocol/        IPv4/TCP/UDP headers, checksum algorithms
│   ├── random/          xoshiro256** PRNG
│   ├── transport/       PacketTransport interface, file/test transports
│   ├── monitor/         Monitor interface, console/null monitors
│   └── app/             Application lifecycle, thread affinity
├── src/                 implementations
└── tests/               (ready for unit tests)
```

### Module Dependency Graph

```
main.cpp
  └── Application
        ├── Config (from CLI parser)
        ├── PacketStrategy (TCP / UDP / future)
        │     ├── IPv4Header
        │     ├── TcpHeader / UdpHeader
        │     └── Checksum
        ├── PacketBuffer (stack-allocated, cache-line aligned)
        ├── FastRandom (worker-local, xoshiro256**)
        ├── PacketTransport (file / test / future)
        └── Monitor (console / null / future)
```

### Design Principles

| Principle | Implementation |
|-----------|---------------|
| Open/Closed | New protocols via `PacketStrategy` subclass — no worker changes |
| Single Responsibility | Each module does one thing |
| No Global Mutable State | Config owns settings, workers own local counters |
| Separation of Concerns | Construction ≠ Transport ≠ Monitoring |
| Dependency Inversion | Workers depend on interfaces, not implementations |

### Adding a New Protocol

1. Create `include/packet/icmp_strategy.hpp`
2. Create `src/packet/icmp_strategy.cpp`
3. Implement `PacketStrategy::build()`
4. Add `PacketMode::Icmp` to config enum
5. Register in `create_strategy()` factory
6. Done — workers, monitor, transport unchanged

### Adding a New Monitor

1. Create `include/monitor/json_monitor.hpp`
2. Create `src/monitor/json_monitor.cpp`
3. Implement `Monitor::start/update/stop`
4. Register in `create_monitor()` factory

## Performance Optimizations

| Optimization | Effect |
|-------------|--------|
| CPU affinity pinning | Prevents L1/L2 cache thrashing between cores |
| Thread-local PPS counters | Eliminates atomic cache-line bouncing |
| Skip IP checksum (kernel computes) | Saves one full buffer walk per packet |
| Power-of-2 spoof pool + bitmask | Branchless IP selection, no modulo |
| No memset in hot loop | Explicit field writes only |
| Precomputed packet lengths | Compile-time constants |
| Stack-allocated aligned packets | No heap allocator in hot path |
| Fast coin flip (bitwise) | Replaces float division |
| yield() rate limiting | No 50-100us scheduler penalty |
| SO_SNDBUF = 1MB | Lean kernel buffering |
| -O3 -march=native -flto -funroll-loops | AVX2, LTO, loop unrolling |

## System Tuning

Applied automatically at startup:

```bash
net.ipv4.tcp_tw_reuse=1
net.ipv4.tcp_fin_timeout=5
net.ipv4.tcp_timestamps=0
net.core.rmem_max=268435456
net.core.wmem_max=268435456
net.core.netdev_max_backlog=500000
net.ipv4.ip_local_port_range="1024 65535"
net.ipv4.conf.all.rp_filter=0
net.ipv4.tcp_max_syn_backlog=500000
net.ipv4.tcp_syncookies=0
```

## Configuration

### Current

CLI-only:
```
sudo ./qevoryx <ip> <port> [threads] [mode] [rate]
```

### Future

Config file support planned — same `Config` struct, additional input adapter.

## Extensibility Roadmap

- [ ] ICMP strategy
- [ ] Config file support (JSON/TOML)
- [ ] JSON/CSV/Prometheus monitors
- [ ] Strategy registry (dynamic plugin loading)
- [ ] Rate limiting as separate `GenerationPolicy` abstraction
- [ ] Unit test suite (checksum, CIDR, strategies, transport)

## Project Structure

```
src/main.cpp                     7 lines    entry point
src/app/application.cpp         ~180 lines  lifecycle, workers, monitoring
src/app/thread_affinity.cpp     ~15 lines   CPU pinning
src/config/cli_parser.cpp       ~50 lines   argument parsing
src/packet/tcp_syn_strategy.cpp ~70 lines   TCP SYN construction
src/packet/udp_strategy.cpp     ~60 lines   UDP construction
src/packet/packet.cpp           ~20 lines   strategy factory
src/protocol/checksum.cpp       ~40 lines   internet/TCP checksum
src/random/fast_random.cpp      ~40 lines   xoshiro256**
src/transport/file_transport.cpp ~25 lines   file output
src/transport/test_transport.cpp ~15 lines   test capture
src/monitor/console_monitor.cpp ~50 lines   live display
src/monitor/monitor_factory.cpp ~15 lines   monitor selection
```

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE)

## Disclaimer

For authorized testing and educational purposes only. Users are responsible for compliance with all applicable laws and regulations.
