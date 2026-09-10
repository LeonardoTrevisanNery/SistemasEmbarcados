#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_WORDS 10
#define WORD_SIZE 5

char Vetor[WORD_SIZE];
char screen[WORD_SIZE] = {' ', ' ', ' ', ' ', ' '};

char avaliableWords[MAX_WORDS][WORD_SIZE + 1];

char Choice;


/* =========================
   LIMPAR TELA
   ========================= */

void clear_screen()
{
    system("cls");
}


/* =========================
   PAINEL
   ========================= */

void pannel()
{
    printf("****************************\n");
    printf("|          GTW             |\n");
    printf("|                          |\n");
    printf("|                          |\n");
    printf("|                          |\n");
    printf("|                          |\n");

    printf("|  [%c][%c][%c][%c][%c]         |\n",
           screen[0],
           screen[1],
           screen[2],
           screen[3],
           screen[4]);

    printf("|                          |\n");
    printf("|                          |\n");
    printf("****************************\n");
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

    /*
       Copia a palavra sorteada
       letra por letra para Vetor
    */

    for (int i = 0; i < WORD_SIZE; i++)
    {
        Vetor[i] = avaliableWords[sorteada][i];
    }
}


/* =========================
   MOSTRAR PALAVRA ESCOLHIDA
   (APENAS PARA TESTE)
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
   MAIN
   ========================= */

int main()
{
    /*
       Inicializa o sorteio.
       O time(NULL) evita sortear
       sempre a mesma palavra.
    */

    srand(time(NULL));


    /* Carrega wordlist.txt */

    int quantidade = loadWordlist();

    if (quantidade == 0)
    {
        printf("Nenhuma palavra valida encontrada.\n");
        return 1;
    }


    /* Sorteia a palavra */

    sortWord(quantidade);


    /* Tela inicial */

    clear_screen();

    pannel();


    /*
       Loop do jogo
    */

    while (1)
    {
        printf("\nEscolha uma letra: ");
        scanf(" %c", &Choice);


        /*
           Procura a letra na palavra
        */

        for (int i = 0; i < WORD_SIZE; i++)
        {
            if (Choice == Vetor[i])
            {
                screen[i] = Choice;
            }
        }


        /*
           Atualiza a tela
        */

        clear_screen();

        pannel();
    }


    return 0;
}
