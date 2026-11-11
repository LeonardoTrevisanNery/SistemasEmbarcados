// compilar e rodar:
//     gcc -Wall Jogo_Psiquico.c -o Jogo_Psiquico.exe -lws2_32
//     Jogo_Psiquico.exe


#include <winsock2.h>
#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define EXE_JUIZ        "Juiz_Server.exe"
#define EXE_CLIENTE     "Jogador_Cliente.exe"
#define SRC_JUIZ        "Juiz_Server.c"
#define SRC_CLIENTE     "Jogador_Cliente.c"

#define PORTA_PADRAO    5000
#define IP_LOCAL        "127.0.0.1"
#define ESPERA_JUIZ_MS  1000   
#define ESPERA_JOG_MS   500    

#define COR_NORMAL      7
#define COR_TITULO      11
#define COR_OPCAO       14
#define COR_OK          10
#define COR_ERRO        12
#define COR_DICA        8

static char g_dir[MAX_PATH];  

static void cor(WORD c)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

static void limparTela(void)
{
    system("cls");
}

static int lerLinha(const char *prompt, char *buf, int tamanho)
{
    cor(COR_OPCAO);
    printf("%s", prompt);
    cor(COR_NORMAL);
    fflush(stdout);

    if (fgets(buf, tamanho, stdin) == NULL)
        return 0;

    buf[strcspn(buf, "\r\n")] = '\0';

    {
        char *ini = buf;
        int fim;

        while (*ini == ' ' || *ini == '\t')
            ini++;

        memmove(buf, ini, strlen(ini) + 1);

        fim = (int)strlen(buf) - 1;
        while (fim >= 0 && (buf[fim] == ' ' || buf[fim] == '\t'))
            buf[fim--] = '\0';
    }

    return 1;
}

static void pausar(void)
{
    char tmp[16];
    cor(COR_DICA);
    printf("\nPressione ENTER para voltar ao menu...");
    cor(COR_NORMAL);
    fflush(stdout);
    fgets(tmp, sizeof(tmp), stdin);
}

static void mostrarBanner(void)
{
    cor(COR_TITULO);
    printf("\n");
    printf("  ==============================================\n");
    printf("            O  J O G O   D O   P S I Q U I C O\n");
    printf("  ==============================================\n");
    cor(COR_NORMAL);
}

static int arquivoExiste(const char *nome)
{
    char caminho[MAX_PATH * 2];
    snprintf(caminho, sizeof(caminho), "%s\\%s", g_dir, nome);
    return GetFileAttributesA(caminho) != INVALID_FILE_ATTRIBUTES;
}

static int hostValido(const char *host)
{
    if (host[0] == '\0' || strlen(host) > 100)
        return 0;

    for (const char *p = host; *p; p++)
    {
        if (!isalnum((unsigned char)*p) && *p != '.' && *p != '-' && *p != '_')
            return 0;
    }

    return 1;
}

static int lerPorta(void)
{
    char buf[32];
    char prompt[64];
    int porta;

    snprintf(prompt, sizeof(prompt), "Porta TCP [%d]: ", PORTA_PADRAO);

    if (!lerLinha(prompt, buf, sizeof(buf)))
        return 0;

    if (buf[0] == '\0')
        return PORTA_PADRAO;

    porta = atoi(buf);

    if (porta < 1024 || porta > 65535)
    {
        cor(COR_ERRO);
        printf("Porta invalida (use um valor entre 1024 e 65535).\n");
        cor(COR_NORMAL);
        return 0;
    }

    return porta;
}

static int portaLivre(int porta)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    int ok;

    if (s == INVALID_SOCKET)
        return 0;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)porta);

    ok = (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == 0);
    closesocket(s);

    return ok;
}

static void listarIPs(void)
{
    char nome[256];
    struct hostent *he;

    if (gethostname(nome, sizeof(nome)) != 0 ||
        (he = gethostbyname(nome)) == NULL)
    {
        printf("  (nao foi possivel descobrir o IP desta maquina)\n");
        return;
    }

    for (int i = 0; he->h_addr_list[i] != NULL; i++)
    {
        struct in_addr a;
        memcpy(&a, he->h_addr_list[i], sizeof(a));
        printf("  -> %s\n", inet_ntoa(a));
    }
}

static int compilarArquivo(const char *fonte, const char *exe, const char *libs)
{
    char cmd[1024];
    int ret;

    if (!arquivoExiste(fonte))
    {
        cor(COR_ERRO);
        printf("Fonte nao encontrado: %s\n", fonte);
        cor(COR_NORMAL);
        return 0;
    }

    snprintf(cmd, sizeof(cmd),
             "gcc -Wall \"%s\" -o \"%s\" -lpthread %s", fonte, exe, libs);

    printf("> %s\n", cmd);
    ret = system(cmd);

    if (ret != 0)
    {
        cor(COR_ERRO);
        printf("Falha ao compilar %s (gcc esta no PATH?).\n", fonte);
        cor(COR_NORMAL);
        return 0;
    }

    cor(COR_OK);
    printf("%s gerado com sucesso.\n", exe);
    cor(COR_NORMAL);
    return 1;
}

static int compilarTudo(void)
{
    int a = compilarArquivo(SRC_JUIZ, EXE_JUIZ, "-lws2_32");
    int b = compilarArquivo(SRC_CLIENTE, EXE_CLIENTE, "-lwsock32");
    return a && b;
}

static int garantirExecutaveis(int precisaJuiz, int precisaCliente)
{
    int faltaJuiz    = precisaJuiz    && !arquivoExiste(EXE_JUIZ);
    int faltaCliente = precisaCliente && !arquivoExiste(EXE_CLIENTE);
    char r[16];

    if (!faltaJuiz && !faltaCliente)
        return 1;

    cor(COR_ERRO);
    if (faltaJuiz)    printf("Nao encontrei %s\n", EXE_JUIZ);
    if (faltaCliente) printf("Nao encontrei %s\n", EXE_CLIENTE);
    cor(COR_NORMAL);

    if (!lerLinha("Compilar agora a partir dos fontes? (s/n): ", r, sizeof(r)) ||
        tolower((unsigned char)r[0]) != 's')
        return 0;

    if (faltaJuiz && !compilarArquivo(SRC_JUIZ, EXE_JUIZ, "-lws2_32"))
        return 0;

    if (faltaCliente && !compilarArquivo(SRC_CLIENTE, EXE_CLIENTE, "-lwsock32"))
        return 0;

    return 1;
}

static HANDLE criarJob(void)
{
    HANDLE job = CreateJobObjectA(NULL, NULL);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;

    if (job == NULL)
        return NULL;

    memset(&info, 0, sizeof(info));
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                 &info, sizeof(info)))
    {
        CloseHandle(job);
        return NULL;
    }

    return job;
}

static HANDLE lancar(HANDLE job, const char *titulo, const char *exe,
                     const char *args)
{
    char cmd[2048];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    snprintf(cmd, sizeof(cmd),
             "cmd.exe /S /C \"title %s & \"%s\\%s\" %s & echo. & pause\"",
             titulo, g_dir, exe, args);

    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
                        CREATE_NEW_CONSOLE | CREATE_SUSPENDED,
                        NULL, g_dir, &si, &pi))
    {
        cor(COR_ERRO);
        printf("Nao foi possivel iniciar %s (erro %lu).\n", exe, GetLastError());
        cor(COR_NORMAL);
        return NULL;
    }

    if (job != NULL)
        AssignProcessToJobObject(job, pi.hProcess);

    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);

    return pi.hProcess;
}

static void esperarPartida(HANDLE job, HANDLE *jogadores, int n)
{
    cor(COR_DICA);
    if (n > 0)
        printf("\nPartida em andamento. Feche as janelas dos jogadores ou "
               "pressione uma tecla aqui para encerrar o Juiz.\n");
    else
        printf("\nJuiz ativo. Pressione uma tecla aqui para encerrar o Juiz.\n");
    cor(COR_NORMAL);

    while (1)
    {
        if (_kbhit())
        {
            _getch();
            break;
        }

        if (n > 0 &&
            WaitForMultipleObjects((DWORD)n, jogadores, TRUE, 0) == WAIT_OBJECT_0)
            break;

        Sleep(200);
    }

    if (job != NULL)
    {
        TerminateJobObject(job, 0);
        CloseHandle(job);
    }

    for (int i = 0; i < n; i++)
        if (jogadores[i] != NULL)
            CloseHandle(jogadores[i]);

    cor(COR_OK);
    printf("Partida encerrada.\n");
    cor(COR_NORMAL);
}

static void partidaLocal(void)
{
    int porta;
    char args[64];
    HANDLE job, jog[2];

    if (!garantirExecutaveis(1, 1))
        return;

    if ((porta = lerPorta()) == 0)
        return;

    if (!portaLivre(porta))
    {
        cor(COR_ERRO);
        printf("A porta %d ja esta em uso. Escolha outra.\n", porta);
        cor(COR_NORMAL);
        return;
    }

    job = criarJob();

    snprintf(args, sizeof(args), "%d", porta);

    if (lancar(job, "Psiquico - JUIZ", EXE_JUIZ, args) == NULL)
        return;

    Sleep(ESPERA_JUIZ_MS);

    snprintf(args, sizeof(args), "%s %d", IP_LOCAL, porta);

    jog[0] = lancar(job, "Psiquico - JOGADOR A", EXE_CLIENTE, args);
    Sleep(ESPERA_JOG_MS);
    jog[1] = lancar(job, "Psiquico - JOGADOR B", EXE_CLIENTE, args);

    if (jog[0] == NULL || jog[1] == NULL)
    {
        /* Nada de partida pela metade: encerra o que foi aberto. */
        if (job) { TerminateJobObject(job, 0); CloseHandle(job); }
        return;
    }

    esperarPartida(job, jog, 2);
}

static void hospedarPartida(void)
{
    int porta;
    char args[64];
    HANDLE job, jog;

    if (!garantirExecutaveis(1, 1))
        return;

    if ((porta = lerPorta()) == 0)
        return;

    if (!portaLivre(porta))
    {
        cor(COR_ERRO);
        printf("A porta %d ja esta em uso. Escolha outra.\n", porta);
        cor(COR_NORMAL);
        return;
    }

    cor(COR_TITULO);
    printf("\nPeca ao outro jogador para usar a opcao 3 com um destes IPs "
           "e a porta %d:\n", porta);
    cor(COR_NORMAL);
    listarIPs();
    cor(COR_DICA);
    printf("(Se a conexao falhar, libere a porta %d no Firewall do Windows.)\n",
           porta);
    cor(COR_NORMAL);

    job = criarJob();

    snprintf(args, sizeof(args), "%d", porta);

    if (lancar(job, "Psiquico - JUIZ", EXE_JUIZ, args) == NULL)
        return;

    Sleep(ESPERA_JUIZ_MS);

    snprintf(args, sizeof(args), "%s %d", IP_LOCAL, porta);

    jog = lancar(job, "Psiquico - SEU JOGADOR", EXE_CLIENTE, args);

    if (jog == NULL)
    {
        if (job) { TerminateJobObject(job, 0); CloseHandle(job); }
        return;
    }

    CloseHandle(jog);
    esperarPartida(job, NULL, 0);
}

static void entrarPartida(void)
{
    char host[128];
    char args[256];
    int porta;
    HANDLE h;

    if (!garantirExecutaveis(0, 1))
        return;

    if (!lerLinha("IP ou nome do anfitriao: ", host, sizeof(host)))
        return;

    if (!hostValido(host))
    {
        cor(COR_ERRO);
        printf("Endereco invalido.\n");
        cor(COR_NORMAL);
        return;
    }

    if ((porta = lerPorta()) == 0)
        return;

    snprintf(args, sizeof(args), "%s %d", host, porta);

    h = lancar(NULL, "Psiquico - JOGADOR", EXE_CLIENTE, args);

    if (h != NULL)
    {
        CloseHandle(h);
        cor(COR_OK);
        printf("Jogador aberto em uma nova janela.\n");
        cor(COR_NORMAL);
    }
}

static void somenteJuiz(void)
{
    int porta;
    char args[32];
    HANDLE job;

    if (!garantirExecutaveis(1, 0))
        return;

    if ((porta = lerPorta()) == 0)
        return;

    if (!portaLivre(porta))
    {
        cor(COR_ERRO);
        printf("A porta %d ja esta em uso. Escolha outra.\n", porta);
        cor(COR_NORMAL);
        return;
    }

    cor(COR_TITULO);
    printf("\nOs dois jogadores devem se conectar a um destes IPs, porta %d:\n",
           porta);
    cor(COR_NORMAL);
    listarIPs();

    job = criarJob();

    snprintf(args, sizeof(args), "%d", porta);

    if (lancar(job, "Psiquico - JUIZ", EXE_JUIZ, args) == NULL)
        return;

    esperarPartida(job, NULL, 0);
}

static void opcaoCompilar(void)
{
    printf("\nCompilando...\n\n");
    compilarTudo();
}

static void mostrarRegras(void)
{
    cor(COR_TITULO);
    printf("\n  COMO JOGAR\n");
    cor(COR_NORMAL);
    printf("  1. O Juiz (servidor) aceita exatamente 2 jogadores.\n");
    printf("  2. Cada jogador define seu objeto secreto:\n");
    printf("       /secret <objeto>\n");
    printf("  3. Quando os dois definirem, ambos adivinham ao mesmo tempo:\n");
    printf("       /guess <objeto>\n");
    printf("  4. Quem acertar primeiro o objeto do oponente vence.\n");
    printf("  5. Use qualquer texto sem barra para conversar com o oponente\n");
    printf("     (dicas, blefes...).\n");
    printf("  6. /END sai do jogo.\n");
    printf("\n  Maiusculas/minusculas e espacos nas pontas sao ignorados.\n");
}

static void mostrarMenu(void)
{
    limparTela();
    mostrarBanner();

    printf("\n");
    cor(COR_OPCAO); printf("   [1] "); cor(COR_NORMAL);
    printf("Partida local         (Juiz + 2 jogadores neste PC)\n");
    cor(COR_OPCAO); printf("   [2] "); cor(COR_NORMAL);
    printf("Hospedar partida      (Juiz + 1 jogador; o outro entra pela rede)\n");
    cor(COR_OPCAO); printf("   [3] "); cor(COR_NORMAL);
    printf("Entrar em partida     (jogador, informando IP e porta)\n");
    cor(COR_OPCAO); printf("   [4] "); cor(COR_NORMAL);
    printf("Somente o Juiz        (servidor dedicado)\n");
    cor(COR_OPCAO); printf("   [5] "); cor(COR_NORMAL);
    printf("Compilar os fontes\n");
    cor(COR_OPCAO); printf("   [6] "); cor(COR_NORMAL);
    printf("Regras e comandos\n");
    cor(COR_OPCAO); printf("   [0] "); cor(COR_NORMAL);
    printf("Sair\n\n");

    cor(COR_DICA);
    printf("   Pasta: %s\n", g_dir);
    printf("   %s: %s | %s: %s\n\n",
           EXE_JUIZ,    arquivoExiste(EXE_JUIZ)    ? "ok" : "ausente",
           EXE_CLIENTE, arquivoExiste(EXE_CLIENTE) ? "ok" : "ausente");
    cor(COR_NORMAL);
}

int main(void)
{
    WSADATA wsaData;
    char opcao[16];

    {
        char *barra;

        if (GetModuleFileNameA(NULL, g_dir, sizeof(g_dir)) == 0)
            strcpy(g_dir, ".");

        barra = strrchr(g_dir, '\\');
        if (barra != NULL)
            *barra = '\0';

        SetCurrentDirectoryA(g_dir);
    }

    SetConsoleTitleA("O Jogo do Psiquico");

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("Nao foi possivel inicializar o Winsock.\n");
        return 1;
    }

    while (1)
    {
        mostrarMenu();

        if (!lerLinha("   Escolha uma opcao: ", opcao, sizeof(opcao)))
            break;

        printf("\n");

        switch (opcao[0])
        {
            case '1': partidaLocal();    pausar(); break;
            case '2': hospedarPartida(); pausar(); break;
            case '3': entrarPartida();   pausar(); break;
            case '4': somenteJuiz();     pausar(); break;
            case '5': opcaoCompilar();   pausar(); break;
            case '6': mostrarRegras();   pausar(); break;
            case '0': WSACleanup(); return 0;
            default:
                cor(COR_ERRO);
                printf("Opcao invalida.\n");
                cor(COR_NORMAL);
                Sleep(800);
        }
    }

    WSACleanup();
    return 0;
}
