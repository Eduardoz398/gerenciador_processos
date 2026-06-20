

#include "menu.h"
#include "fetch_proc.h"
#include "proc_control.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static const char *action_names[] = {
        "",      
        "suspenso",
        "retomado",
        "sinal de término enviado",
        "finalizado",
        "reiniciado"
    };

/**
 * print_action_menu - Exibe as opções de controle de estado de processo.
 */
static void print_action_menu(void)
{
    printf("\n  Ação desejada:\n");
    printf("  %d. Bloquear (suspender)\n",       PROC_ACTION_STOP);
    printf("  %d. Continuar (retomar suspenso)\n", PROC_ACTION_CONT);
    printf("  %d. Finalizar (gracioso - SIGTERM)\n", PROC_ACTION_TERM);
    printf("  %d. Finalizar (imediato - SIGKILL)\n", PROC_ACTION_KILL);
    printf("  %d. Reiniciar\n",                   PROC_ACTION_RESTART);
    printf("  Escolha: ");
}

/**
 * print_action_result - Exibe mensagem de resultado após uma ação de controle.
 *
 * @param result  Código de retorno da operação.
 * @param pid     PID sobre o qual a ação foi aplicada.
 * @param action  Ação que foi tentada.
 */
static void print_action_result(ProcResult result, pid_t pid, ProcessAction action)
{
    

    if (result == PROC_OK) {
        printf("\033[32mProcesso %d %s com sucesso.\033[0m\n",
               pid, action_names[action]);
        return;
    }

    switch (result) {
        case PROC_ERR_NOT_FOUND:
            printf("\033[31mProcesso %d não encontrado.\033[0m\n", pid);
            break;
        case PROC_ERR_NO_PERM:
            printf("\033[31mPermissão negada. Execute como root para "
                   "controlar este processo.\033[0m\n");
            break;
        case PROC_ERR_INVALID_ARG:
            printf("\033[31mAção inválida.\033[0m\n");
            break;
        default:
            printf("\033[31mErro inesperado ao controlar o processo.\033[0m\n");
            break;
    }
}


/**
 * handle_list_users - Lista todos os usuários do sistema.
 */
static void handle_list_users(void)
{
    list_system_users();
}

/**
 * handle_list_user_procs - Solicita usuário, valida acesso e lista processos.
 *
 * Aplica proteção por senha quando o usuário alvo é root.
 */
static void handle_list_user_procs(void)
{
    char username[USERNAME_MAX];

    printf("Digite o nome do usuário: ");
    if (scanf("%255s", username) != 1) {
        clear_input_buffer();
        printf("\033[31mEntrada inválida.\033[0m\n");
        return;
    }
    clear_input_buffer();


    uid_t target_uid;
    ProcResult res = resolve_uid(username, &target_uid);

    if (res != PROC_OK) {
        printf("\033[33mUsuário '%s' não encontrado.\033[0m\n", username);
        return;
    }

    list_processes_by_uid(target_uid, username);
}

/**
 * handle_query_pid - Solicita um PID e exibe suas informações detalhadas.
 */
static void handle_query_pid(void)
{
    pid_t pid;

    printf("Digite o PID do processo: ");
    if (scanf("%d", &pid) != 1) {
        clear_input_buffer();
        printf("\033[31mPID inválido.\033[0m\n");
        return;
    }
    clear_input_buffer();

    print_process_info(pid);
}

/**
 * handle_control_process - Exibe submenu de ações e aplica a escolhida.
 *
 * Solicita PID, exibe o submenu de ações (STOP/CONT/TERM/KILL/RESTART),
 * chama apply_process_action() e exibe o resultado.
 */
static void handle_control_process(void)
{
    pid_t pid;

    printf("Digite o PID do processo: ");
    if (scanf("%d", &pid) != 1) {
        clear_input_buffer();
        printf("\033[31mPID inválido.\033[0m\n");
        return;
    }
    clear_input_buffer();

    /* Mostra o estado atual antes de agir */
    print_process_info(pid);

    print_action_menu();

    int raw_action;
    if (scanf("%d", &raw_action) != 1) {
        clear_input_buffer();
        printf("\033[31mEntrada inválida.\033[0m\n");
        return;
    }
    clear_input_buffer();

    ProcessAction action = (ProcessAction)raw_action;
    ProcResult result = apply_process_action(pid, action);
    print_action_result(result, pid, action);
}

/**
 * handle_change_priority - Altera o nice de um processo já em execução.
 *
 * Solicita PID e novo valor nice, chama set_process_nice() e confirma.
 */
static void handle_change_priority(void)
{
    pid_t pid;
    int nice_val;

    printf("Digite o PID do processo: ");
    if (scanf("%d", &pid) != 1) {
        clear_input_buffer();
        printf("\033[31mPID inválido.\033[0m\n");
        return;
    }
    clear_input_buffer();

    /* Mostra o nice atual antes de alterar */
    print_process_info(pid);

    printf("Novo valor de prioridade nice (%d a %d, menor = maior prioridade): ",
           NICE_MIN, NICE_MAX);

    if (scanf("%d", &nice_val) != 1) {
        clear_input_buffer();
        printf("\033[31mValor inválido.\033[0m\n");
        return;
    }
    clear_input_buffer();

    ProcResult result = set_process_nice(pid, nice_val);

    switch (result) {
        case PROC_OK:
            printf("\033[32mPrioridade do processo %d alterada para nice=%d.\033[0m\n",
                   pid, nice_val);
            break;
        case PROC_ERR_NOT_FOUND:
            printf("\033[31mProcesso %d não encontrado.\033[0m\n", pid);
            break;
        case PROC_ERR_NO_PERM:
            printf("\033[31mPermissão negada. Valores de nice negativos "
                   "exigem privilégio de root.\033[0m\n");
            break;
        case PROC_ERR_INVALID_ARG:
            printf("\033[31mValor nice fora da faixa (%d a %d).\033[0m\n",
                   NICE_MIN, NICE_MAX);
            break;
        default:
            printf("\033[31mErro inesperado ao alterar prioridade.\033[0m\n");
            break;
    }
}

/**
 * handle_run_with_priority - Executa um novo comando com nice definido.
 *
 * Solicita o valor nice e a linha de comando, monta o argv e chama
 * run_with_nice(). O processo filho roda em background; o menu continua.
 */
static void handle_run_with_priority(void)
{
    int nice_val;

    printf("Valor de prioridade nice para o novo processo (%d a %d): ",
           NICE_MIN, NICE_MAX);

    if (scanf("%d", &nice_val) != 1) {
        clear_input_buffer();
        printf("\033[31mValor inválido.\033[0m\n");
        return;
    }
    clear_input_buffer();

    if (nice_val < NICE_MIN || nice_val > NICE_MAX) {
        printf("\033[31mValor nice fora da faixa (%d a %d).\033[0m\n",
               NICE_MIN, NICE_MAX);
        return;
    }

    char command_line[LINE_BUF_MAX];

    printf("Comando a executar (ex: sleep 30): ");
    if (!fgets(command_line, sizeof(command_line), stdin)) {
        printf("\033[31mEntrada inválida.\033[0m\n");
        return;
    }
    /* Remove '\n' do fgets */
    command_line[strcspn(command_line, "\n")] = '\0';

    if (command_line[0] == '\0') {
        printf("\033[31mComando vazio.\033[0m\n");
        return;
    }

    /*
     * Constrói argv a partir da linha de comando tokenizando por espaço.
     * Limite de MAX_ARGV_TOKENS argumentos (suficiente para uso interativo).
     */

    char *argv[MAX_ARGV_TOKENS + 1]; /* +1 para o NULL terminal */
    int   argc = 0;

    char *token = strtok(command_line, " \t");
    while (token && argc < MAX_ARGV_TOKENS) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    argv[argc] = NULL; /* execvp exige vetor terminado em NULL */

    ProcResult result = run_with_nice(nice_val, argv);

    if (result != PROC_OK) {
        switch (result) {
            case PROC_ERR_NO_PERM:
                printf("\033[31mPermissão negada. Nice negativo exige root.\033[0m\n");
                break;
            case PROC_ERR_IO:
                printf("\033[31mFalha ao criar processo (fork).\033[0m\n");
                break;
            default:
                printf("\033[31mErro ao executar o comando.\033[0m\n");
                break;
        }
    }
}


/**
 * print_menu - Exibe as opções do menu principal.
 */
static void print_menu(void)
{
    printf("\n=== Gerenciador de Processos ===\n");
    printf("%d. Listar usuários do sistema\n",                    MENU_LIST_USERS);
    printf("%d. Listar processos de um usuário\n",                MENU_LIST_USER_PROCS);
    printf("%d. Consultar informações de um processo (PID)\n",    MENU_QUERY_PID);
    printf("%d. Alterar estado de um processo\n",                 MENU_CONTROL_PROCESS);
    printf("%d. Trocar prioridade de um processo em execução\n",  MENU_CHANGE_PRIORITY);
    printf("%d. Executar processo com prioridade definida\n",     MENU_RUN_WITH_PRIORITY);
    printf("%d. Sair\n",                                          MENU_EXIT);
    printf("Escolha uma opção: ");
}

void run_menu_loop(void)
{
    int raw_choice = 0;

    while (1) {
        print_menu();

        if (scanf("%d", &raw_choice) != 1) {
            clear_input_buffer();
            printf("\033[31mEntrada inválida. Digite um número.\033[0m\n");
            continue;
        }
        clear_input_buffer();

        MenuOption choice = (MenuOption)raw_choice;

        switch (choice) {
            case MENU_LIST_USERS:
                handle_list_users();
                break;

            case MENU_LIST_USER_PROCS:
                handle_list_user_procs();
                break;

            case MENU_QUERY_PID:
                handle_query_pid();
                break;

            case MENU_CONTROL_PROCESS:
                handle_control_process();
                break;

            case MENU_CHANGE_PRIORITY:
                handle_change_priority();
                break;

            case MENU_RUN_WITH_PRIORITY:
                handle_run_with_priority();
                break;

            case MENU_EXIT:
                printf("Saindo...\n");
                return;

            default:
                printf("\033[31mOpção inválida. Escolha entre %d e %d.\033[0m\n",
                       MENU_LIST_USERS, MENU_EXIT);
                break;
        }
    }
}