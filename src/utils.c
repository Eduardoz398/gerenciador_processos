#include "utils.h"
#include "process.h"

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

void clear_input_buffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ; /* Descarta caracteres pendentes */
}