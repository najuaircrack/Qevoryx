# Qevoryx Architecture (Open edition)

This is the complete map of the public codebase. No C2 internals live here —
the Enterprise server is a separate private tree that plugs into the
controller interface described below.

## Big picture

```
                        ┌────────────────────────────────────────┐
                        │              qevoryx binary            │
                        │                                        │
  keyboard ────────────▶│  FTXUI PANEL (src/ftxui)               │
                        │   config │ actions │ status │ event log │
                        │         │                                    │
                        │         │  snapshot (5×/s) + actions        │
                        │         ▼                                    │
                        │  CONTROLLER (src/app/application_controller)│
                        │   ├─ local runtime (threads, counters)      │
                        │   └─ remote client (Enterprise, optional)   │
                        └───────┬──────────────────┬─────────────────┘
                                │ local            │ remote (F4/token)
                                ▼                  ▼
                    ┌───────────────────┐  ┌───────────────────┐
                    │  PACKET ENGINE    │  │  Enterprise C2    │
                    │  (this process)   │  │  (other machine)  │
                    └───────────────────┘  └───────────────────┘
```

## Layer by layer

```
src/packet/      Strategy per profile: TCP SYN / UDP / ICMP / ACK / RST / SYN-ACK / Mixed
                       │  build(ctx, buffer) — pure function, no I/O, no threads
                       ▼
src/protocol/    Wire structs (IPv4/TCP/UDP/ICMP) + checksums (verified by tests/)
src/random/      FastRandom: one instance PER THREAD (not thread-safe, not crypto)
src/transport/   Packet sinks (raw socket path lives in app/, file/test transports here)
src/app/         Workers: N threads × (build → send → count). atomics for counters.
src/monitor/     Console progress + PPS math
src/config/      CLI parsing (throws, never exits) + settings.ini load/save
src/remote/      Dependency-free HTTP+JSON operator client (also used by Enterprise)
src/ftxui/       Panel: renders snapshots, translates keys → controller calls
include/ftxui/   Snapshot structs — the ONLY contract between UI and backend
```

## The snapshot contract (the most important idea)

The UI never reads backend state directly. Every 200 ms it asks the
controller for an `ApplicationSnapshot` (plain data: config, counters,
agents, events) and renders that. Actions go the other way as method calls
(`launch`, `stop`, `c2_broadcast_task`, …). Consequences:

* The UI thread never blocks on workers or the network.
* A remote server can drive the identical panels — only the controller
  implementation changes, which is exactly how Enterprise remote mode works.
* Anything you add to the UI must flow through the snapshot or an action.
  No side channels.

## Threading map (local runtime)

```
UI thread ──snapshot(5Hz)──▶ controller ──atomics──▶ worker threads (N)
   │                              │                       │ raw sockets
   │ actions                      │ join/reap             ▼
   └──────────────────────────────┘                    target
```

* Workers share nothing except atomic counters and read-only config.
* Stop is non-blocking (`request_stop`); the finished thread is reaped on the
  next snapshot, so the panel never freezes.
* Raw sockets need privileges: run elevated (`sudo`), or workers fail closed
  with a clear error instead of sending nothing.

## Build matrix

```
cmake -S . -B build            # full build if C2 sources present, else…
cmake -S . -B build -DQEVORYX_ENABLE_C2=OFF   # …forced local-only build

sources present? ──yes──▶ qevoryx + qevoryx-agent + tests
      │
      no
      ▼
  qevoryx (local only) + tests   ← this is what CI builds from this repo
```

`QEVORYX_ENABLE_C2` is a compile-time switch. C2 code paths become logging
stubs; the local engine is untouched. The private tree adds the engine back
with zero changes to these files.

## Data on disk (this edition)

```
~/.config/qevoryx/settings.ini      local panel preferences (atomic write, 0600)
```

No agents, no tasks, no server state — there is no server here. Enterprise
persists audit/loot/tasks/operators under its own data dir (private docs).
