<p align="center">
  <img src="/assets/banner.png" alt="Qevoryx banner" width="880">
</p>

# Qevoryx

Qevoryx is a packet generator for careful network testing in places where you have permission to work. It opens as a polished FTXUI control panel, keeps your settings between runs, and gives you clear feedback while it runs.

The command line is still there when you need it, but the panel is now the main way to use Qevoryx.

## Editions

| | Open (this repo) | Enterprise (private) |
|---|---|---|
| Local packet engine + panel + CLI | Yes | Yes |
| C2 server, agents, headless mode | No | Yes |
| Remote operator client (F4) | Yes — drive Enterprise | Yes |
| Service install / auto-resume | No | Yes |

Full guides live in [`docs/`](docs/): [architecture](docs/ARCHITECTURE.md),
[usage](docs/USAGE.md). The picture first:

```
┌─────────────┐   config    ┌──────────────────┐   snapshot   ┌─────────────┐
│  YOU (keys) │ ──────────▶ │  FTXUI PANEL     │ ◀─────────── │  CONTROLLER │
└─────────────┘   actions   │  config/actions/ │   5×/second  │  backend    │
                            │  status/log      │              └──────┬──────┘
                            └──────────────────┘                     │ threads,
                                                                     sockets,
                                                                     packets
```

The panel never touches workers, sockets, or packets directly — everything
goes through the controller, which is why the UI stays responsive while
thousands of packets per second flow underneath.

## What you get

* A component based FTXUI control panel
* A responsive layout that adapts to terminal width and height
* Separate panels for configuration, actions, status, and the event log
* An animated 96×27 GIF-derived splash on start, and a compact emblem in the header
* Context sensitive keyboard hints
* A dedicated runtime screen with live counters
* A help screen and reusable confirmation dialogs
* Settings that are remembered in a simple file on your computer
* Safe first run values: one worker, a rate limit, your real interface address, and a local target
* Seven traffic profiles for mixed TCP, UDP, and ICMP testing
* A typed confirmation before live traffic starts
* Live packet and error counters
* Clean shutdown that waits for workers to finish

## Requirements

Qevoryx builds on Linux, macOS, and Windows. You need a C++17 compiler, CMake 3.20 or newer, and pthread support. FTXUI is fetched automatically by CMake.

On Debian or Ubuntu, install the build tools with:

```bash
sudo apt install build-essential cmake
```

On Windows, use Visual Studio 2022 or newer with the C++ and CMake workloads.

Run Qevoryx from an elevated terminal when the tool needs raw socket access.

Use a terminal that can show UTF-8 characters. Qevoryx falls back to simpler arrows and status marks when your locale does not use UTF-8.

## Build

```bash
make
```

Or configure and build directly with CMake:

```bash
cmake -S . -B build
cmake --build build --config Release
```

The resulting `qevoryx` executable is the FTXUI frontend. It supports the same configuration fields and backend actions as before, including typed launch confirmation, live counters, pause/resume, stop, save/reset, event-log scrolling, and help.

## Start the panel

Run Qevoryx without arguments:

```bash
sudo ./qevoryx
```

The control panel opens immediately. You can also ask for it directly:

```bash
sudo ./qevoryx --tui
```

On start, Qevoryx shows a brief colored splash of the logo. Press any key to
skip it, or set `QEVORYX_NO_SPLASH=1` to disable it.

## Panel controls

* Arrow keys move through fields and actions
* Tab moves focus through configuration, actions, and the event log
* Enter starts editing a field or activates the selected action
* Select the Interface row and use Left or Right to switch detected IPv4 adapters
* Left and Right change a choice or adjust a number
* Space selects the next choice
* Esc cancels the current edit
* P pauses or resumes the runtime
* S saves your settings
* R restores the safe defaults
* Q quits
* Ctrl+C also quits

The panel is built with FTXUI. The UI is separated from the backend through an application controller, so the interface never touches worker threads, sockets, or packet construction directly. Configuration changes remain in the UI while actions update runtime state, preventing launch, stop, pause, help, and log actions from restoring old values. The Interface row cycles through active IPv4 adapters and prefers the adapter with the default route. When the event log has focus, the arrow keys scroll through older events.

## Remote operator mode (Enterprise)

This build contains no C2 engine, but it can operate a remote Enterprise
server you have credentials for. Press **F4**, enter the server host, port,
and operator token (or check `--remote-status` for scripting):

```bash
QEVORYX_REMOTE_TOKEN=<token> ./qevoryx --remote-status --remote-host 10.0.0.1 --remote-port 8080
```

Remote operators can dispatch tests within their assigned scope and stop
their own tests. Server lifecycle stays local-only by design: there is no
remote stop, by protocol, not just by UI. See [usage](docs/USAGE.md) for the
full walkthrough.

## Screens

The main screen shows configuration, actions, status, and the latest events. Launching switches to a runtime screen with the selected profile, worker count, target, live packet count, and error count. Press `R` to return to the main screen, `S` to stop, or `Q` to quit.

Press `?` at any time to open the help screen. Resetting settings opens a centered confirmation dialog so you can cancel safely.

The startup splash is generated from `/assets/intro.gif` as a 96×27
Unicode half-block animation, while the header emblem is generated from
`/assets/logo.png`. Both keep the brand's white-and-red palette, and the block
art is generated by `tools/logo_converter.py`.
The layout adapts to your terminal size. The launch action asks you to type YES
before any live traffic starts.

## Remembered settings

The panel loads your previous configuration when it starts. Press S to save your choices.

The settings file lives here:

* Default location: `$HOME/.config/qevoryx/settings.ini`
* Custom location: `$XDG_CONFIG_HOME/qevoryx/settings.ini`
* Windows: `%APPDATA%\Qevoryx\settings.ini`

If the file is missing or contains an invalid value, Qevoryx quietly returns to the safe defaults instead of guessing.

## Command line

The command line remains available for scripts and quick runs:

```bash
sudo ./qevoryx --cli <target_ip> <port> [threads] [mode] [rate] [flags]
```

For example, to generate mixed traffic with one worker and a rate limit:

```bash
sudo ./qevoryx --cli 192.168.1.100 25565 1 0 1000
```

To use a specific real interface address:

```bash
sudo ./qevoryx --cli 192.168.1.100 25565 1 1 1000 --real-ip wlan0
```

Press Ctrl+C when you want Qevoryx to stop.

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
| `--cli` | Use the command line interface |
| `--tui` | Open the terminal control panel |
| `--real-ip [interface]` | Use the address from a real network interface |
| `--spoof` | Explicitly enable spoofed source addresses |
| `--interface <name>` | Choose the network interface for real source selection |

## Assets

* `/assets/banner.png` is the banner shown at the top of this page
* `/assets/logobg.png` is the source for the compact panel logo
* `/assets/logo.png` is the standalone logo

## Design notes

Qevoryx keeps packet building, transport, and monitoring separate. Workers use a compact random generator and cache aligned packet buffers. Release builds target a portable x86 64 baseline so the resulting binary can run on many machines.

## Use responsibly

Please run Qevoryx only on systems you own or have written permission to test. The panel asks for confirmation, but the responsibility to choose a valid target remains with you.

## License

Qevoryx is released under the GNU General Public License v3.0.
