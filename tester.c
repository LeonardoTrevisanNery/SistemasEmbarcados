#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define TAMANHO_MEMORIA 12

/* Ponteiro para a regiao mapeada, separado do handle. */
typedef struct {
    const char *nome;
    const char *rotulo;
    HANDLE handle;
    const char *memoria;
    char ultima[TAMANHO_MEMORIA + 1];
    int primeira_leitura;
} Player;

int main(void) {
    Player players[2] = {
        { "SMP1", "PLAYER 1", NULL, NULL, "", 1 },
        { "SMP2", "PLAYER 2", NULL, NULL, "", 1 }
    };

    printf("====================================\n");
    printf("       PSIQUE - MONITOR C\n");
    printf("====================================\n");
    printf("Aguardando as memorias dos players...\n\n");

    /* Aguarda cada Python criar sua memoria compartilhada. */
    for (int i = 0; i < 2; i++) {
        while (players[i].handle == NULL) {
            players[i].handle = OpenFileMappingA(
                FILE_MAP_READ,
                FALSE,
                players[i].nome
            );

            if (players[i].handle == NULL) {
                Sleep(250);
            }
        }

        players[i].memoria = (const char *)MapViewOfFile(
            players[i].handle,
            FILE_MAP_READ,
            0,
            0,
            TAMANHO_MEMORIA
        );

        if (players[i].memoria == NULL) {
            printf("Erro ao mapear %s: %lu\n",
                   players[i].rotulo, GetLastError());

            for (int j = 0; j <= i; j++) {
                if (players[j].memoria != NULL)
                    UnmapViewOfFile(players[j].memoria);
                if (players[j].handle != NULL)
                    CloseHandle(players[j].handle);
            }
            return 1;
        }

        printf("[CONECTADO] %s -> %s\n",
               players[i].rotulo, players[i].nome);
    }

    printf("\nMonitorando Player 1 e Player 2...\n");
    printf("Pressione CTRL+C para encerrar.\n\n");

    while (1) {
        for (int i = 0; i < 2; i++) {
            char atual[TAMANHO_MEMORIA + 1];

            memcpy(atual, players[i].memoria, TAMANHO_MEMORIA);
            atual[TAMANHO_MEMORIA] = '\0';

            /* Evita imprimir repetidamente o mesmo conteudo. */
            if (players[i].primeira_leitura ||
                memcmp(atual, players[i].ultima,
                       TAMANHO_MEMORIA + 1) != 0) {

                memcpy(players[i].ultima, atual,
                       TAMANHO_MEMORIA + 1);
                players[i].primeira_leitura = 0;

                printf("[%s] Dados recebidos: \"%s\"\n",
                       players[i].rotulo, atual);
                fflush(stdout);
            }
        }

        Sleep(100);
    }

    /* O loop e encerrado externamente (CTRL+C). */
    for (int i = 0; i < 2; i++) {
        if (players[i].memoria != NULL)
            UnmapViewOfFile(players[i].memoria);
        if (players[i].handle != NULL)
            CloseHandle(players[i].handle);
    }

    return 0;
}
