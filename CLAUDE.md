# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

flrig is a cross-platform amateur radio transceiver control program. This fork (v2.0.10) adds Yaesu FTX-1 support on top of the upstream release. The active development branch is `support_ftx1_tab-changes`.

## Build System

Uses GNU Autotools (Autoconf + Automake). The configure script is pre-generated.

```bash
./configure                   # Configure for the local system
make -j5                      # Build (output: src/flrig)
make clean                    # Remove build artifacts
make distclean                # Remove all generated files including configure outputs
make appbundle                # macOS: build .dmg application bundle
```

Dependencies: FLTK toolkit, optional libflxmlrpc (internal fallback included), optional libgpiod.

There are no automated tests. Verification is done by running the compiled binary manually.

## Architecture

**Core design:** Each radio transceiver is encapsulated in its own C++ class. `rigbase` (`src/include/rigbase.h`, `src/rigs/rigbase.cxx`) is the abstract base class. All 110+ rig implementations derive from it and override virtual methods for vendor-specific CAT protocols.

**Key subsystems:**

- `src/rigs/` — Rig implementations organized by manufacturer (`yaesu/`, `icom/`, `kenwood/`, `elecraft/`, `tentec/`, `xiegu/`, etc.). The factory/registry is `src/rigs/rigs.cxx`.
- `src/include/` - Header files for all source files. (for specific rigs see`yaesu/`, and `rigbase.h` generic for all rigs)
- `src/UI/` — FLTK UI definitions (`.fl` files and generated headers). Main panels are `rigpanel`, `meters_dialog`, and per-rig panels (K3, K4, KX3).
- `src/support/` — I/O utilities: `serial.cxx` (serial port), `socket_io.cxx` (TCP/IP), `rig_io.cxx` (higher-level coordination), `status.cxx` (global program state), `threads.cxx` (threading).
- `src/server/` — XML-RPC server (`xml_server.cxx`, `xmlrpc_rig.cxx`) exposing rig control over the network.
- `src/cwio/`, `src/fskio/` — CW (Morse) and FSK keyer I/O.
- `src/gpio/` — GPIO support (Raspberry Pi etc.).
- `src/xmlrpcpp/` — Bundled XML-RPC library (~37 files), used when system libflxmlrpc is unavailable.

**Connection types:** `SERIAL`, `TCPIP`, `TCI` (defined in `rigbase.h`).

**UI framework:** FLTK (Fast Light Toolkit). The app is single-binary, no web or scripting layers.

## Adding a New Rig

1. Create `src/rigs/<manufacturer>/<MODEL>.h` and `.cxx`, following an existing driver as a template (e.g., `FTX1.cxx` is based on `FT710`).
2. Register the rig in `src/rigs/rigs.cxx`.
3. Add source files to `src/Makefile.am`.

## FTX-1 Specific Notes

The active branch (`support_ftx1_tab-changes`) is refining the Yaesu FTX-1 driver (`src/rigs/yaesu/FTX1.cxx`, `src/include/yaesu/FTX1.h`). The FTX-1 driver is based on the FT-710 driver. Mode values use hex characters `'1'`–`'I'` as CAT protocol codes. The `TRACE_STREAM(level, expr)` macro (defined locally in `FTX1.cxx`) wraps the `trace()` call for streaming-style debug output.

## Debugging

Use the `trace()` function and `TRACE_STREAM` macro for runtime tracing. The `#define TESTING 1` guard near the top of rig files enables additional test output — comment it back out before distribution.