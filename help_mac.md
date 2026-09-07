# [01] MAC INTERFACE SPOOFER KERNEL LOGIC

The spoofer manipulates the Linux kernel network stack using privileged bash commands executed via `pkexec`.

### 1. Kernel Link State Cycle
* **Link Down:** `ip link set dev [iface] down`
  Flushes routing descriptors and tears down virtual `IFF_UP` flags. Disconnects the device from layer-2 broadcasts.
* **Hardware Registration:** `ip link set dev [iface] address [mac]`
  Writes the 48-bit hardware address directly into the kernel's virtual `net_device` configuration structure.
* **Link Up:** `ip link set dev [iface] up`
  Re-initializes queuing disciplines (`qdisc`), clears socket buffers, and broadcasts a fresh DHCP discovery loop.

### 2. Random Address Bitmask Allocation
The random generator strictly configures the first octet using bitmasks `0x02`, `0x06`, `0x0A`, `0x0E`:
* **U/L Bit (Universal/Local):** Forced to 1 (Flags address as Locally Administered).
* **I/G Bit (Individual/Group):** Forced to 0 (Flags address as Unicast).
* **Result:** Prevents enterprise switches and routers from dropping spoofed frames.
