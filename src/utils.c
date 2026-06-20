/**
 * utils.c - Implementação dos utilitários de terminal e entrada.
 */

#include "utils.h"
#include "process.h"

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

void read_password(char *out_password, size_t buf_size)
{
    if (!out_password || buf_size == 0)
        return;

    struct termios original, silent;
    tcgetattr(STDIN_FILENO, &original);

    silent = original;
    silent.c_lflag &= ~(ECHO); /* Desliga o echo visual do terminal */
    tcsetattr(STDIN_FILENO, TCSANOW, &silent);

    if (fgets(out_password, (int)buf_size, stdin)) {
        /* Remove '\n' inserido pelo fgets */
        out_password[strcspn(out_password, "\n")] = '\0';
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &original);
    printf("\n");
}

void clear_input_buffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ; /* Descarta caracteres pendentes */
}