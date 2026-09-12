<p align="center">
  <img src="/assets/banner.png" alt="Qevoryx banner" width="880">
</p>

# Qevoryx

Qevoryx is a packet generator for careful network testing in places where you have permission to work. It opens as a polished ncursesw control panel, keeps your settings between runs, and gives you clear feedback while it runs.

The command line is still there when you need it, but the panel is now the main way to use Qevoryx.

## What you get

* A component based ncursesw control panel
* A responsive layout that adapts to terminal width and height
* Separate panels for configuration, actions, status, and the event log
* A compact Qevoryx logo in the header
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

Qevoryx is built for Linux and POSIX systems. You need root privileges, a C++17 compiler, pthread support, and ncursesw with panel support.

On Debian or Ubuntu, install the development package with:

```bash
sudo apt install libncursesw5-dev
```

Windows builds are available, but raw socket support depends on your system setup. If you use Windows, run Qevoryx from an elevated shell and confirm that your platform allows the socket type Qevoryx requests. The panel also needs a terminal that supports ANSI escape sequences, such as Windows Terminal.

## Build

```bash
make
```

For a static binary, use:

```bash
make static
```

## Start the panel

Run Qevoryx without arguments:

```bash
sudo ./qevoryx
```

The control panel opens immediately. You can also ask for it directly:

```bash
sudo ./qevoryx --tui
```

## Panel controls

* Arrow keys move through fields and actions
* Tab switches between the configuration panel and the actions panel
* Enter starts editing a field or activates the selected action
* Left and Right change a choice or adjust a number
* Space selects the next choice
* Esc cancels the current edit
* S saves your settings
* D restores the safe defaults
* Q quits
* Ctrl+C also quits

The panel uses ncursesw windows and panels internally. The UI is separated from the backend through an application controller, so the interface never touches worker threads, sockets, or packet construction directly.

## Screens

The main screen shows configuration, actions, status, and the latest events. Launching switches to a runtime screen with the selected profile, worker count, target, live packet count, and error count. Press `R` to return to the main screen, `S` to stop, or `Q` to quit.

Press `?` at any time to open the help screen. Resetting settings opens a centered confirmation dialog so you can cancel safely.

The top of the panel shows a truecolor mark made from `/assets/logobg.png`. The layout adapts to your terminal size, and the status area explains what each action does. The launch action asks you to type YES before any live traffic starts.

## Remembered settings

The panel loads your previous configuration when it starts. It saves your choices when you press S and when you confirm a launch.

The settings file lives here:

* Linux: `$HOME/.config/qevoryx/settings.ini`
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
