# NetworkMonitor - Feature Suggestions

## 🚀 Proposed Features

### 1. Data Usage Alerts ⭐ High Priority
- Alert when exceeding data threshold (e.g., 10GB/day, 100GB/month)
- Toast/balloon notification when approaching limit
- Useful for 4G/5G users with data caps

### 2. Export/Import History
- Export history to CSV/Excel for analysis
- Import data from backup
- Sync between multiple machines (optional)

### 3. Per-Application Bandwidth Monitor
- Monitor bandwidth per app (Chrome, Discord, Games...)
- Use Windows Filtering Platform (WFP) or ETW events
- Display top bandwidth-consuming apps

### 4. Real-time Network Speed Graph
- Real-time graph in Dashboard (currently static chart)
- Live graph updating every second
- Zoom in/out timeline

### 5. Ping/Latency Monitor
- Display ping to gateway or DNS (8.8.8.8)
- Alert when latency is high
- Useful for gamers

### 6. Multiple Monitor Support
- Allow selecting which monitor to display overlay
- Support taskbar on multiple monitors

### 7. Keyboard Shortcuts
- Hotkey to toggle overlay (Win+Shift+N)
- Quick toggle on/off monitoring

### 8. Connection Status Notification
- Notify when internet connection is lost
- Auto-detect and log when network goes down/up

### 9. Widget/Gadget Mode
- Compact floating widget on desktop
- Always-on-top with transparency
- Drag and drop positioning

### 10. Tray Icon Customization
- Display speed numbers directly on icon
- Choose icon color based on speed

---

## 💡 Priority Matrix

| Priority | Feature | Reason |
|----------|---------|--------|
| 🔴 High | Data Usage Alerts | Very useful, leverages existing SQLite history |
| 🔴 High | Ping Monitor | Easy to implement, high value |
| 🟡 Medium | Export History | Utilizes already logged data |
| 🟡 Medium | Desktop Widget | Better UX than overlay |
| 🟢 Low | Per-App Monitor | Complex, requires WFP driver |

---

*Created: 2024-12-05*
