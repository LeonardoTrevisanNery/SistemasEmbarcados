#include <stdio.h>
#include <stdlib.h>

/* ---------- utilidades ---------- */
void clear(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
int vida=4; //cada jogador gasta 25 de vida ou seja 4 tentativas ate zerar vida e sanidade
int sanidade=100;
void pausar(void) {
    printf("\n  Pressione ENTER para continuar...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF); // limpa buffer
    getchar();                                   // espera ENTER
}

/* ---------- telas ---------- */
void menu_principal(void) {
    printf("\n");
    printf("  ___________________________________________\n");
    printf(" |                                           |\n");
    printf(" |        * * *   P S I Q U E   * * *        |\n");
    printf(" |              _ 2 J O G A D O R E S _      |\n");
    printf(" |                                           |\n");
    printf(" |     _ _ _ _ _ _ _ _ _ _ _ _ _ _ _         |\n");
    printf(" |                                           |\n");
    printf(" |        [1]  -  Novo Jogo (J1 vs J2)       |\n");
    printf(" |        [2]  -  Instrucoes                 |\n");
    printf(" |        [3]  -  Creditos                   |\n");
    printf(" |        [0]  -  Sair                       |\n");
    printf(" |                                           |\n");
    printf(" |     _ _ _ _ _ _ _ _ _ _ _ _ _ _ _         |\n");
    printf(" |                                           |\n");
    printf(" |           *  |  *   VS   *  |  *          |\n");
    printf(" |___________________________________________|\n");
    printf("\n         Escolha uma opcao > ");
}

void tela_creditos(void) {
    clear();
    printf("\n");
    printf("  ___________________________________________\n");
    printf(" |                                           |\n");
    printf(" |        * * *   P S I Q U E   * * *        |\n");
    printf(" |        _ P R O G R A M A D O R E S _      |\n");
    printf(" |                                           |\n");
    printf(" |     _ _ _ _ _ _ _ _ _ _ _ _ _ _ _         |\n");
    printf(" |                                           |\n");
    printf(" |         *  Leonardo Trevisan Nery         |\n");
    printf(" |         *  Arthur Santos Correia          |\n");
    printf(" |         *  Luis Furlannis                 |\n");
    printf(" |                                           |\n");
    printf(" |     _ _ _ _ _ _ _ _ _ _ _ _ _ _ _         |\n");
    printf(" |                                           |\n");
    printf(" |           *  |  *   VS   *  |  *          |\n");
    printf(" |___________________________________________|\n");
    pausar();
}

void tela_instrucoes(void) {
    clear();
    printf("\n");
    printf("  ___________________________________________\n");
    printf(" |                                           |\n");
    printf(" |         [ ]  I N S T R U C O E S          |\n");
    printf(" |                                           |\n");
    printf(" |     _ _ _ _ _ _ _ _ _ _ _ _ _ _ _         |\n");
    printf(" |                                           |\n");
    printf(" |   * Dois jogadores alternam turnos        |\n");
    printf(" |   * Cada um escolhe uma acao por turno    |\n");
    printf(" |   * Vida e sanidade chegam a 0 = derrota  |\n");
    printf(" |   * Passe o teclado entre os turnos       |\n");
    printf(" |                                           |\n");
    printf(" |     _ _ _ _ _ _ _ _ _ _ _ _ _ _ _         |\n");
    printf(" |___________________________________________|\n");
    pausar();
}

/* ---------- main ---------- */
int main(void) {
    int opcao;

    do {
        clear();
        menu_principal();

        if (scanf("%d", &opcao) != 1) {
            opcao = -1; // entrada inválida
        }

        switch (opcao) {
            case 1:
                clear();
                printf("\n  >>> Novo Jogo ainda em construcao... <<<\n");
                pausar();
                break;

            case 2:
                tela_instrucoes();
                break;

            case 3:
                tela_creditos();
                break;

            case 0:
                clear();
                break;

            default:
                printf("\n  [!] Opcao invalida. Tente novamente.\n");
                pausar();
                break;
        }
    } while (opcao != 0);

    return 0;
}
