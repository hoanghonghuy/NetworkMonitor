# Changelog

All notable changes to this project will be documented in this file.

The format roughly follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project aims to follow [Semantic Versioning](https://semver.org/) where practical.

## [v1.0.0-healthcheck1] - 2025-11-23

### Added
- Soft logging support with debug logging toggle and an "Open log file" button in Settings.
- Basic CMake-driven test suite covering:
  - HistoryLogger (totals, recent samples, trim).
  - NetworkMonitorClass (lifecycle: start/stop, IsRunning).
  - Utils (FormatBytes/FormatSpeed and conversions).
  - NetworkCalculator.
  - ConfigManager (registry round-trip and auto-start).
  - TrayIcon and TaskbarOverlay (sanity init/update/cleanup).
- GitHub Actions CI workflow to build and run tests on Windows.

### Changed
- Cleaned up legacy/unused code and includes (removed old `main_old.cpp`, unused members and headers).
- Adjusted CMake configuration to build SQLite cleanly under `/WX` on MSVC.

### Notes
- This tag serves as a health-check baseline for the codebase and CI.
