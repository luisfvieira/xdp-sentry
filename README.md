# 🚀 XDP-Sentry: High-Performance Kernel-Level Networking Defense

A low-latency, kernel-level firewall built with **C**, **eBPF**, and **XDP**. This project demonstrates how to intercept and drop malicious network traffic at the driver level, bypassing the heavy lifting of the standard Linux networking stack.



## 🏗️ System Architecture

This project implements a **Split-Plane Architecture**, decoupling high-speed packet processing from security policy management:

1.  **Data Plane (Kernel Space):** An eBPF program attached to the **XDP (eXpress Data Path)** hook. It performs L2/L3 parsing and packet dropping in nanoseconds.
2.  **Control Plane (User Space):** A C application that interacts with the Data Plane via **BPF Maps**, allowing for real-time updates to the IP blacklist without reloading the kernel program.

---

## 📂 Repository Structure

| File | Role | Technical Description |
| :--- | :--- | :--- |
| `vmlinux.h` | **Kernel Definitions** | Internal kernel structures generated via BTF for CO-RE compliance. |
| `xdp_fw.bpf.c` | **Kernel Program** | eBPF source code implementing L3 filtering logic and $O(1)$ hash map lookups. |
| `xdp_fw.bpf.o` | **BPF Bytecode** | Compiled ELF object containing eBPF instructions. |
| `sentry.c` | **Control Plane** | Management interface CLI to dynamically push blacklisted IPs into the kernel map. |
| `Makefile` | **Build System** | Automates compilation of both BPF bytecode and Userspace binary. |

---

## ⚡ Performance: Why XDP?

Traditional firewalls like `iptables` process packets after the kernel has already allocated an `sk_buff` (socket buffer). This is costly during high-volume DDoS attacks.

**XDP-Sentry** operates at the earliest possible point:
* **Zero-Copy:** Packets are processed directly in the RX ring buffer.
* **Constant Time:** Blacklist lookups use BPF Hash Maps with $O(1)$ complexity.
* **Driver-Level Execution:** By returning `XDP_DROP`, CPU cycles are saved, preventing interrupt storms and protecting system stability.



---

## 🛠️ Build and Run

### Prerequisites
* Ubuntu 22.04 LTS (Kernel 5.15+)
* `clang`, `llvm`, `libbpf-dev`, `bpftool`

### 1. Build the project
Simply run `make` to compile both the Kernel-space BPF bytecode and the User-space Sentinel tool:
```bash
make
```

### 2. Deploy to Interface

To load the program into the `lo` (loopback) interface:

```bash
sudo make load
```

### 3. Run the Sentinel

```bash
sudo ./sentinel
```

### 4. Cleanup

To detach the program and remove build artifacts:

```bash
sudo make unload
make clean
```