# Qevoryx

High-performance raw-socket packet generator with modular architecture, IP spoofing, and pluggable protocol strategies.

## Features

- **7 packet modes** — Mixed, TCP SYN, UDP, ICMP Echo, TCP ACK, TCP RST, TCP SYN-ACK
- **TUI mode** — interactive terminal configuration when `--tui` is passed
- **Source selection** — real interface IP by default, spoofed source IPs only with explicit `--spoof`
- **Modular architecture** — strategy pattern for packet types, pluggable transports, pluggable monitors
- **Performance-optimized** — xoshiro256** PRNG, CPU affinity pinning, stack-allocated cache-line aligned packets, no-memset hot paths, thread-local PPS counters, yield-based rate limiting
- **Real-time monitoring** — live PPS, total packets, max PPS tracking

## Requirements

- Linux (requires raw sockets)
- Root privileges (`sudo`)
- g++ with C++17 support
- pthread

## Building

```bash
make
```

## Usage

```bash
# CLI mode
sudo ./qevoryx <target_ip> <port> [threads] [mode] [rate] [flags]

# TUI mode (interactive)
sudo ./qevoryx --tui
```

### Modes

| Value | Mode | Description |
|-------|------|-------------|
| 0 | Mixed | Round-robin TCP+UDP+ICMP |
| 1 | TCP SYN | SYN flood with random seq |
| 2 | UDP | Random payload flood |
| 3 | ICMP Echo | Ping flood |
| 4 | TCP ACK | ACK flood |
| 5 | TCP RST | RST flood |
| 6 | TCP SYN-ACK | SYN+ACK flood |

### Flags

| Flag | Description |
|------|-------------|
| `--tui` | Launch interactive terminal UI |
| `--real-ip [interface]` | Use the real interface IP (default) |
| `--spoof` | Explicitly enable spoofed source IPs |
| `--interface <name>` | Specify network interface for real IP |

### Examples

```bash
# Mixed TCP+UDP+ICMP, 5000 threads
sudo ./qevoryx 192.168.1.100 25565 5000 0 0

# TCP SYN, 10000 threads
sudo ./qevoryx 192.168.1.100 80 10000 1 0

# UDP flood with per-thread 1M PPS limit
sudo ./qevoryx 192.168.1.100 53 5000 2 1000000

# ICMP with real IP from wlan0
sudo ./qevoryx 192.168.1.100 25565 5000 3 0 --real-ip wlan0
sudo ./qevoryx 192.168.1.100 25565 5000 1 0 --spoof

# Interactive TUI
sudo ./qevoryx --tui
```

### Stopping

Press `Ctrl+C` for graceful shutdown.

## TUI Mode

The interactive TUI guides you through 6 steps:

1. **Target** — set IP and port
2. **Attack Mode** — pick from 7 packet strategies
3. **Workers** — set thread count (max 10000)
4. **IP Mode** — real by default, spoofed only if explicitly selected
5. **Payload** — min/max bytes for UDP modes
6. **Rate Limit** — per-thread PPS cap

After configuration, you get a summary and confirm before launch.

## Architecture

```
Qevoryx/
├── Makefile
├── LICENSE
├── include/
│   ├── common/          constants, types (PacketBuffer, PacketStatistics)
│   ├── config/          Config struct, CLI parser
│   ├── packet/          PacketStrategy interface, 6 strategy implementations
│   ├── protocol/        IPv4/TCP/UDP/ICMP headers, checksum algorithms
│   ├── random/          xoshiro256** PRNG
│   ├── transport/       PacketTransport interface, file/test transports
│   ├── monitor/         Monitor interface, console/null monitors
│   ├── tui/             Interactive terminal configuration
│   └── app/             Application lifecycle, thread affinity
├── src/                 implementations
└── README.md
```

## Adding a New Protocol

1. Create `include/packet/xxx_strategy.hpp`
2. Create `src/packet/xxx_strategy.cpp`
3. Implement `PacketStrategy::build()`
4. Add `PacketMode::Xxx` to config enum
5. Register in `create_strategy()` factory
6. Done — workers, monitor, transport unchanged

## Performance Optimizations

| Optimization | Effect |
|-------------|--------|
| CPU affinity pinning | Prevents L1/L2 cache thrashing |
| Thread-local PPS counters | Eliminates atomic cache-line bouncing |
| Skip IP checksum (kernel computes) | Saves one full buffer walk per packet |
| No memset in hot loop | Explicit field writes only |
| Stack-allocated aligned packets | No heap allocator in hot path |
| yield() rate limiting | No scheduler penalty |
| -O3 -march=native -flto | AVX2, LTO, loop unrolling |

## License

GNU General Public License v3.0

## Disclaimer

For authorized testing and educational purposes only.
