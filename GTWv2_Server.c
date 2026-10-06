/*
 *                  GTWv2.c
 *
 * GTW como SERVIDOR TCP MULTITHREAD.
 *
 * Baseado na estrutura de:
 *   - ServidorMultiAtivosTCP.c
 *   - ClienteMultiTCP.c
 *
 * O servidor:
 *   - aceita varios clientes usando uma thread por conexao;
 *   - atribui jogador 1 ou 2 aos dois primeiros clientes;
 *   - recebe a tentativa enviada pelo ClienteMultiTCP;
 *   - avalia a palavra;
 *   - atualiza o estado do jogo;
 *   - envia o painel atualizado aos clientes.
 *
 * Compilar (MinGW):
 *   gcc -Wall GTWv2.c -o GTWv2.exe -lpthread -lws2_32
 *
 * Executar:
 *   GTWv2.exe 5000
 *
 * Cliente:
 *   ClienteMultiTCP.exe 127.0.0.1 5000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <winsock2.h>
#include <unistd.h>
#include <pthread.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_WORDS 1000
#define WORD_SIZE 5

#define QUEUE_LENGTH 5
#define MAX_FLOW_SIZE 1000
#define MAX_ATIVOS 10
#define MAX_PLAYERS 2

#define RESET   "\033[0m"
#define GREEN   "\033[42m\033[30m"
#define YELLOW  "\033[43m\033[30m"
#define RED     "\033[41m\033[37m"
#define GRAY    "\033[100m\033[37m"

#define true 1

/*
 * =========================
 * VARIAVEIS DO GTW
 * =========================
 */

char Vetor[WORD_SIZE + 1];

char avaliableWords[MAX_WORDS][WORD_SIZE + 1];

int player = 1;

char screen[WORD_SIZE] = {
    ' ', ' ', ' ', ' ', ' '
};

/*
 * 0 = vazio
 * 1 = vermelho
 * 2 = amarelo
 * 3 = verde
 */
int colors[WORD_SIZE] = {
    0, 0, 0, 0, 0
};

/*
 * =========================
 * VARIAVEIS DO SERVIDOR
 * =========================
 */

pthread_mutex_t mut1;

int threadId = 0;
int ativos = 0;

int conexoes[MAX_ATIVOS];

/*
 * Jogador associado a cada conexao.
 * 0 = espectador/sem jogador
 * 1 = jogador 1
 * 2 = jogador 2
 */
int jogadores[MAX_ATIVOS];

/*
 * Controle do jogo.
 */
int jogadorConectado[MAX_PLAYERS + 1] = {0, 0, 0};
int jogoFinalizado = 0;

/*
 * =========================
 * FUNCOES ORIGINAIS DO GTW
 * =========================
 */

void enableANSI()
{
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hOut == INVALID_HANDLE_VALUE)
        return;

    DWORD dwMode = 0;

    if (!GetConsoleMode(hOut, &dwMode))
        return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    SetConsoleMode(hOut, dwMode);
#endif
}

void printBox(char letter, int color)
{
    if (color == 3)
        printf(GREEN " %c " RESET, letter);
    else if (color == 2)
        printf(YELLOW " %c " RESET, letter);
    else if (color == 1)
        printf(RED " %c " RESET, letter);
    else
        printf(GRAY "   " RESET);
}

void pannel()
{
    printf("\n");

    printf("__________________________________\n");
    printf("|                                |\n");

    printf("|          G T W                 |\n");
    printf("|       GUESS THE WORD           |\n");

    printf("|                                |\n");

    printf("|     Vez do jogador %d           |\n", player);

    printf("|                                |\n");

    printf("|     ");

    for (int i = 0; i < WORD_SIZE; i++)
    {
        printBox(screen[i], colors[i]);

        if (i < WORD_SIZE - 1)
            printf(" ");
    }

    printf("     |\n");

    printf("|                                |\n");
    printf("__________________________________\n");

    printf("\n");

    printf(" " GREEN "   " RESET " Correta     ");
    printf(YELLOW "   " RESET " Existe      ");
    printf(RED "   " RESET " Errada\n");

    printf("\n");
}

int loadWordlist()
{
    FILE *file = fopen("wordlist.txt", "r");

    if (file == NULL)
    {
        printf("Erro ao abrir o arquivo wordlist.txt\n");
        return 0;
    }

    char line[100];
    int quantidade = 0;

    while (quantidade < MAX_WORDS &&
           fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\r\n")] = '\0';

        if (strlen(line) == WORD_SIZE)
        {
            strcpy(avaliableWords[quantidade], line);
            quantidade++;
        }
    }

    fclose(file);

    return quantidade;
}

void sortWord(int quantidade)
{
    int sorteada = rand() % quantidade;

    for (int i = 0; i < WORD_SIZE; i++)
        Vetor[i] = avaliableWords[sorteada][i];

    Vetor[WORD_SIZE] = '\0';
}

void evaluateWord(const char *Choice)
{
    /*
     * Primeiro verifica letras na posicao correta.
     */
    for (int i = 0; i < WORD_SIZE; i++)
    {
        colors[i] = 1;

        if (Choice[i] == Vetor[i])
            colors[i] = 3;
    }

    /*
     * Depois verifica letras que existem
     * em outras posicoes.
     */
    for (int i = 0; i < WORD_SIZE; i++)
    {
        if (colors[i] == 3)
            continue;

        for (int j = 0; j < WORD_SIZE; j++)
        {
            if (Choice[i] == Vetor[j])
            {
                colors[i] = 2;
                break;
            }
        }
    }

    /*
     * Copia a palavra para o painel.
     */
    for (int i = 0; i < WORD_SIZE; i++)
        screen[i] = Choice[i];
}

int checkWin(const char *Choice)
{
    for (int i = 0; i < WORD_SIZE; i++)
    {
        if (Choice[i] != Vetor[i])
            return 0;
    }

    return 1;
}

/*
 * =========================
 * FUNCOES DO SERVIDOR
 * =========================
 */

/*
 * Monta o painel como texto para enviar ao cliente.
 *
 * O ClienteMultiTCP nao precisa conhecer as regras do GTW:
 * ele simplesmente recebe a mensagem e imprime.
 */
void montarPainel(char *saida, int tamanho)
{
    char linha[256];

    saida[0] = '\0';

    snprintf(linha, sizeof(linha),
             "\n"
             "__________________________________\n"
             "|                                |\n"
             "|          G T W                 |\n"
             "|       GUESS THE WORD           |\n"
             "|                                |\n"
             "|     Vez do jogador %d           |\n"
             "|                                |\n"
             "|     ",
             player);

    strncat(saida, linha, tamanho - strlen(saida) - 1);

    /*
     * No cliente, ANSI tambem sera interpretado pelo terminal.
     */
    for (int i = 0; i < WORD_SIZE; i++)
    {
        char box[32];

        if (colors[i] == 3)
            snprintf(box, sizeof(box), GREEN " %c " RESET, screen[i]);
        else if (colors[i] == 2)
            snprintf(box, sizeof(box), YELLOW " %c " RESET, screen[i]);
        else if (colors[i] == 1)
            snprintf(box, sizeof(box), RED " %c " RESET, screen[i]);
        else
            snprintf(box, sizeof(box), GRAY "   " RESET);

        strncat(saida, box, tamanho - strlen(saida) - 1);

        if (i < WORD_SIZE - 1)
            strncat(saida, " ", tamanho - strlen(saida) - 1);
    }

    strncat(saida,
            "     |\n"
            "|                                |\n"
            "__________________________________\n"
            "\n"
            " " GREEN "   " RESET " Correta     "
            YELLOW "   " RESET " Existe      "
            RED "   " RESET " Errada\n",
            tamanho - strlen(saida) - 1);
}

/*
 * Envia uma mensagem para um socket.
 */
int enviarMensagem(int sockId, const char *mensagem)
{
    int total = (int)strlen(mensagem);
    int enviado = 0;

    while (enviado < total)
    {
        int ret = send(sockId,
                       mensagem + enviado,
                       total - enviado,
                       0);

        if (ret <= 0)
            return 0;

        enviado += ret;
    }

    return 1;
}

/*
 * Envia o estado atual para todos os clientes conectados.
 *
 * A lista de conexoes e protegida pelo mutex.
 */
void broadcast(const char *mensagem)
{
    pthread_mutex_lock(&mut1);

    for (int i = 0; i < MAX_ATIVOS; i++)
    {
        if (conexoes[i] != 0)
        {
            if (!enviarMensagem(conexoes[i], mensagem))
            {
                printf("Falha ao enviar para a conexao %d\n",
                       conexoes[i]);
            }
        }
    }

    pthread_mutex_unlock(&mut1);
}

/*
 * Encontra uma posicao livre no vetor de conexoes.
 */
int encontrarPosicaoLivre()
{
    for (int i = 0; i < MAX_ATIVOS; i++)
    {
        if (conexoes[i] == 0)
            return i;
    }

    return -1;
}

/*
 * Encontra o primeiro jogador livre.
 */
int encontrarJogadorLivre()
{
    for (int i = 1; i <= MAX_PLAYERS; i++)
    {
        if (!jogadorConectado[i])
            return i;
    }

    return 0;
}

/*
 * =========================
 * THREAD DO CLIENTE
 * =========================
 */

void *servThread(void *arg)
{
    int connId = *(int *)arg;
    free(arg);

    int Id;
    int recvBytes;
    int minhaPosicao = -1;
    int meuJogador = 0;

    char buf[MAX_FLOW_SIZE];

    /*
     * Entrada na regiao critica.
     *
     * Aqui a thread recebe:
     *   - seu ID;
     *   - sua posicao na lista;
     *   - seu numero de jogador.
     */
    pthread_mutex_lock(&mut1);

    ativos++;

    minhaPosicao = encontrarPosicaoLivre();

    if (minhaPosicao >= 0)
        conexoes[minhaPosicao] = connId;

    Id = ++threadId;

    meuJogador = encontrarJogadorLivre();

    if (meuJogador != 0)
        jogadorConectado[meuJogador] = 1;

    if (meuJogador == 0)
    {
        pthread_mutex_unlock(&mut1);

        enviarMensagem(
            connId,
            "Servidor: os dois jogadores ja estao conectados.\n"
            "Esta conexao nao pode participar desta partida.\n");

        close(connId);
        pthread_exit(0);
    }

    jogadores[minhaPosicao] = meuJogador;

    pthread_mutex_unlock(&mut1);

    printf("Thread %d - Jogador %d abriu conexao\n",
           Id, meuJogador);

    /*
     * Mensagem inicial.
     */
    {
        char mensagem[MAX_FLOW_SIZE];

        snprintf(mensagem, sizeof(mensagem),
                 "\nVoce e o JOGADOR %d.\n"
                 "Adivinhe a palavra de 5 letras.\n",
                 meuJogador);

        enviarMensagem(connId, mensagem);
    }

    /*
     * Envia o painel inicial.
     */
    {
        char painel[MAX_FLOW_SIZE];

        pthread_mutex_lock(&mut1);
        montarPainel(painel, sizeof(painel));
        pthread_mutex_unlock(&mut1);

        enviarMensagem(connId, painel);
    }

    /*
     * Loop da thread.
     */
    do
    {
        memset(buf, 0, sizeof(buf));

        recvBytes = recv(connId,
                         buf,
                         MAX_FLOW_SIZE - 1,
                         0);

        if (recvBytes <= 0)
        {
            if (recvBytes < 0)
                printf("Thread %d - A conexao foi perdida\n", Id);
            else
                printf("Thread %d - Encerrando a conexao\n", Id);

            break;
        }

        buf[recvBytes] = '\0';

        /*
         * Remove CR/LF caso existam.
         */
        buf[strcspn(buf, "\r\n")] = '\0';

        printf("Thread %d - Jogador %d enviou: [%s]\n",
               Id, meuJogador, buf);

        /*
         * END nao precisa ser enviado pelo cliente atual,
         * pois o ClienteMultiTCP fecha o socket localmente.
         * Este tratamento fica por seguranca.
         */
        if (strcmp(buf, "END") == 0)
            break;

        /*
         * So palavras de exatamente 5 caracteres sao aceitas.
         */
        if (strlen(buf) != WORD_SIZE)
        {
            enviarMensagem(
                connId,
                "Digite exatamente 5 letras!\n");

            continue;
        }

        /*
         * Converte para minusculas.
         */
        for (int i = 0; i < WORD_SIZE; i++)
        {
            if (buf[i] >= 'A' && buf[i] <= 'Z')
                buf[i] += 32;
        }

        /*
         * Tudo que altera o estado do jogo fica dentro do mutex.
         * Isso evita que duas threads alterem o jogo ao mesmo tempo.
         */
        pthread_mutex_lock(&mut1);

        if (jogoFinalizado)
        {
            pthread_mutex_unlock(&mut1);

            enviarMensagem(
                connId,
                "\nA partida ja terminou.\n");

            continue;
        }

        /*
         * Confere se realmente e a vez deste jogador.
         */
        if (player != meuJogador)
        {
            pthread_mutex_unlock(&mut1);

            enviarMensagem(
                connId,
                "\nAinda nao e a sua vez!\n");

            continue;
        }

        /*
         * Avalia a tentativa.
         */
        evaluateWord(buf);

        /*
         * Guarda se venceu antes de trocar o jogador.
         */
        if (checkWin(buf))
        {
            char resultado[MAX_FLOW_SIZE];

            jogoFinalizado = 1;

            snprintf(resultado, sizeof(resultado),
                     "\n"
                     GREEN "        PARABENS! "
                     "Jogador %d acertou!\n" RESET
                     "        Palavra: %s\n",
                     meuJogador,
                     Vetor);

            /*
             * Primeiro libera o mutex porque broadcast()
             * tambem usa o mutex.
             */
            pthread_mutex_unlock(&mut1);

            broadcast(resultado);

            /*
             * Envia tambem o painel final.
             */
            {
                char painel[MAX_FLOW_SIZE];

                pthread_mutex_lock(&mut1);
                montarPainel(painel, sizeof(painel));
                pthread_mutex_unlock(&mut1);

                broadcast(painel);
            }

            break;
        }

        /*
         * Troca o jogador.
         */
        if (player == 1)
            player = 2;
        else
            player = 1;

        /*
         * Monta o novo painel enquanto o estado
         * ainda esta protegido.
         */
        {
            char painel[MAX_FLOW_SIZE];

            montarPainel(painel, sizeof(painel));

            pthread_mutex_unlock(&mut1);

            /*
             * Todos os clientes recebem a mesma atualizacao.
             */
            broadcast(painel);
        }

    } while (recvBytes > 0);

    /*
     * Remove a conexao da lista de ativos.
     */
    pthread_mutex_lock(&mut1);

    ativos--;

    if (minhaPosicao >= 0 &&
        minhaPosicao < MAX_ATIVOS &&
        conexoes[minhaPosicao] == connId)
    {
        conexoes[minhaPosicao] = 0;
        jogadores[minhaPosicao] = 0;
    }

    if (meuJogador >= 1 && meuJogador <= MAX_PLAYERS)
        jogadorConectado[meuJogador] = 0;

    pthread_mutex_unlock(&mut1);

    close(connId);

    printf("Thread %d - conexao encerrada\n", Id);

    pthread_exit(0);
}

/*
 * =========================
 * MAIN DO SERVIDOR
 * =========================
 */

int main(int argc, char *argv[])
{
    int sockId;
    int bindRet;
    int getRet;
    int connId;
    int serverPort;

    unsigned int servLen;
    unsigned int cliLen;

    struct sockaddr_in server;
    struct sockaddr_in client;

    WSADATA wsaData;

    /*
     * Inicializa Winsock.
     */
    if (WSAStartup(0x0101, &wsaData) != 0)
    {
        printf("Erro ao inicializar Winsock\n");
        return 1;
    }

    enableANSI();

    /*
     * Inicializa mutex.
     */
    pthread_mutex_init(&mut1, NULL);

    /*
     * Inicializa vetores.
     */
    memset(conexoes, 0, sizeof(conexoes));
    memset(jogadores, 0, sizeof(jogadores));

    /*
     * Inicializa sorteio.
     */
    srand((unsigned int)time(NULL));

    /*
     * Carrega wordlist.
     */
    {
        int quantidade = loadWordlist();

        if (quantidade == 0)
        {
            printf("Nenhuma palavra valida encontrada.\n");

            pthread_mutex_destroy(&mut1);
            WSACleanup();

            return 1;
        }

        sortWord(quantidade);
    }

    /*
     * Testa porta.
     */
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
        printf("Porta invalida: %s\n", argv[1]);

        pthread_mutex_destroy(&mut1);
        WSACleanup();

        return 1;
    }

    /*
     * Cria socket TCP.
     */
    sockId = socket(AF_INET, SOCK_STREAM, 0);

    if (sockId < 0)
    {
        printf("Stream socket nao pode ser aberto\n");

        pthread_mutex_destroy(&mut1);
        WSACleanup();

        return 1;
    }

    /*
     * Configura endereco.
     */
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(serverPort);

    /*
     * Bind.
     */
    bindRet = bind(sockId,
                   (struct sockaddr *)&server,
                   sizeof(server));

    if (bindRet < 0)
    {
        printf("O bind para o stream socket falhou\n");

        close(sockId);
        pthread_mutex_destroy(&mut1);
        WSACleanup();

        return 1;
    }

    /*
     * Descobre o endereco/porta.
     */
    servLen = sizeof(server);

    getRet = getsockname(sockId,
                         (struct sockaddr *)&server,
                         &servLen);

    if (getRet < 0)
    {
        printf("Nao foi possivel obter o nome do socket\n");

        close(sockId);
        pthread_mutex_destroy(&mut1);
        WSACleanup();

        return 1;
    }

    printf("\n=====================================\n");
    printf("          GTW SERVER TCP\n");
    printf("=====================================\n");
    printf("Porta do servidor: %d\n",
           ntohs(server.sin_port));
    printf("Aguardando 2 jogadores...\n");
    printf("=====================================\n\n");

    /*
     * Listen.
     */
    listen(sockId, QUEUE_LENGTH);

    /*
     * Aceita conexoes continuamente.
     */
    do
    {
        cliLen = sizeof(client);

        connId = accept(sockId,
                        (struct sockaddr *)&client,
                        &cliLen);

        if (connId < 0)
        {
            printf("O socket nao pode aceitar conexoes\n");
        }
        else
        {
            /*
             * Verifica limite de conexoes.
             */
            pthread_mutex_lock(&mut1);

            if (ativos >= MAX_ATIVOS)
            {
                pthread_mutex_unlock(&mut1);

                enviarMensagem(
                    connId,
                    "Limite maximo de clientes excedido.\n");

                close(connId);

                continue;
            }

            pthread_mutex_unlock(&mut1);

            /*
             * IMPORTANTE:
             *
             * Nao passamos &connId diretamente para a thread,
             * porque o loop de accept() pode alterar connId antes
             * de a thread ler seu argumento.
             *
             * Criamos uma copia dinamica para cada thread.
             */
            int *novoConnId = (int *)malloc(sizeof(int));

            if (novoConnId == NULL)
            {
                printf("Erro ao alocar memoria para a thread\n");
                close(connId);
                continue;
            }

            *novoConnId = connId;

            /*
             * Uma thread por cliente, como no
             * ServidorMultiAtivosTCP.c.
             */
            pthread_t newThread;

            if (pthread_create(&newThread,
                               NULL,
                               servThread,
                               novoConnId) != 0)
            {
                printf("Nao foi possivel criar a thread\n");

                free(novoConnId);
                close(connId);

                continue;
            }

            /*
             * A thread e independente do main.
             */
            pthread_detach(newThread);
        }

    } while (true);

    close(sockId);

    pthread_mutex_destroy(&mut1);
    WSACleanup();

    return 0;
}
