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
load:
	sudo bpftool prog load $(BPF_OBJ) /sys/fs/bpf/xdp_fw
	sudo bpftool net attach xdpgeneric pinned /sys/fs/bpf/xdp_fw dev lo

# Helper to unload
unload:
	sudo bpftool net detach xdpgeneric dev lo
	sudo rm -f /sys/fs/bpf/xdp_fw

.PHONY: all clean load unload