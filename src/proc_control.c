

#include "proc_control.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/resource.h>


/**
 * send_signal - Envia um sinal por meio da função kill do SO a um PID e mapeia errno para ProcResult.
 *
 * Centraliza o tratamento de erros de kill(2) para não repetir o mesmo
 * bloco switch em cada função de controle.
 *
 * @param pid  PID do processo alvo.
 * @param sig  Número do sinal a enviar.
 * @return PROC_OK, PROC_ERR_NOT_FOUND ou PROC_ERR_NO_PERM.
 */
static ProcResult send_signal(pid_t pid, int sig)
{
    if (kill(pid, sig) == 0)
        return PROC_OK;

    switch (errno) {
        case ESRCH: return PROC_ERR_NOT_FOUND;
        case EPERM: return PROC_ERR_NO_PERM;
        default:    return PROC_ERR_IO;
    }
}


ProcResult apply_process_action(pid_t pid, ProcessAction action)
{
    switch (action) {
        case PROC_ACTION_STOP:
            return send_signal(pid, SIGSTOP);

        case PROC_ACTION_CONT:
            return send_signal(pid, SIGCONT);

        case PROC_ACTION_TERM:
            return send_signal(pid, SIGTERM);

        case PROC_ACTION_KILL:
            return send_signal(pid, SIGKILL);

        case PROC_ACTION_RESTART:
            send_signal(pid, SIGCONT);
            return send_signal(pid, SIGTERM);

        default:
            return PROC_ERR_INVALID_ARG;
    }
}

ProcResult set_process_nice(pid_t pid, int nice_val)
{
    if (nice_val < NICE_MIN || nice_val > NICE_MAX)
        return PROC_ERR_INVALID_ARG;

    if (setpriority(PRIO_PROCESS, (id_t)pid, nice_val) == -1) {
        switch (errno) {
            case ESRCH: return PROC_ERR_NOT_FOUND;
            case EACCES:
            case EPERM:  return PROC_ERR_NO_PERM;
            default:     return PROC_ERR_IO;
        }
    }

    return PROC_OK;
}

ProcResult run_with_nice(int nice_val, char *argv[])
{
    if (!argv || !argv[0])
        return PROC_ERR_INVALID_ARG;

    if (nice_val < NICE_MIN || nice_val > NICE_MAX)
        return PROC_ERR_INVALID_ARG;

    pid_t child = fork();

    if (child == -1)
        return PROC_ERR_IO;

    if (child == 0) {
        /* Processo filho: define a prioridade antes de executar o comando */
        if (setpriority(PRIO_PROCESS, 0, nice_val) == -1) {
            perror("setpriority");
            _exit(EXIT_FAILURE); /* _exit evita flush de buffers do pai */
        }
        execvp(argv[0], argv);
        /* Se chegou aqui, execvp falhou */
        perror("execvp");
        _exit(EXIT_FAILURE);
    }

    /* Processo pai: informa o PID do filho e retorna sem esperar */
    printf("\033[32mProcesso '%s' iniciado com PID %d (nice: %d).\033[0m\n",
           argv[0], child, nice_val);

    return PROC_OK;
}