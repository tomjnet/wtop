# Changelog

All notable changes to wtop are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-09-29

### Added

- Task Manager style performance screens for CPU, Memory, every disk, every
  network adapter and every GPU, each with a 60-second braille history graph
  and a statistics panel.
- Resource sidebar with live mini graphs on wide terminals.
- Editor-style tab line, tmux-style status line and `Ctrl+b` key bindings
  (`n`, `p`, `l`, `0`-`9`, `w`, `?`, `d`).
- htop-style function key bar with `F1` (help) and `F10` (quit).
- `--delay`, `--tab`, `--snapshot`, `--help` and `--version` options.
- `wtop(1)` man page.
- NVIDIA GPU support through `nvidia-smi`, AMD and Intel through `/sys/class/drm`.
- `install.sh` installer that needs no root: downloads the release binary,
  or builds from source.
- Static release binaries for x86_64 and aarch64, published by GitHub Actions
  with SHA-256 checksums and build provenance attestations.

[Unreleased]: https://github.com/tomjnet/wtop/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/tomjnet/wtop/releases/tag/v0.1.0
