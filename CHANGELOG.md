# Changelog

All notable changes to NetworkMonitor will be documented in this file.

## [1.1.0] - 2025-12-06

### Added
- **Ping Monitor**: Real-time latency display on overlay with color coding
  - Green (<100ms), Yellow (100-200ms), Red (>200ms or timeout)
  - Configurable ping target (IP/domain) and interval
- **Keyboard Shortcut**: `Win+Shift+N` to toggle overlay (customizable)
- **Connection Notifications**: Balloon notification on network connect/disconnect
- **Settings UI**: New Advanced section for ping and hotkey configuration

### Changed
- Enhanced CI workflow with Debug/Release matrix builds
- Updated README with new features

## [1.0.0] - 2025-11-22

### Added
- Initial release
- System tray icon with traffic monitoring
- Taskbar overlay showing download/upload speeds
- Dashboard with today/monthly statistics
- History logging to SQLite
- Settings dialog with language support (EN/VI)
- Dark theme support
- Auto-start with Windows option
