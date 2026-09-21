# Qevoryx Usage Guide (Open edition)

Step-by-step: install → first local test → panel mastery → remote mode →
troubleshooting. Only test systems you own or have written permission to test.

## 1. First run (60 seconds)

```bash
sudo ./qevoryx            # panel opens; safe defaults are preloaded
```

What you see:

```
┌─ CONFIGURATION ────────┐  ┌─ ACTIONS ────┐
│ ▶ Target IP   [127...] │  │ ▶ Launch     │
│   Target Port [25565]  │  │   Stop       │
│   Profile     [mixed]  │  │   Pause      │
│   Workers     [1]      │  │   Save       │
└────────────────────────┘  └──────────────┘
┌─ STATUS ─────────────────────────────────┐
│ Runtime idle   Generated: 0   Errors: 0  │
└──────────────────────────────────────────┘
┌─ EVENT LOG ──────────────────────────────┘
```

1. Check Target IP is your own machine/lab (`127.0.0.1` default).
2. Select **Launch**, type `YES`, Enter. Counters start moving.
3. `P` pauses, `X` stops, `Q` quits.

## 2. Panel keys (complete)

| Key | Where | Does what |
|---|---|---|
| `↑↓` | anywhere | Move selection |
| `←→` / `Space` | config row | Change choice / adjust number |
| `Enter` | config row | Edit value (`Esc` cancels) |
| `Enter` | action | Run it |
| `Tab` | anywhere | Cycle Configuration → Actions → Event Log |
| `L` | anywhere | Launch confirmation |
| `P` / `X` | running | Pause / Stop (non-blocking) |
| `S` | anywhere | Save settings to disk |
| `R` | anywhere | Reset to safe defaults (typed confirm) |
| `?` | anywhere | Help overlay |
| `F2` / `F3` | anywhere | Local mode / C2 panels |
| `F4` | anywhere | Remote connect dialog (Enterprise) |
| `Q` / `Ctrl+C` | anywhere | Quit (workers joined cleanly) |

## 3. CLI cookbook

```bash
# Panel (default) and explicit panel flag
sudo ./qevoryx
sudo ./qevoryx --tui

# One-shot traffic: target port workers profile rate
sudo ./qevoryx --cli 192.168.1.100 25565 4 0 1000

# Use a real interface address instead of spoofed sources
sudo ./qevoryx --cli 192.168.1.100 80 2 1 1000 --real-ip wlan0

# Snapshot render (for docs/CI, no interaction)
./qevoryx --snapshot 120 30
```

Profiles `0-6`: mixed, TCP SYN, UDP, ICMP echo, TCP ACK, TCP RST, TCP SYN-ACK.

## 4. Remote operator mode (needs Enterprise server + token)

Your admin gives you three things: **host, API port, operator token**.
Your powers: dispatch tests inside your assigned scope, stop your own tests,
see your history. You can never stop/start the server — that path does not
exist in the protocol.

```
Panel:  F4 → host → port → token → C2 panels go live (REMOTE LINK banner)
Script: QEVORYX_REMOTE_TOKEN=<t> ./qevoryx --remote-status \
            --remote-host 10.0.0.1 --remote-port 8080
        ./qevoryx --remote-dispatch --remote-host 10.0.0.1 --remote-port 8080 \
            --remote-target 10.9.0.5 --remote-tport 80 \
            --remote-workers 4 --remote-duration 300
```

Prefer the env var over `--remote-token`: flags stay visible in `ps`.

## 5. Settings file

`~/.config/qevoryx/settings.ini` (or `%APPDATA%\Qevoryx\settings.ini`).
Written atomically with owner-only permissions. Corrupt file → safe defaults,
never a crash. `S` saves, `R` resets (both ask first where it matters).

## 6. Troubleshooting

| Symptom | Cause → fix |
|---|---|
| `socket() failed` / workers all fail | Not elevated → `sudo` / admin terminal |
| `F4` says "not included" | Old binary — rebuild; remote client ships in every build |
| Remote connect fails | Wrong host/port/token, or server unreachable (check firewall/API port) |
| `403 target outside operator scope` | Your token isn't scoped to that target — ask your admin |
| `401` | Bad/expired token — ask for a fresh one |
| Panel frozen on Stop | Fixed in current builds (non-blocking stop); update if seen |
| Counters stay 0 | Target dropping packets (check target + firewall), or rate limit 0 |
| `--serve` says "not included" | Correct on this build — headless serve is Enterprise-only |

## 7. Responsible use (read this)

* Only targets you own or have **written** permission to test.
* Start with 1 worker + rate limit against loopback; scale deliberately.
* Spoofed sources can look like an attack to every middlebox between you and
  the target — prefer `--real-ip` on your own networks.
* The panel's YES/RESET confirmations exist to make accidents hard, not
  impossible. The responsibility for the target is always yours.
