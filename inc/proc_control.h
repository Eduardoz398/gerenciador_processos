#ifndef PROC_CONTROL_H
#define PROC_CONTROL_H

/**
 * proc_control.h - API de controle de estado e prioridade de processos.
 *
 * Separa responsabilidades: fetch_proc.h lê dados, proc_control.h age sobre
 * processos. Toda operação recebe um pid_t e retorna ProcResult, seguindo
 * o mesmo contrato do restante do projeto.
 *
 * Sinais usados internamente (definidos em <signal.h>):
 *   SIGSTOP  → suspende o processo (não pode ser ignorado pelo processo)
 *   SIGCONT  → retoma um processo suspenso
 *   SIGTERM  → solicita encerramento gracioso
 *   SIGKILL  → encerramento imediato e incondicional
 *
 * Troca de prioridade usa setpriority(2) / getpriority(2) de <sys/resource.h>.
 * Faixa válida de nice: NICE_MIN (-20) a NICE_MAX (19).
 * Valores menores = maior prioridade de CPU.
 */

#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>

#include "process.h"

/* ── Faixa de valores nice ───────────────────────────────────────────────── */

#define NICE_MIN  (-20)  /* Maior prioridade (requer privilégio de root) */
#define NICE_MAX   (19)  /* Menor prioridade                             */

/* ── Enum de ações sobre o estado do processo ────────────────────────────── */

/**
 * ProcessAction - Ações possíveis sobre o estado de um processo.
 *
 * Usado internamente pelo submenu para mapear escolha do usuário → sinal.
 * Para adicionar uma nova ação (ex.: PROC_ACTION_HUP para recarregar config):
 *   1. Adicione o valor ao enum.
 *   2. Adicione o case em apply_process_action().
 */
typedef enum {
    PROC_ACTION_STOP     = 1,  /* Bloquear (suspender) o processo   — SIGSTOP */
    PROC_ACTION_CONT     = 2,  /* Continuar processo suspenso        — SIGCONT */
    PROC_ACTION_TERM     = 3,  /* Finalizar (gracioso)               — SIGTERM */
    PROC_ACTION_KILL     = 4,  /* Finalizar (imediato)               — SIGKILL */
    PROC_ACTION_RESTART  = 5   /* Reiniciar (SIGTERM + SIGCONT fake via exec)  */
} ProcessAction;

/* ── Controle de estado ──────────────────────────────────────────────────── */

/**
 * apply_process_action - Aplica uma ação de controle a um processo.
 *
 * Envia o sinal correspondente à ação via kill(2). A ação PROC_ACTION_RESTART
 * envia SIGTERM seguido de SIGCONT para reativar o processo se ele estiver
 * suspenso antes de finalizar; quem reinicia de fato é o sistema/supervisor.
 *
 * @param pid     PID do processo alvo.
 * @param action  Ação a aplicar (valor de ProcessAction).
 * @return PROC_OK em sucesso,
 *         PROC_ERR_NOT_FOUND se o PID não existir,
 *         PROC_ERR_NO_PERM se não houver permissão,
 *         PROC_ERR_INVALID_ARG se a ação for inválida.
 */
ProcResult apply_process_action(pid_t pid, ProcessAction action);

/* ── Controle de prioridade ──────────────────────────────────────────────── */

/**
 * set_process_nice - Altera o valor nice de um processo em execução.
 *
 * Chama setpriority(PRIO_PROCESS, pid, nice_val). Diminuir o nice (aumentar
 * prioridade) abaixo de 0 requer privilégio de root (CAP_SYS_NICE).
 *
 * @param pid       PID do processo alvo.
 * @param nice_val  Novo valor nice (NICE_MIN a NICE_MAX).
 * @return PROC_OK em sucesso,
 *         PROC_ERR_NOT_FOUND se o PID não existir,
 *         PROC_ERR_NO_PERM se não houver privilégio suficiente,
 *         PROC_ERR_INVALID_ARG se nice_val estiver fora da faixa.
 */
ProcResult set_process_nice(pid_t pid, int nice_val);

/**
 * run_with_nice - Executa um comando com prioridade definida.
 *
 * Equivalente a: nice -n <nice_val> <command> [args...]
 * Usa fork()+execvp() internamente. O processo filho herda o nice_val
 * antes do exec, sem afetar o processo pai (o gerenciador).
 *
 * @param nice_val  Valor nice desejado para o novo processo.
 * @param argv      Vetor de argumentos terminado em NULL (argv[0] = comando).
 * @return PROC_OK se o processo filho foi criado com sucesso (não espera pelo
 *         término do filho),
 *         PROC_ERR_INVALID_ARG se argv for NULL ou argv[0] for NULL,
 *         PROC_ERR_NO_PERM se o nice_val exigir privilégio não disponível,
 *         PROC_ERR_IO se fork() falhar.
 */
ProcResult run_with_nice(int nice_val, char *argv[]);

#endif /* PROC_CONTROL_H */