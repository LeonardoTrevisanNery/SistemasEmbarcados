/*
 *                  Jogador_Cliente.c
 *
 * "O Jogo do Psiquico" - CLIENTE (JOGADOR) TCP.
 *
 * Baseado na estrutura de:
 *   - ClienteMultiTCP.c (Jose Martins Junior)
 *
 * O cliente:
 *   - conecta ao juiz (servidor);
 *   - uma thread recebe e exibe as mensagens do juiz;
 *   - o programa principal le comandos do usuario e envia ao juiz.
 *
 * COMANDOS DO USUARIO (digitados no terminal):
 *   /secret <objeto>   define seu objeto secreto
 *   /guess  <objeto>   tenta adivinhar o objeto do oponente
 *   /END               encerra o jogo
 *   <qualquer texto>   enviado como mensagem de chat para o oponente
 *
 * Esses comandos sao traduzidos para o protocolo do juiz:
 *   /secret x  -> SECRET|x
 *   /guess  x  -> GUESS|x
 *   texto      -> CHAT|texto
 *
 * Compilar (MinGW no Windows):
 *   gcc -Wall Jogador_Cliente.c -o Jogador_Cliente.exe -lpthread -lwsock32
 *
 * Uso:
 *   Jogador_Cliente.exe 127.0.0.1 5000
 */

#include <sys/types.h>
#include <winsock2.h>
#include <unistd.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FLOW_SIZE 1024

/*
 * Processa uma linha recebida do juiz e imprime de forma amigavel.
 * Formato das mensagens: TIPO|...  (ver protocolo no servidor).
 */
void exibirMensagem(char *linha)
{
    if (strncmp(linha, "MSG|", 4) == 0)
    {
        printf("\n[JUIZ] %s\n", linha + 4);
    }
    else if (strncmp(linha, "CHAT|", 5) == 0)
    {
        /* CHAT|J<n>|<texto> */
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
        /*
         * O juiz envia RESULT|MISS para palpite errado.
         * O acerto nao vem como RESULT: vem como WIN/LOSE.
         */
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

/*
 * Thread de recepcao.
 *
 * O protocolo e orientado a linhas (cada mensagem termina em '\n'),
 * mas o TCP e um fluxo continuo: uma unica chamada de recv() pode
 * trazer varias mensagens juntas OU uma mensagem partida ao meio.
 *
 * Para tratar isso corretamente usamos um buffer acumulador:
 *   - os bytes recebidos sao concatenados em 'acumulado';
 *   - processamos apenas as linhas COMPLETAS (ate um '\n');
 *   - o resto (uma linha ainda incompleta) permanece no buffer
 *     ate o proximo recv() completar.
 */
void *clientThread(void *arg)
{
    int sockId = *(int *)arg;
    int recvBytes;

    char buf[MAX_FLOW_SIZE];
    char acumulado[MAX_FLOW_SIZE * 2];
    int  tamAcumulado = 0;

    acumulado[0] = '\0';

    do
    {
        memset(buf, 0, sizeof(buf));

        recvBytes = recv(sockId, buf, MAX_FLOW_SIZE - 1, 0);

        if (recvBytes <= 0)
        {
            if (recvBytes < 0)
                printf("\nA conexao foi perdida\n");
            else
                printf("\nEncerrando a conexao do cliente\n");

            break;
        }

        buf[recvBytes] = '\0';

        /*
         * Concatena o que chegou ao buffer acumulado.
         * Se nao couber, descartamos o excesso para nao estourar
         * (improvavel neste jogo, mas evita overflow).
         */
        int espaco = (int)sizeof(acumulado) - tamAcumulado - 1;

        if (recvBytes > espaco)
            recvBytes = espaco;

        memcpy(acumulado + tamAcumulado, buf, recvBytes);
        tamAcumulado += recvBytes;
        acumulado[tamAcumulado] = '\0';

        /*
         * Processa todas as linhas completas (terminadas em '\n').
         */
        char *inicio = acumulado;
        char *quebra;

        while ((quebra = strchr(inicio, '\n')) != NULL)
        {
            *quebra = '\0';

            /* Remove possivel '\r' no fim da linha. */
            inicio[strcspn(inicio, "\r")] = '\0';

            if (strlen(inicio) > 0)
                exibirMensagem(inicio);

            inicio = quebra + 1;
        }

        /*
         * O que sobrou (linha incompleta) vai para o inicio do
         * buffer, aguardando o restante no proximo recv().
         */
        tamAcumulado = (int)strlen(inicio);
        memmove(acumulado, inicio, tamAcumulado + 1);

    } while (recvBytes > 0);

    close(sockId);
    pthread_exit(0);
}

/*
 * Converte um comando amigavel do usuario para o protocolo do juiz:
 *   /secret x -> SECRET|x
 *   /guess  x -> GUESS|x
 *   outro     -> CHAT|outro
 */
void montarComando(const char *entrada, char *saida, int tamanho)
{
    if (strncmp(entrada, "/secret ", 8) == 0)
        snprintf(saida, tamanho, "SECRET|%s", entrada + 8);
    else if (strncmp(entrada, "/guess ", 7) == 0)
        snprintf(saida, tamanho, "GUESS|%s", entrada + 7);
    else
        snprintf(saida, tamanho, "CHAT|%s", entrada); /* chat por padrao */
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
    WSAStartup(0x0101, &wsaData);

    if (argc != 3)
    {
        printf("Uso: %s IP_ou_nome_do_servidor porta_do_servidor\n",
               argv[0]);
        return 1;
    }

    hp = gethostbyname(argv[1]);

    if (((char *)hp) == NULL)
    {
        printf("Host invalido: %s\n", argv[1]);
        return 1;
    }

    memcpy((char *)&client.sin_addr, (char *)hp->h_addr, hp->h_length);
    client.sin_family = AF_INET;

    serverPort = atoi(argv[2]);

    if (serverPort <= 0)
    {
        printf("Porta invalida: %s\n", argv[2]);
        return 1;
    }

    client.sin_port = htons(serverPort);

    sockId = socket(AF_INET, SOCK_STREAM, 0);

    if (sockId < 0)
    {
        printf("Stream socket nao pode ser aberto\n");
        return 1;
    }

    connId = connect(sockId, (struct sockaddr *)&client, sizeof client);

    if (connId == -1)
    {
        printf("A conexao com o servidor %s na porta %s falhou\n",
               argv[1], argv[2]);
        close(sockId);
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
    pthread_create(&newThread, 0, clientThread, &sockId);

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
            send(sockId, "END", 3, 0);
            close(sockId);
            printf("Encerrado\n");
            return 0;
        }

        montarComando(buf, comando, sizeof(comando));

        sentBytes = send(sockId, comando, strlen(comando), 0);

        if (sentBytes < 0)
        {
            close(sockId);
            printf("A conexao foi perdida\n");
            return 1;
        }
    }

    close(sockId);
    return 0;
}
