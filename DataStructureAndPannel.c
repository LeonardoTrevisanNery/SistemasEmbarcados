#include <stdio.h>
#include <string.h>

char Vetor[5] = {'c', 'e', 'r', 't', 'o'};
char screen[5] = {' ', ' ', ' ', ' ', ' '};
char Choice;

void clear_screen()
{
    printf("\033[2J\033[H");
}

void pannel()
{
    printf("****************************\n");
    printf("|          GTW             |\n");
    printf("|                          |\n");
    printf("|                          |\n");
    printf("|                          |\n");
    printf("|                          |\n");
    printf("|  [%c][%c][%c][%c][%c]         |\n",
           screen[0], screen[1], screen[2], screen[3], screen[4]);
    printf("|                          |\n");
    printf("|                          |\n");
    printf("****************************\n");
}

int main()
{
    clear_screen();

    pannel();

    printf("Escolha uma letra: ");
    scanf(" %c", &Choice);

    for (int i = 0; i < 5; i++)
    {
        if (Choice == Vetor[i])
        {
            screen[i] = Choice;
            main(); // Call main() again to refresh the screen and prompt for another letter
        }
    }

    clear_screen();
    pannel();

    return 0;
}