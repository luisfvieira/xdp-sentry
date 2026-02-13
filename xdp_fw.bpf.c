#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

/* Definição da constante do protocolo IPv4 (0x0800) */
#ifndef ETH_P_IP
#define ETH_P_IP 0x0800
#endif

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10000);
    __type(key, u32);   // Endereço IP
    __type(value, u8);  // Valor dummy
} blacklist SEC(".maps");

SEC("xdp")
int xdp_firewall(struct xdp_md *ctx) {
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;

    // 1. Camada Ethernet
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;

    // Verificação do protocolo IPv4
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;

    // 2. Camada IP
    struct iphdr *iph = (void *)(eth + 1);
    if ((void *)(iph + 1) > data_end)
        return XDP_PASS;

    // 3. Verificação na Blacklist
    u32 src_ip = iph->saddr;
    u8 *blocked = bpf_map_lookup_elem(&blacklist, &src_ip);
    
    if (blocked) {
        return XDP_DROP; // Bloqueio imediato no driver
    }

    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";