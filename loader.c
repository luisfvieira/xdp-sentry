#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <linux/if_link.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <interface_name>\n", argv[0]);
        return 1;
    }

    const char *ifname = argv[1];
    int ifindex = if_nametoindex(ifname);
    if (ifindex == 0) {
        fprintf(stderr, "ERROR: Interface %s not found\n", ifname);
        return 1;
    }

    // Load BPF object
    struct bpf_object *obj = bpf_object__open_file("xdp_fw.bpf.o", NULL);
    if (libbpf_get_error(obj)) {
        fprintf(stderr, "ERROR: Failed to open BPF object\n");
        return 1;
    }

    if (bpf_object__load(obj)) {
        fprintf(stderr, "ERROR: Failed to load BPF object into kernel\n");
        return 1;
    }

    // Find the program within the object
    struct bpf_program *prog = bpf_object__find_program_by_name(obj, "xdp_firewall");
    int fd = bpf_program__fd(prog);

    // Attempt to attach (Try Native first, then Generic/SKB)
    int err = bpf_xdp_attach(ifindex, fd, XDP_FLAGS_DRV_MODE, NULL);
    if (err) {
        printf("Native mode not supported, falling back to Generic (SKB) mode...\n");
        err = bpf_xdp_attach(ifindex, fd, XDP_FLAGS_SKB_MODE, NULL);
    }

    if (err) {
        fprintf(stderr, "ERROR: Could not attach XDP program to %s\n", ifname);
        return 1;
    }

    printf("SUCCESS: XDP-Sentry deployed on %s\n", ifname);
    printf("To deactivate: sudo ip link set dev %s xdp off\n", ifname);

    return 0;
}