#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <arpa/inet.h>

// Convert IP string to Little Endian uint32
uint32_t ip_to_uint(const char *ip) {
    struct in_addr addr;
    inet_aton(ip, &addr);
    return addr.s_addr;
}

int main() {
    int map_fd;
    char ip_to_block[16];
    uint8_t value = 0;
    // Standard path for pinned maps
    const char *map_path = "/sys/fs/bpf/xdp_fw/blacklist";

    // 1. Connect to the pinned map
    map_fd = bpf_obj_get(map_path); 
    if (map_fd < 0) {
        fprintf(stderr, "ERROR: Failed to fetch BPF map at %s\n", map_path);
        fprintf(stderr, "Check if the XDP program is loaded and pinned correctly.\n");
        return 1;
    }

    printf("--- eBPF Sentinel Active ---\n");
    printf("Enter IP to block (or 'exit'): ");

    while (scanf("%15s", ip_to_block) && strcmp(ip_to_block, "exit") != 0) {
        uint32_t key = ip_to_uint(ip_to_block);

        // 2. Inject IP into the Kernel Map
        if (bpf_map_update_elem(map_fd, &key, &value, BPF_ANY) == 0) {
            printf("[BANNED] IP %s blocked at driver level.\n", ip_to_block);
        } else {
            perror("Failed to update BPF map");
        }
        printf("Next IP: ");
    }

    return 0;
}