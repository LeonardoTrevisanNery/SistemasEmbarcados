#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_WORDS 1000 //quantidade maxima que vai ser puxada da wordlist
#define WORD_SIZE 5

/* =========================
   CORES ANSI
   ========================= */

#define RESET   "\033[0m"
#define GREEN   "\033[42m\033[30m"
#define YELLOW  "\033[43m\033[30m"
#define RED     "\033[41m\033[37m"
#define GRAY    "\033[100m\033[37m"

/* =========================
   VARIÁVEIS
   ========================= */

char Vetor[WORD_SIZE];
int player = 1;
char screen[WORD_SIZE] = {
    ' ', ' ', ' ', ' ', ' '
};

/*
   0 = vazio
   1 = vermelho
   2 = amarelo
   3 = verde
*/
int colors[WORD_SIZE] = {
    0, 0, 0, 0, 0
};

char avaliableWords[MAX_WORDS][WORD_SIZE + 1];

char Choice[WORD_SIZE + 1];


/* =========================
   ATIVAR ANSI NO WINDOWS
   ========================= */

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


/* =========================
   LIMPAR TELA
   ========================= */

void clear_screen()
{
    system("cls");
}


/* =========================
   MOSTRAR QUADRADO
   ========================= */

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


/* =========================
   PAINEL
   ========================= */

/* =========================
   PAINEL
   ========================= */

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

/* =========================
   CARREGAR WORDLIST
   ========================= */

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
        /* Remove \n e \r */

        line[strcspn(line, "\r\n")] = '\0';

        /* Só aceita palavras de exatamente 5 letras */

        if (strlen(line) == WORD_SIZE)
        {
            strcpy(avaliableWords[quantidade], line);

            quantidade++;
        }
    }

    fclose(file);

    return quantidade;
}


/* =========================
   SORTEAR PALAVRA
   ========================= */

void sortWord(int quantidade)
{
    int sorteada = rand() % quantidade;

    for (int i = 0; i < WORD_SIZE; i++)
    {
        Vetor[i] = avaliableWords[sorteada][i];
    }
}


/* =========================
   MOSTRAR PALAVRA ESCOLHIDA
   ========================= */

void showWord()
{
    printf("\nPalavra sorteada: ");

    for (int i = 0; i < WORD_SIZE; i++)
    {
        printf("%c", Vetor[i]);
    }

    printf("\n");
}


/* =========================
   AVALIAR PALAVRA
   ========================= */

void evaluateWord()
{
    /*
       Primeiro verifica letras
       na posição correta.
    */

    for (int i = 0; i < WORD_SIZE; i++)
    {
        colors[i] = 1;

        if (Choice[i] == Vetor[i])
        {
            colors[i] = 3;
        }
    }


    /*
       Depois verifica letras que
       existem em outras posições.
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
       Copia a palavra digitada
       para o painel.
    */

    for (int i = 0; i < WORD_SIZE; i++)
    {
        screen[i] = Choice[i];
    }
}


/* =========================
   VERIFICAR VITÓRIA
   ========================= */

int checkWin()
{
    for (int i = 0; i < WORD_SIZE; i++)
    {
        if (Choice[i] != Vetor[i])
            return 0;
    }

    return 1;
}


/* =========================
   MAIN
   ========================= */

int main()
{
    /* Inicializa o sorteio */

    srand(time(NULL));

    /* Ativa ANSI */

    enableANSI();


    /* Carrega wordlist.txt */

    int quantidade = loadWordlist();

    if (quantidade == 0)
    {
        printf("Nenhuma palavra valida encontrada.\n");
        return 1;
    }


    /* Sorteia palavra */

    sortWord(quantidade);


    /* Tela inicial */

    clear_screen();

    pannel();


    /*
       Loop do jogo
    */

       /*
       Loop do jogo
    */

    while (1)
    {
        printf("Jogador %d, digite uma palavra de 5 letras: ",
               player);

        scanf("%5s", Choice);


        /*
           Converte para minúsculas.
        */

        for (int i = 0; i < WORD_SIZE; i++)
        {
            if (Choice[i] >= 'A' &&
                Choice[i] <= 'Z')
            {
                Choice[i] += 32;
            }
        }


        /*
           Verifica se realmente possui
           5 letras.
        */

        if (strlen(Choice) != WORD_SIZE)
        {
            printf("\nDigite exatamente 5 letras!\n");
            continue;
        }


        /* Avalia */

        evaluateWord();


        /* Atualiza a tela */

        
        
        /* Verifica vitória */
        
        if (checkWin())
        {
            printf("\n");
            printf(GREEN "        PARABENS! Voce acertou!\n" RESET);
            
            printf("        Palavra: %s\n", Vetor);
            
            system("pause");
            
            break;
        }
        
        
        /*
        Troca o jogador
        depois da tentativa.
        */
       
       if (player == 1)
       {
           player = 2;
        }
        else
        {
            player = 1;
        }
        clear_screen();
        pannel();
    }
    return 0;
}
