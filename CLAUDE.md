# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Node.js native addon for opening cash drawers connected to ESC/POS printers. It uses N-API for cross-platform native bindings and supports Windows, macOS, and Linux.

## Build Commands

```bash
# Install dependencies and compile native addon
npm install

# Run tests (uses non-existent printer for safe testing)
npm test

# Create prebuilds for current platform
npm run prebuild

# Create prebuilds for all architectures on current platform
npm run prebuild-all

# Clean build artifacts
npm run clean
```

## Architecture

### Native Addon Layer
- `src/openCashDrawer.cc` - C++ native code using N-API. Contains platform-specific implementations:
  - **Windows**: Uses Windows Print Spooler API (`winspool.h`) with RAII wrapper `PrinterHandle`
  - **macOS/Linux**: Uses CUPS library with temporary file approach
- `binding.gyp` - Node-gyp build configuration with platform-specific compiler flags and libraries
- `binding.js` - Loads the native addon using `node-gyp-build` (handles prebuilds vs compiled builds)

### JavaScript Layer
- `index.js` - Main entry point. Exports `openCashDrawer` (async), `getAvailablePrinters`, and `PrinterStatus` enum
- `index.d.ts` - TypeScript type definitions

### Key Implementation Details
- ESC/POS command format: `[0x1B, 0x70, pin, pulseOnTime, pulseOffTime]`
- Virtual printers (PDF, XPS, Fax) are blocked in the native layer (`BLOCKED_VIRTUAL_PRINTERS` list)
- `openCashDrawer` is async and returns a Promise with `{success, errorCode, errorMessage}`
- `getAvailablePrinters` uses `wmic` on Windows (synchronous, Windows-only currently)

### Error Codes
Defined in both `DrawerErrorCodes` enum (C++) and `index.d.ts`:
- 0: Success
- 1000-1005: Print operation errors
- 1006: Invalid printer name (JS validation)
- 1007: Other/catch-all error
- 1008: Virtual printer blocked
