/*
 *                  Jogador_Cliente.c
 *
 * "O Jogo do Psiquico" - CLIENTE (JOGADOR) TCP.
 *
 * Baseado na estrutura dos exemplos:
 *   - ClienteMonoTCP.c (Jose Martins Junior)
 *   - ClienteMultiTCP.c (Jose Martins Junior)
 *
 * Funcao:
 *   Enviar e receber mensagens por sockets TCP, permitindo que dois jogadores
 *   conversem e tentem adivinhar o objeto secreto um do outro.
 *
 * Plataforma: Windows com MinGW, usando Winsock e pthreads.
 *
 * O cliente:
 *   - conecta ao juiz (servidor);
 *   - cria uma thread para receber e exibir mensagens do juiz;
 *   - o programa principal le comandos do usuario e os envia ao juiz.
 *
 * COMANDOS DO USUARIO:
 *   /secret <objeto>   define seu objeto secreto
 *   /guess  <objeto>   tenta adivinhar o objeto do oponente
 *   /END               encerra o jogo
 *   <qualquer texto>   envia mensagem de chat para o oponente
 *
 * Traducao para o protocolo do juiz:
 *   /secret x  -> SECRET|x
 *   /guess  x  -> GUESS|x
 *   texto      -> CHAT|texto
 *
 * Compilar (MinGW com suporte a pthreads):
 *   gcc -Wall Jogador_Cliente.c -o Jogador_Cliente.exe -lpthread -lwsock32
 *
 * Uso:
 *   Jogador_Cliente.exe 127.0.0.1 5000
 */

#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FLOW_SIZE 1024

/*
 * ============================================================================
 * Processa uma mensagem recebida do juiz e a exibe de forma amigavel.
 *
 * O protocolo utiliza mensagens no formato TIPO|dados.
 * ============================================================================
 */
void exibirMensagem(char *linha)
{
    if (strncmp(linha, "MSG|", 4) == 0)
    {
        printf("\n[JUIZ] %s\n", linha + 4);
    }
    else if (strncmp(linha, "CHAT|", 5) == 0)
    {
        /* Formato: CHAT|J<n>|<texto> */
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
        /* O juiz envia RESULT|MISS quando o palpite esta errado. */
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
 * ============================================================================
 * Thread de recepcao das mensagens do servidor.
 *
 * O TCP e um fluxo continuo: uma chamada recv() pode receber varias mensagens
 * juntas ou somente parte de uma mensagem. Por isso, as mensagens sao
 * separadas por '\n' e processadas apenas quando a linha esta completa.
 * ============================================================================
 */
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
        /*
         * ================================================================
         * Recebendo as mensagens do servidor.
         * ================================================================
         */
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

        /*
         * Acumula os bytes recebidos. O limite evita ultrapassar
         * o tamanho reservado para o buffer.
         */
        {
            int espaco = (int)sizeof(acumulado) - tamAcumulado - 1;

            if (recvBytes > espaco)
                recvBytes = espaco;

            memcpy(acumulado + tamAcumulado, buf, recvBytes);
            tamAcumulado += recvBytes;
            acumulado[tamAcumulado] = '\0';
        }

        /*
         * Processa todas as mensagens completas, terminadas em '\n'.
         */
        {
            char *inicio = acumulado;
            char *quebra;

            while ((quebra = strchr(inicio, '\n')) != NULL)
            {
                *quebra = '\0';

                /* Remove o '\r' que pode existir em mensagens Windows. */
                inicio[strcspn(inicio, "\r")] = '\0';

                if (strlen(inicio) > 0)
                    exibirMensagem(inicio);

                inicio = quebra + 1;
            }

            /*
             * Preserva no inicio do buffer a mensagem ainda incompleta.
             */
            tamAcumulado = (int)strlen(inicio);
            memmove(acumulado, inicio, tamAcumulado + 1);
        }

    } while (recvBytes > 0);

    closesocket((SOCKET)sockId);
    pthread_exit(NULL);

    return NULL;
}

/*
 * ============================================================================
 * Converte o comando digitado pelo jogador para o protocolo do juiz.
 *
 * /secret objeto -> SECRET|objeto
 * /guess objeto  -> GUESS|objeto
 * qualquer texto -> CHAT|texto
 * ============================================================================
 */
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

    /*
     * =========================================================================
     * Inicializacao do Winsock (biblioteca de sockets do Windows).
     * =========================================================================
     */
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("Nao foi possivel inicializar o Winsock.\n");
        return 1;
    }

    /*
     * =========================================================================
     * Testando os argumentos passados na chamada.
     * O programa espera o endereco do servidor e a porta TCP.
     * =========================================================================
     */
    if (argc != 3)
    {
        printf("Uso: %s IP_ou_nome_do_servidor porta_do_servidor\n", argv[0]);
        WSACleanup();
        return 1;
    }

    /*
     * =========================================================================
     * Setando e testando o endereco informado (primeiro argumento).
     * =========================================================================
     */
    hp = gethostbyname(argv[1]);

    if (hp == NULL)
    {
        printf("Host invalido: %s\n", argv[1]);
        WSACleanup();
        return 1;
    }

    /*
     * =========================================================================
     * Setando os atributos da estrutura do socket.
     * =========================================================================
     */
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

    /*
     * =========================================================================
     * Abrindo um socket do tipo stream (TCP).
     * =========================================================================
     */
    sockId = (int)socket(AF_INET, SOCK_STREAM, 0);

    if (sockId == (int)INVALID_SOCKET)
    {
        printf("Socket TCP nao pode ser aberto.\n");
        WSACleanup();
        return 1;
    }

    /*
     * =========================================================================
     * Conectando o socket ao servidor (juiz).
     * =========================================================================
     */
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

    /*
     * =========================================================================
     * Criando a thread que recebe mensagens do servidor em paralelo.
     * =========================================================================
     */
    pthread_t newThread;

    if (pthread_create(&newThread, NULL, clientThread, &sockId) != 0)
    {
        printf("Nao foi possivel criar a thread de recepcao.\n");

        closesocket((SOCKET)sockId);
        WSACleanup();

        return 1;
    }

    /*
     * =========================================================================
     * Leitura dos comandos do usuario e envio das mensagens ao servidor.
     * =========================================================================
     */
    while (1)
    {
        printf("> ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;

        buf[strcspn(buf, "\r\n")] = '\0';

        if (strlen(buf) == 0)
            continue;

        /*
         * Condicao de parada: envia END ao servidor para encerrar a sessao.
         */
        if (strcmp(buf, "/END") == 0 || strcmp(buf, "END") == 0)
        {
            send((SOCKET)sockId, "END\n", 4, 0);
            shutdown((SOCKET)sockId, SD_BOTH);
            closesocket((SOCKET)sockId);
            printf("Encerrado.\n");
            WSACleanup();

            return 0;
        }

        /*
         * Monta a mensagem de acordo com o protocolo definido pelo jogo.
         */
        montarComando(buf, comando, sizeof(comando));

        /*
         * Enviando as mensagens para o servidor.
         * As mensagens sao terminadas em '\n' para delimitar cada linha.
         */
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