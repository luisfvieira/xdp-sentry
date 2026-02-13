# Variables
CLANG = clang
CFLAGS = -O2 -g -Wall
BPF_CFLAGS = -target bpf -D__TARGET_ARCH_x86 -I/usr/include/bpf

# Target names
BPF_OBJ = xdp_fw.bpf.o
LOADER_EXE = sentinel

# Default target
all: $(BPF_OBJ) $(LOADER_EXE)

# Compile the eBPF program
$(BPF_OBJ): xdp_fw.bpf.c
	$(CLANG) $(CFLAGS) $(BPF_CFLAGS) -c $< -o $@

# Compile the Userspace Sentinel/Loader
$(LOADER_EXE): sentry.c
	$(CLANG) $(CFLAGS) -o $@ $< -lbpf

# Clean build artifacts
clean:
	rm -f $(BPF_OBJ) $(LOADER_EXE)

# Helper to load the program (Replace 'lo' with your interface)
load: all
	# 1. Clean up old artifacts to prevent conflicts
	sudo rm -rf /sys/fs/bpf/xdp_fw
	sudo mkdir -p /sys/fs/bpf/xdp_fw
	
	# 2. Load the program (pinning the program itself)
	sudo bpftool prog load $(BPF_OBJ) /sys/fs/bpf/xdp_fw/obj_fw
	
	# 3. Dynamic Pinning Logic
	@MAP_ID=$$(sudo bpftool map show | grep blacklist | head -n 1 | cut -d: -f1); \
	if [ -z "$$MAP_ID" ]; then \
		echo "ERROR: Could not find blacklist map ID"; \
		exit 1; \
	fi; \
	echo "Found Blacklist Map ID: $$MAP_ID"; \
	sudo bpftool map pin id $$MAP_ID /sys/fs/bpf/xdp_fw/blacklist
	
	# 4. Attach to the interface
	sudo bpftool net attach xdpgeneric name xdp_firewall dev lo
	@echo "SUCCESS: Firewall loaded and map pinned at /sys/fs/bpf/xdp_fw/blacklist"

unload:
	sudo bpftool net detach xdpgeneric dev lo || true
	sudo rm -rf /sys/fs/bpf/xdp_fw
	@echo "SUCCESS: Firewall detached and pins removed."

# ... (previous targets: all, sentinel, load, unload)

status:
	@echo "==========================================="
	@echo "📊 XDP-SENTRY OPERATIONAL STATUS"
	@echo "==========================================="
	@echo "\n1. XDP Program Attachment:"
	@sudo bpftool net list | grep xdp || echo "   [!] No XDP programs currently attached."
	
	@echo "\n2. Blacklisted IPs (Kernel Map):"
	@if [ -f /sys/fs/bpf/xdp_fw/blacklist ]; then \
		sudo bpftool map dump pinned /sys/fs/bpf/xdp_fw/blacklist | \
		grep -v "found 0 elements" || echo "   [ ] Blacklist is currently empty."; \
	else \
		echo "   [!] Blacklist map not found in BPF FS."; \
	fi
	
	@echo "\n3. Interface Stats (lo):"
	@ip -s link show lo | grep -A 1 "RX:"
	@echo "==========================================="

.PHONY: all clean load unload status