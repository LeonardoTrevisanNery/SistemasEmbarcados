// comandos:
//     /secret para escolher o objeto
//     /guess para advinhar
//     /END para finalizar 
//     <qualquer texto> para enviar mensagem de chat

// compilar e rodar:
//     gcc -Wall Jogador_Cliente.c -o Jogador_Cliente.exe -lpthread -lwsock32
//     Jogador_Cliente.exe 172.0.0.1 5000

#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FLOW_SIZE 1024

void exibirMensagem(char *linha)
{
    if (strncmp(linha, "MSG|", 4) == 0)
    {
        printf("\n[JUIZ] %s\n", linha + 4);
    }
    else if (strncmp(linha, "CHAT|", 5) == 0)
    {
        char *origem = linha + 5;
        char *sep = strchr(origem, '|');

        if (sep != NULL)
        {
            *sep = '\0';
            printf("\n[CHAT %s] %s\n", origem, sep + 1);
        }
        else
        {
            printf("\n[CHAT] %s\n", origem);
        }
    }
    else if (strncmp(linha, "RESULT|", 7) == 0)
    {
        printf("\n[RESULTADO] Palpite errado. Tente de novo.\n");
    }
    else if (strncmp(linha, "WIN|", 4) == 0)
    {
        printf("\n*** VOCE VENCEU! Acertou o objeto do oponente. ***\n");
    }
    else if (strncmp(linha, "LOSE|", 5) == 0)
    {
        printf("\n*** VOCE PERDEU! O jogador %s acertou primeiro. ***\n",
               linha + 5);
    }
    else
    {
        printf("\n%s\n", linha);
    }

    printf("> ");
    fflush(stdout);
}

void *clientThread(void *arg)
{
    int sockId = *(int *)arg;
    int recvBytes;

    char buf[MAX_FLOW_SIZE];
    char acumulado[MAX_FLOW_SIZE * 2];
    int tamAcumulado = 0;

    acumulado[0] = '\0';

    do
    {
        memset(buf, 0, sizeof(buf));
        recvBytes = recv((SOCKET)sockId, buf, MAX_FLOW_SIZE - 1, 0);

        if (recvBytes <= 0)
        {
            if (recvBytes < 0)
                printf("\nA conexao foi perdida.\n");
            else
                printf("\nEncerrando a conexao do cliente.\n");

            break;
        }

        buf[recvBytes] = '\0';

        {
            int espaco = (int)sizeof(acumulado) - tamAcumulado - 1;

            if (recvBytes > espaco)
                recvBytes = espaco;

            memcpy(acumulado + tamAcumulado, buf, recvBytes);
            tamAcumulado += recvBytes;
            acumulado[tamAcumulado] = '\0';
        }

        {
            char *inicio = acumulado;
            char *quebra;

            while ((quebra = strchr(inicio, '\n')) != NULL)
            {
                *quebra = '\0';

                // remove o '\r' que pode existir em mensagens Windows.
                inicio[strcspn(inicio, "\r")] = '\0';

                if (strlen(inicio) > 0)
                    exibirMensagem(inicio);

                inicio = quebra + 1;
            }

            tamAcumulado = (int)strlen(inicio);
            memmove(acumulado, inicio, tamAcumulado + 1);
        }

    } while (recvBytes > 0);

    closesocket((SOCKET)sockId);
    pthread_exit(NULL);

    return NULL;
}

void montarComando(const char *entrada, char *saida, int tamanho)
{
    if (strncmp(entrada, "/secret ", 8) == 0)
        snprintf(saida, tamanho, "SECRET|%s", entrada + 8);
    else if (strncmp(entrada, "/guess ", 7) == 0)
        snprintf(saida, tamanho, "GUESS|%s", entrada + 7);
    else
        snprintf(saida, tamanho, "CHAT|%s", entrada);
}

int main(int argc, char *argv[])
{
    int sockId;
    int serverPort;
    int connId;
    int sentBytes;

    char buf[MAX_FLOW_SIZE];
    char comando[MAX_FLOW_SIZE];

    struct sockaddr_in client;
    struct hostent *hp;

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("Nao foi possivel inicializar o Winsock.\n");
        return 1;
    }

    if (argc != 3)
    {
        printf("Uso: %s IP_ou_nome_do_servidor porta_do_servidor\n", argv[0]);
        WSACleanup();
        return 1;
    }

    hp = gethostbyname(argv[1]);

    if (hp == NULL)
    {
        printf("Host invalido: %s\n", argv[1]);
        WSACleanup();
        return 1;
    }

    memset(&client, 0, sizeof(client));
    memcpy((char *)&client.sin_addr, (char *)hp->h_addr, hp->h_length);
    client.sin_family = AF_INET;

    serverPort = atoi(argv[2]);

    if (serverPort <= 0 || serverPort > 65535)
    {
        printf("Porta invalida: %s\n", argv[2]);
        WSACleanup();
        return 1;
    }

    client.sin_port = htons((unsigned short)serverPort);

    sockId = (int)socket(AF_INET, SOCK_STREAM, 0);

    if (sockId == (int)INVALID_SOCKET)
    {
        printf("Socket TCP nao pode ser aberto.\n");
        WSACleanup();
        return 1;
    }

    connId = connect(
        (SOCKET)sockId,
        (struct sockaddr *)&client,
        sizeof(client)
    );

    if (connId == SOCKET_ERROR)
    {
        printf("A conexao com o servidor %s na porta %s falhou.\n",
               argv[1], argv[2]);

        closesocket((SOCKET)sockId);
        WSACleanup();

        return 1;
    }

    printf("\n==============================================\n");
    printf("        O JOGO DO PSIQUICO - JOGADOR\n");
    printf("==============================================\n");
    printf("Comandos:\n");
    printf("  /secret <objeto>  -> define seu objeto secreto\n");
    printf("  /guess  <objeto>  -> adivinha o objeto do oponente\n");
    printf("  /END              -> sai do jogo\n");
    printf("  <texto>           -> envia mensagem de chat\n");
    printf("==============================================\n\n");

    pthread_t newThread;

    if (pthread_create(&newThread, NULL, clientThread, &sockId) != 0)
    {
        printf("Nao foi possivel criar a thread de recepcao.\n");

        closesocket((SOCKET)sockId);
        WSACleanup();

        return 1;
    }

    while (1)
    {
        printf("> ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;

        buf[strcspn(buf, "\r\n")] = '\0';

        if (strlen(buf) == 0)
            continue;

        if (strcmp(buf, "/END") == 0 || strcmp(buf, "END") == 0)
        {
            send((SOCKET)sockId, "END\n", 4, 0);
            shutdown((SOCKET)sockId, SD_BOTH);
            closesocket((SOCKET)sockId);
            printf("Encerrado.\n");
            WSACleanup();

            return 0;
        }

        montarComando(buf, comando, sizeof(comando));

        {
            size_t tamanhoEnvio = strlen(comando);

            if (tamanhoEnvio + 1 < sizeof(comando))
            {
                comando[tamanhoEnvio] = '\n';
                comando[tamanhoEnvio + 1] = '\0';
            }

            sentBytes = send(
                (SOCKET)sockId,
                comando,
                (int)strlen(comando),
                0
            );
        }

        if (sentBytes == SOCKET_ERROR)
        {
            printf("A conexao foi perdida.\n");

            closesocket((SOCKET)sockId);
            WSACleanup();

            return 1;
        }
    }

    shutdown((SOCKET)sockId, SD_BOTH);
    closesocket((SOCKET)sockId);
    WSACleanup();

    return 0;
}