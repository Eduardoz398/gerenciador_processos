#ifndef FETCH_PROC_H
#define FETCH_PROC_H

/**
 * fetch_proc.h - API de consulta de processos via /proc.
 *
 * Todas as funções que lêem do sistema de arquivos /proc residem aqui.
 * O design favorece a reutilização: fill_process_info() é o único ponto
 * de leitura de dados de um PID; as demais funções constroem sobre ele.
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>

#include "process.h"



/* ── Resolução de usuário ────────────────────────────────────────────────── */

/**
 * resolve_uid - Converte nome de usuário em UID.
 *
 * @param username  Nome de usuário a resolver.
 * @param out_uid   Ponteiro onde o UID será escrito em caso de sucesso.
 * @return PROC_OK em sucesso, PROC_ERR_NOT_FOUND se o usuário não existir,
 *         PROC_ERR_INVALID_ARG se algum ponteiro for NULL.
 */
ProcResult resolve_uid(const char *username, uid_t *out_uid);

/* ── Listagem de usuários do sistema ─────────────────────────────────────── */

/**
 * list_system_users - Lista todos os usuários cadastrados no sistema.
 *
 * Percorre /etc/passwd via getpwent() e imprime nome, UID e shell de cada
 * entrada. Inclui tanto usuários humanos quanto contas de sistema.
 *
 * @return PROC_OK sempre (erros de getpwent são silenciosos por design do SO).
 */
ProcResult list_system_users(void);

/* ── Leitura de dados de processo ────────────────────────────────────────── */

/**
 * fill_process_info - Preenche uma estrutura ProcessInfo a partir do /proc.
 *
 * Ponto central de leitura: lê /proc/[pid]/status e /proc/[pid]/stat,
 * resolve o UID para nome de usuário e calcula o tempo de CPU.
 * Qualquer função que precise de dados de um processo deve chamar esta
 * primeiro, sem reler o /proc por conta própria.
 *
 * @param pid   PID do processo a inspecionar.
 * @param info  Estrutura a ser preenchida (não pode ser NULL).
 * @return PROC_OK em sucesso, PROC_ERR_NOT_FOUND se o PID não existir,
 *         PROC_ERR_NO_PERM se não houver permissão de leitura,
 *         PROC_ERR_IO em falha de leitura inesperada.
 */
ProcResult fill_process_info(pid_t pid, ProcessInfo *info);

/* ── Listagem de processos por usuário ───────────────────────────────────── */

/**
 * list_processes_by_uid - Lista PIDs e nomes de processos de um UID.
 *
 * Percorre /proc, filtra entradas numéricas (PIDs), chama fill_process_info()
 * para cada um e imprime apenas os que pertencem a target_uid.
 *
 * @param target_uid  UID cujos processos serão listados.
 * @param username    Nome do usuário (usado apenas para exibição).
 * @return PROC_OK, ou PROC_ERR_IO se /proc não puder ser aberto.
 */
ProcResult list_processes_by_uid(uid_t target_uid, const char *username);

/* ── Exibição de detalhes de processo ────────────────────────────────────── */

/**
 * print_process_info - Exibe informações detalhadas de um processo.
 *
 * Chama fill_process_info() e imprime o resultado formatado.
 * Não relê o /proc por conta própria.
 *
 * @param pid  PID do processo a exibir.
 * @return PROC_OK, PROC_ERR_NOT_FOUND ou PROC_ERR_NO_PERM.
 */
ProcResult print_process_info(pid_t pid);

#endif /* FETCH_PROC_H */