#ifndef UTILS_H
#define UTILS_H

#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#include "process.h"


/**
 * clear_input_buffer - Descarta caracteres pendentes no stdin.
 *
 * Útil após leituras com scanf para evitar que '\n' residual
 * seja consumido pela próxima leitura.
 */
void clear_input_buffer(void);

#endif /* UTILS_H */