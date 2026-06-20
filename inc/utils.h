#ifndef UTILS_H
#define UTILS_H

/**
 * utils.h - Utilitários de terminal e entrada de dados.
 *
 * Funções auxiliares independentes da lógica de processos:
 * leitura segura de strings, leitura de senhas sem echo, etc.
 */

#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#include "process.h"

/**
 * read_password - Lê uma senha do terminal sem exibir os caracteres digitados.
 *
 * Desabilita temporariamente o echo do terminal (ECHO), lê até PASSWORD_MAX-1
 * caracteres e restaura as configurações originais.
 *
 * @param out_password  Buffer de destino (deve ter ao menos PASSWORD_MAX bytes).
 * @param buf_size      Tamanho do buffer (use PASSWORD_MAX).
 */
void read_password(char *out_password, size_t buf_size);

/**
 * clear_input_buffer - Descarta caracteres pendentes no stdin.
 *
 * Útil após leituras com scanf para evitar que '\n' residual
 * seja consumido pela próxima leitura.
 */
void clear_input_buffer(void);

#endif /* UTILS_H */