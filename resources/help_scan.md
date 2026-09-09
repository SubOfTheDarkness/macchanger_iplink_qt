# [02] ARP CACHE EXPLOITATION & SUBNET SWEEP

The scanner discovers local nodes without triggering firewall alerts or kernel security warnings.

### 1. Stealth Network Probing (No-Root Sweep)
* **Subnet Target:** The tool queries network masks via `QNetworkInterface` to map the active `/24` domain.
* **The Wave:** Shoots rapid, concurrent `ping -c 1 -W 1` shell processes to all 254 endpoints in parallel.
* **Execution Trick:** The app completely discards process return logs. The core goal is simply to trigger hardware echo loops.

### 2. ARP Kernel Database Harvesting
* **Mechanism:** When active nodes return ICMP echo packets, the Linux kernel sub-system automatically saves their physical bindings.
* **Reading Cache:** The toolkit extracts these active hardware translations directly from the system user-space file `/proc/net/arp`.
