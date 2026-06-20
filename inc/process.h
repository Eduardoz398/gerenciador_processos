#ifndef PROCESS_H
#define PROCESS_H

/**
 * process.h - Tipos e estruturas centrais do gerenciador de processos.
 *
 * Este header define as abstrações de dados compartilhadas por todo o sistema.
 * Qualquer nova funcionalidade (ex.: alterar estado, enviar sinais) deve operar
 * sobre estas estruturas, evitando duplicação de leitura do /proc.
 */

#include <sys/types.h>

/* ── Constantes ─────────────────────────────────────────────────────────── */

#define PROC_NAME_MAX   256   /* Tamanho máximo do nome de um processo         */
#define USERNAME_MAX    256   /* Tamanho máximo de um nome de usuário           */
#define LINE_BUF_MAX    512 
#define STAT_BUF_MAX   1024  

typedef enum {
    PROC_OK              =  0,
    PROC_ERR_NOT_FOUND   = -1,
    PROC_ERR_NO_PERM     = -2,
    PROC_ERR_INVALID_ARG = -3,
    PROC_ERR_IO          = -4
} ProcResult;

/* ── Estrutura principal de informações de processo ─────────────────────── */

/**
 * ProcessInfo - Snapshot de dados de um processo lido do /proc.
 *
 * Centraliza todos os campos necessários para as operações atuais e futuras.
 * Novas funcionalidades (ex.: envio de sinal, alteração de nice) devem
 * receber ou preencher esta estrutura, sem reler o /proc por conta própria.
 */
typedef struct {
    pid_t  pid;                    /* Identificador do processo                */
    uid_t  uid;                    /* UID do proprietário                      */
    long   nice;                   /* Prioridade de execução (nice value)      */
    double cpu_time_seconds;       /* Tempo total de CPU (user + kernel)       */
    char   name[PROC_NAME_MAX];    /* Nome do executável                       */
    char   state[PROC_NAME_MAX];   /* Estado legível (ex.: "S (sleeping)")     */
    char   username[USERNAME_MAX]; /* Nome do proprietário (resolvido do UID)  */
} ProcessInfo;

#endif /* PROCESS_H */