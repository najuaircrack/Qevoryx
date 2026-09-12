<p align="center">
  <img src="assets/banner.png" alt="Qevoryx banner" width="880">
</p>

# Qevoryx

Qevoryx is a modular packet generator for people who need to test network behaviour in a controlled environment. It keeps configuration clear, gives you useful feedback while it runs, and shuts down cleanly when you ask it to stop.

## What you get

* Seven traffic profiles, including mixed TCP, UDP, and ICMP generation
* An interactive terminal setup for people who prefer guided configuration
* Clear source selection, with your real interface address used by default
* A modular design that keeps packet building, transport, and monitoring separate
* Live packet and error counters so you can see what the tool is doing
* Predictable shutdown that waits for workers to finish before the process exits

## Requirements

Qevoryx is primarily built for Linux because raw sockets are simplest there. You will need root privileges, a C++17 compiler, and pthread support.

Windows builds are available, but raw socket support depends on your system setup. If you are on Windows and the tool cannot open a raw socket, run it from an elevated shell and confirm that your platform allows the socket type Qevoryx requests.

## Build

```bash
make
```

If you want a static binary, use:

```bash
make static
```

## Quick start

Start with the interactive interface if you would rather answer a few questions than remember argument positions:

```bash
sudo ./qevoryx --tui
```

If you prefer the command line, the basic form is:

```bash
sudo ./qevoryx <target_ip> <port> [threads] [mode] [rate] [flags]
```

For example, to generate mixed traffic with 1000 workers and no rate limit:

```bash
sudo ./qevoryx 192.168.1.100 25565 1000 0 0
```

To use a specific real interface address:

```bash
sudo ./qevoryx 192.168.1.100 25565 1000 1 0 --real-ip wlan0
```

To explicitly enable spoofed source addresses:

```bash
sudo ./qevoryx 192.168.1.100 25565 1000 1 0 --spoof
```

Press `Ctrl+C` when you want Qevoryx to stop.

## Traffic profiles

| Value | Profile | What it sends |
|-------|---------|---------------|
| 0 | Mixed | Alternates between TCP SYN, UDP, and ICMP traffic |
| 1 | TCP SYN | TCP packets with the SYN flag |
| 2 | UDP | UDP packets with a random payload |
| 3 | ICMP Echo | ICMP echo requests |
| 4 | TCP ACK | TCP packets with the ACK flag |
| 5 | TCP RST | TCP packets with the RST flag |
| 6 | TCP SYN ACK | TCP packets with both SYN and ACK flags |

## Command line options

| Option | Meaning |
|--------|---------|
| `--help` | Show the built in help text |
| `--version` | Show the current version |
| `--tui` | Start the interactive terminal interface |
| `--real-ip [interface]` | Use the address from a real network interface |
| `--spoof` | Explicitly enable spoofed source addresses |
| `--interface <name>` | Choose the network interface for real source selection |

## Interactive setup

The terminal interface walks you through six short steps:

1. Choose the target address and port
2. Pick one of the seven traffic profiles
3. Set the number of workers
4. Choose between a real interface address and spoofed source addresses
5. Set the payload range for UDP traffic
6. Set a per worker rate limit, or leave it unlimited

At the end you get a summary and must confirm before anything starts.

## Project layout

```text
Qevoryx/
├── Makefile
├── LICENSE
├── assets/
├── include/
│   ├── common/
│   ├── config/
│   ├── packet/
│   ├── protocol/
│   ├── random/
│   ├── transport/
│   ├── monitor/
│   ├── tui/
│   └── app/
├── src/
└── README.md
```

## Add a new traffic profile

1. Create a header under `include/packet/`
2. Create the matching implementation under `src/packet/`
3. Implement the `PacketStrategy::build()` method
4. Add the new mode to the configuration enum
5. Register it in the strategy factory
6. Rebuild the project

Workers, monitoring, and transport code do not need to change when you add a new profile.

## Design notes

Qevoryx uses a compact random number generator, cache aligned packet buffers, and a simple worker model. The build uses a portable x86 64 baseline so the resulting binary can run on a wide range of machines.

## License

Qevoryx is released under the GNU General Public License v3.0.

## Use responsibly

Please run Qevoryx only on systems you own or have written permission to test.
