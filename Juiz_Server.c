/*
 *                  Juiz_Server.c
 *
 * Este programa foi desenvolvido para simular uma aplicacao servidora
 * multithread baseada no "Jogo do Psiquico".
 *
 * Funcao:     Receber e retornar streams compostos de caracteres de
 *             multiplas conexoes de clientes, aplicando as regras do
 *             Jogo do Psiquico (2 jogadores, objeto secreto, palpites
 *             simultaneos e chat).
 * Plataforma: Windows ou CodeBlocks com MinGW
 * Compilar:   gcc -Wall Juiz_Server.c -o Juiz_Server.exe -lpthread -lws2_32
 * Uso:        Juiz_Server.exe [porta_do_servidor]
 *
 * Autor:      Jose Martins Junior (estrutura base)
 *             Adaptado para o Jogo do Psiquico
 *
 * Baseado em:
 *   - ServidorMonoTCP.c  (Jose Martins Junior)
 *   - ServidorMultiTCP.c (Jose Martins Junior)
 *
 * PROTOCOLO (uma mensagem por linha, terminada em '\n')
 *
 *   Cliente -> Juiz:
 *     SECRET|<objeto>   define o objeto secreto do jogador
 *     GUESS|<objeto>    palpite do objeto do oponente
 *     CHAT|<texto>      mensagem de chat
 *     END               encerra a conexao
 *
 *   Juiz -> Cliente:
 *     MSG|<texto>           mensagem/estado do servidor
 *     CHAT|J<n>|<texto>     chat recebido do jogador n
 *     RESULT|MISS           o ultimo palpite errou
 *     WIN|<n>               o jogador n venceu (voce ganhou)
 *     LOSE|<n>              o jogador n venceu (voce perdeu)
 *
 *   Observacao: o acerto NAO e enviado como RESULT|HIT; ele e
 *   comunicado diretamente por WIN|<n> (vencedor) e LOSE|<n>
 *   (perdedor).
 */

#include <sys/types.h>
#include <winsock2.h>
#include <unistd.h>

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define QUEUE_LENGTH   5      // Tamanho maximo da fila de conexoes de clientes
#define MAX_FLOW_SIZE  1000   // Tamanho maximo do buffer de caracteres
#define MAX_PLAYERS    2
#define MAX_OBJ_LEN    128
#define true 1

/*
 * =========================
 * ESTADO DO JUIZ
 * =========================
 */

pthread_mutex_t mut1;  // Mutex global que protege o estado dos jogadores
int threadId;          // Id das threads - variavel global - condicao de disputa

/*
 * Slots dos jogadores (1 e 2 -> indices 0 e 1).
 */
typedef struct
{
    int  ativo;                     /* slot ocupado?               */
    int  sockId;                    /* socket do jogador           */
    int  temSegredo;                /* ja definiu o objeto?        */
    char segredo[MAX_OBJ_LEN + 1];  /* objeto secreto (minusculo)  */
} Jogador;

Jogador jogadores[MAX_PLAYERS];

int jogoFinalizado = 0;

/*
 * =========================
 * UTILIDADES
 * =========================
 */

/*
 * Normaliza um objeto/palpite:
 *   - remove espacos no inicio e no fim;
 *   - converte para minusculas.
 * Comparacao fica case-insensitive e tolerante a espacos nas pontas.
 */
void normalizar(const char *entrada, char *saida, int tamanho)
{
    int inicio = 0;
    int fim;

    while (entrada[inicio] != '\0' &&
           (entrada[inicio] == ' ' || entrada[inicio] == '\t'))
        inicio++;

    fim = (int)strlen(entrada) - 1;

    while (fim >= inicio &&
           (entrada[fim] == ' ' || entrada[fim] == '\t'))
        fim--;

    int n = 0;

    for (int i = inicio; i <= fim && n < tamanho - 1; i++)
        saida[n++] = (char)tolower((unsigned char)entrada[i]);

    saida[n] = '\0';
}

/*
 * =========================
 * ENVIO DE MENSAGENS
 * =========================
 */

int enviarMensagem(int sockId, const char *mensagem)
{
    int total = (int)strlen(mensagem);
    int enviado = 0;

    while (enviado < total)
    {
        int ret = send(sockId, mensagem + enviado, total - enviado, 0);

        if (ret <= 0)
            return 0;

        enviado += ret;
    }

    return 1;
}

/*
 * Envia uma mensagem garantindo que ela termine com '\n'.
 */
int enviarLinha(int sockId, const char *mensagem)
{
    int tam = (int)strlen(mensagem);

    if (tam > 0 && mensagem[tam - 1] == '\n')
        return enviarMensagem(sockId, mensagem);

    char linha[MAX_FLOW_SIZE + 2];
    snprintf(linha, sizeof(linha), "%s\n", mensagem);
    return enviarMensagem(sockId, linha);
}

/* Envia a todos os jogadores ativos (ja deve estar sem o mutex). */
void broadcast(const char *mensagem)
{
    pthread_mutex_lock(&mut1);

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (jogadores[i].ativo)
            enviarLinha(jogadores[i].sockId, mensagem);
    }

    pthread_mutex_unlock(&mut1);
}

/* Numero de jogadores conectados (chamar com o mutex travado). */
int contarAtivos()
{
    int n = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (jogadores[i].ativo)
            n++;
    return n;
}

/* Todos definiram o segredo? (chamar com o mutex travado). */
int todosComSegredo()
{
    if (contarAtivos() < MAX_PLAYERS)
        return 0;

    for (int i = 0; i < MAX_PLAYERS; i++)
        if (jogadores[i].ativo && !jogadores[i].temSegredo)
            return 0;

    return 1;
}

/*
 * =========================
 * THREAD DO CLIENTE
 * =========================
 */

void *servThread(void *arg)   // Inicio da funcao que cada thread executarah
{
    int Id, recvBytes;
    char buf[MAX_FLOW_SIZE];

    int connId = *(int *)arg;

    pthread_mutex_lock(&mut1);   // Inicio de uma regiao critica
    Id = ++threadId;
    pthread_mutex_unlock(&mut1); // Final de uma regiao critica

    printf("Thread %d - abrindo conexao\n", Id);

    int meuSlot = -1;
    int meuNumero = 0;      /* 1 ou 2 */

    /* Reserva um slot de jogador. */
    pthread_mutex_lock(&mut1);

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!jogadores[i].ativo)
        {
            meuSlot = i;
            break;
        }
    }

    if (meuSlot < 0)
    {
        pthread_mutex_unlock(&mut1);

        enviarLinha(connId,
            "MSG|A partida ja tem 2 jogadores. Tente mais tarde.\n");

        close(connId);
        return NULL;  // Encerramento da thread
    }

    jogadores[meuSlot].ativo = 1;
    jogadores[meuSlot].sockId = connId;
    jogadores[meuSlot].temSegredo = 0;
    jogadores[meuSlot].segredo[0] = '\0';

    meuNumero = meuSlot + 1;

    pthread_mutex_unlock(&mut1);

    printf("Thread %d - Jogador %d conectou.\n", Id, meuNumero);

    /* Identificacao e instrucoes iniciais. */
    {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "MSG|Voce e o JOGADOR %d. "
                 "Defina seu objeto secreto com SECRET.\n",
                 meuNumero);
        enviarLinha(connId, msg);
    }

    do {

/*
 *******************************************************************************
               Recebendo as mensagens do cliente
 *******************************************************************************
 */
        memset(buf, 0, sizeof(buf));

        recvBytes = recv(connId, buf, MAX_FLOW_SIZE - 1, 0);

        if (recvBytes <= 0) {
            if (recvBytes < 0)
                printf("Thread %d - A conexao foi perdida\n", Id);
            else
                printf("Thread %d - Encerrando a conexao do cliente\n", Id);

            break;
        }
/*----------------------------------------------------------------------------*/

        else {

/*
 *******************************************************************************
               Processando as mensagens do cliente
 *******************************************************************************
 */
            buf[recvBytes] = '\0';
            buf[strcspn(buf, "\r\n")] = '\0';

            if (strcmp(buf, "END") == 0)
                break;

            /* ---- SECRET|<objeto> ---- */
            if (strncmp(buf, "SECRET|", 7) == 0)
            {
                char objeto[MAX_OBJ_LEN + 1];
                normalizar(buf + 7, objeto, sizeof(objeto));

                if (strlen(objeto) == 0)
                {
                    enviarLinha(connId,
                        "MSG|Objeto invalido. Tente novamente.\n");
                    continue;
                }

                pthread_mutex_lock(&mut1);

                strcpy(jogadores[meuSlot].segredo, objeto);
                jogadores[meuSlot].temSegredo = 1;

                int prontos = todosComSegredo();

                pthread_mutex_unlock(&mut1);

                printf("Thread %d - Jogador %d definiu o objeto secreto.\n",
                       Id, meuNumero);

                enviarLinha(connId,
                    "MSG|Objeto secreto registrado.\n");

                if (prontos)
                    broadcast("MSG|Os dois objetos foram definidos. "
                              "Comecem a adivinhar com GUESS!\n");
                else
                    enviarLinha(connId,
                        "MSG|Aguardando o outro jogador definir o objeto...\n");

                continue;
            }

            /* ---- GUESS|<objeto> ---- */
            if (strncmp(buf, "GUESS|", 6) == 0)
            {
                char palpite[MAX_OBJ_LEN + 1];
                normalizar(buf + 6, palpite, sizeof(palpite));

                pthread_mutex_lock(&mut1);

                if (jogoFinalizado)
                {
                    pthread_mutex_unlock(&mut1);
                    enviarLinha(connId, "MSG|A partida ja terminou.\n");
                    continue;
                }

                if (!todosComSegredo())
                {
                    pthread_mutex_unlock(&mut1);
                    enviarLinha(connId,
                        "MSG|Aguarde: os dois objetos ainda nao foram definidos.\n");
                    continue;
                }

                int oponente = (meuSlot == 0) ? 1 : 0;
                int acertou = (strcmp(palpite, jogadores[oponente].segredo) == 0);

                if (acertou)
                    jogoFinalizado = 1;

                pthread_mutex_unlock(&mut1);

                printf("Thread %d - Jogador %d chutou [%s] -> %s\n",
                       Id, meuNumero, palpite, acertou ? "ACERTOU" : "errou");

                if (acertou)
                {
                    char msgWin[64];
                    char msgLose[64];

                    snprintf(msgWin, sizeof(msgWin), "WIN|%d\n", meuNumero);
                    snprintf(msgLose, sizeof(msgLose), "LOSE|%d\n", meuNumero);

                    pthread_mutex_lock(&mut1);   // Inicio de uma regiao critica

                    for (int i = 0; i < MAX_PLAYERS; i++)
                    {
                        if (!jogadores[i].ativo)
                            continue;

                        if (i == meuSlot)
                            enviarLinha(jogadores[i].sockId, msgWin);
                        else
                            enviarLinha(jogadores[i].sockId, msgLose);
                    }

                    pthread_mutex_unlock(&mut1); // Final de uma regiao critica
                }
                else
                {
                    enviarLinha(connId, "RESULT|MISS\n");
                }

                continue;
            }

            /* ---- CHAT|<texto> ---- */
            if (strncmp(buf, "CHAT|", 5) == 0)
            {
                const char *texto = buf + 5;

                char msg[MAX_FLOW_SIZE];
                snprintf(msg, sizeof(msg), "CHAT|J%d|%s\n", meuNumero, texto);

                pthread_mutex_lock(&mut1);   // Inicio de uma regiao critica

                for (int i = 0; i < MAX_PLAYERS; i++)
                {
                    if (jogadores[i].ativo && i != meuSlot)
                        enviarLinha(jogadores[i].sockId, msg);
                }

                pthread_mutex_unlock(&mut1); // Final de uma regiao critica

                continue;
            }

            /* Comando desconhecido. */
            enviarLinha(connId,
                "MSG|Comando invalido. Use SECRET|, GUESS| ou CHAT|.\n");
/*----------------------------------------------------------------------------*/
        }
    } while (recvBytes > 0);

    /* Limpeza do slot. */
    pthread_mutex_lock(&mut1);

    jogadores[meuSlot].ativo = 0;
    jogadores[meuSlot].sockId = 0;
    jogadores[meuSlot].temSegredo = 0;
    jogadores[meuSlot].segredo[0] = '\0';

    pthread_mutex_unlock(&mut1);

    /* Avisa o outro jogador que o oponente saiu. */
    {
        char msg[64];
        snprintf(msg, sizeof(msg),
                 "MSG|O jogador %d saiu da partida.\n", meuNumero);
        broadcast(msg);
    }

    close(connId);

    printf("Thread %d - Jogador %d - conexao encerrada\n", Id, meuNumero);

    return NULL;  // Encerramento da thread
} // Final da funcao que cada thread executarah

/*
 * =========================
 * MAIN
 * =========================
 */

int main(int argc, char *argv[])
{
    int sockId, bindRet, getRet, connId, serverPort;
    int servLen, cliLen;
    struct sockaddr_in server, client;

    WSADATA wsaData;
    WSAStartup(0x0101, &wsaData);

    pthread_mutex_init(&mut1, NULL);

    memset(jogadores, 0, sizeof(jogadores));

    if (argc != 2)
    {
        printf("Uso: %s porta_do_servidor\n", argv[0]);
        pthread_mutex_destroy(&mut1);
        WSACleanup();
        return 1;
    }

    serverPort = atoi(argv[1]);

    if (serverPort <= 0)
    {
        printf("Porta Invalida: %s\n", argv[1]);
        pthread_mutex_destroy(&mut1);
        WSACleanup();
        return 1;
    }

/*
 *******************************************************************************
               Abrindo um socket do tipo stream (TCP)
 *******************************************************************************
 */
    sockId = socket(AF_INET, SOCK_STREAM, 0);
    if (sockId < 0) {
        printf("Stream socket nao pode ser aberto\n");
        return(1);
    }
/*----------------------------------------------------------------------------*/

/*
 *******************************************************************************
               Setando os atributos da estrutura do socket
 *******************************************************************************
 */
    memset(&server, 0, sizeof(server));
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port        = htons(serverPort);
/*----------------------------------------------------------------------------*/

/*
 *******************************************************************************
               Fazendo o bind para o stream socket aberto
 *******************************************************************************
 */
    bindRet = bind(sockId, (struct sockaddr *)&server, sizeof(server));
    if (bindRet < 0) {
        printf("O bind para o stream socket falhou\n");
        close(sockId);
        return(1);
    }
/*----------------------------------------------------------------------------*/

/*
 *******************************************************************************
               Obtendo o nome do socket
 *******************************************************************************
 */
    servLen = sizeof(server);
    getRet  = getsockname(sockId, (struct sockaddr *)&server, &servLen);
    if (getRet < 0) {
        printf("Nao foi possivel obter o nome do socket\n");
        close(sockId);
        return(1);
    }
/*----------------------------------------------------------------------------*/

    printf("\n=====================================\n");
    printf("     O JOGO DO PSIQUICO - JUIZ\n");
    printf("=====================================\n");
    printf("Porta do servidor: %d\n", ntohs(server.sin_port));
    printf("Aguardando 2 jogadores...\n");
    printf("=====================================\n\n");

/*
 *******************************************************************************
               Colocando o servidor em modo listening
 *******************************************************************************
 */
    listen(sockId, QUEUE_LENGTH);
/*----------------------------------------------------------------------------*/

    do {

/*
 *******************************************************************************
               Habilitando o servidor de receber conexoes
 *******************************************************************************
 */
        cliLen = sizeof(client);
        connId = accept(sockId, (struct sockaddr *)&client, &cliLen);

        if (connId < 0)
            printf("O socket nao pode aceitar conexoes\n");
/*----------------------------------------------------------------------------*/

        else {  // Criacao de uma thread para atender cada cliente
            int *novoConnId = (int *)malloc(sizeof(int));

            if (novoConnId == NULL) {
                printf("Erro ao alocar memoria para a thread\n");
                close(connId);
                continue;
            }

            *novoConnId = connId;

            pthread_t newThread;

            if (pthread_create(&newThread, 0, (void *)servThread, novoConnId) != 0)
            {
                printf("Nao foi possivel criar a thread\n");
                free(novoConnId);
                close(connId);
                continue;
            }

            /* A thread e independente: libera seus recursos ao terminar. */
            pthread_detach(newThread);
        }
    } while (true);

    close(sockId);
    pthread_mutex_destroy(&mut1);
    WSACleanup();
    return(0);
}