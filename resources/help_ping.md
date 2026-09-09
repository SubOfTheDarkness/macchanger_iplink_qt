# [03] LATENCY TELEMETRY & BG GUARDIAN DAEMON

The diagnostic engine tracks connection drops, standard deviations, and stability windows.

### 1. Telemetry Engineering
* **Jitter Control:** Computes the mathematical standard deviation of RTT to evaluate bufferbloat on local lines.
* **Auto-Scale Eng:** Monitored metrics scale automatically. If connection latency ticks past 9999 ms, display fields upscale raw integers to floating-point seconds (`14.25 s`) to shield the layout.

### 2. Background Heartbeat Guardian
* **Fail-Safe Loop:** A high-precision `QTimer` acts as a connection drop interceptor.
* **Tray Broker:** If a host times out, the daemon flags system widgets (`danger=true`), modifies CSS, and sends non-spammy alerts to the system tray panel.
