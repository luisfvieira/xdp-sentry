#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <linux/if_link.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: %s <interface>\n", argv[0]);
        return 1;
    }

    const char *ifname = argv[1];
    int ifindex = if_nametoindex(ifname);
    
    // 1. Carregar o objeto BPF
    struct bpf_object *obj = bpf_object__open_file("xdp_fw.bpf.o", NULL);
    if (libbpf_get_error(obj)) return 1;

    if (bpf_object__load(obj)) return 1;

    // 2. Encontrar o programa XDP no objeto
    struct bpf_program *prog = bpf_object__find_program_by_name(obj, "xdp_firewall");
    int fd = bpf_program__fd(prog);

    // 3. Tentar Attach (Primeiro Nativo, depois Genérico)
    // XDP_FLAGS_DRV = Nativo | XDP_FLAGS_SKB_MODE = Genérico
    int err = bpf_xdp_attach(ifindex, fd, XDP_FLAGS_DRV_MODE, NULL);
    if (err) {
        printf("Modo Nativo não suportado, tentando Genérico...\n");
        err = bpf_xdp_attach(ifindex, fd, XDP_FLAGS_SKB_MODE, NULL);
    }

    if (err) {
        printf("Erro ao anexar XDP na interface %s\n", ifname);
        return 1;
    }

    printf("Sucesso! Firewall XDP ativo na interface %s\n", ifname);
    printf("Para desativar: sudo ip link set dev %s xdp off\n", ifname);

    return 0;
}