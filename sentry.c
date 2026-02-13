#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>

// Função para converter string IP "127.0.0.1" para o formato do Kernel
uint32_t ip_to_uint(const char *ip) {
    uint32_t a, b, c, d;
    sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d);
    return (d << 24) | (c << 16) | (b << 8) | a;
}

int main() {
    int map_fd;
    char ip_to_block[16];
    uint8_t value = 0;

    // 1. Conectar ao mapa 'blacklist' que já está no Kernel (ID 5 no seu caso)
    // Dica acadêmica: No mundo real, usaríamos o nome do mapa ou o caminho no /sys/fs/bpf
    map_fd = bpf_obj_get("/sys/fs/bpf/xdp_fw/blacklist"); 
    if (map_fd < 0) {
        printf("Erro: Certifique-se de que o programa XDP está carregado e pinado!\n");
        return 1;
    }

    printf("--- eBPF Sentry Ativo ---\n");
    printf("Digite um IP para banir instantaneamente (ou 'exit'): ");

    while (scanf("%15s", ip_to_block) && strcmp(ip_to_block, "exit") != 0) {
        uint32_t key = ip_to_uint(ip_to_block);

        // 2. Injetar o IP no mapa do Kernel
        if (bpf_map_update_elem(map_fd, &key, &value, BPF_ANY) == 0) {
            printf("[BANIDO] IP %s bloqueado no nível de Driver!\n", ip_to_block);
        } else {
            perror("Erro ao atualizar mapa");
        }
        printf("Próximo IP: ");
    }

    return 0;
}