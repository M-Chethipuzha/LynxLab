# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial project structure and documentation
- Codec library: wire-protocol frame struct with encode/decode, CRC32 checksum, and C unit tests
- Telemetry agent: reads /proc (CPU, memory, load), encodes metrics into codec frames, sends over TCP; CLI entry point with configurable host/port/interval/node-id; unit tests for collect/connect/send paths
- Acquisition daemon (lynxd): TCP listen on port 9710, select-based accept loop, wire-protocol frame decode with partial-read handling, metrics payload parsing and logging; unit tests for encode/decode roundtrip, TCP accept and frame processing, and clean shutdown
