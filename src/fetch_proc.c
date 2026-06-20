#include "fetch_proc.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>



#define STAT_FIELD_START  3   
#define STAT_FIELD_UTIME 14   
#define STAT_FIELD_STIME 15  
#define STAT_FIELD_NICE  19   


/**
 * parse_status_file - Extrai Name, State e Uid de /proc/[pid]/status.
 *
 * @param path  Caminho completo do arquivo status.
 * @param info  Estrutura ProcessInfo a preencher (name, state, uid).
 * @return 1 em sucesso, 0 se o arquivo não puder ser aberto.
 */
static int parse_status_file(const char *path, ProcessInfo *info)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;

    char line[LINE_BUF_MAX];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Name:", 5) == 0)
            sscanf(line, "Name:\t%255s", info->name);

        if (strncmp(line, "State:", 6) == 0) {
            strncpy(info->state, line + 7, sizeof(info->state) - 1);
            info->state[strcspn(info->state, "\n")] = '\0';
        }

        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid:\t%u", &info->uid);
            break; /* UID é o último campo que nos interessa */
        }
    }

    fclose(f);
    return 1;
}

/**
 * parse_stat_file - Extrai nice, utime e stime de /proc/[pid]/stat.
 *
 * O nome do processo fica entre parênteses (pode conter espaços), por isso
 * localiza o ')' mais à direita antes de tokenizar os demais campos.
 *
 * @param path  Caminho completo do arquivo stat.
 * @param info  Estrutura ProcessInfo a preencher (cpu_time_seconds, nice).
 * @return 1 em sucesso, 0 se o arquivo não puder ser aberto ou parseado.
 */
static int parse_stat_file(const char *path, ProcessInfo *info)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;

    char buf[STAT_BUF_MAX];
    int ok = (fgets(buf, sizeof(buf), f) != NULL);
    fclose(f);

    if (!ok)
        return 0;

    char *after_name = strrchr(buf, ')');
    if (!after_name)
        return 0;

    unsigned long utime = 0, stime = 0;
    long nice_val = 0;

    char *token = strtok(after_name + 1, " \t");
    for (int field = STAT_FIELD_START; token && field <= STAT_FIELD_NICE; field++) {
        switch (field) {
            case STAT_FIELD_UTIME: utime    = strtoul(token, NULL, 10); break;
            case STAT_FIELD_STIME: stime    = strtoul(token, NULL, 10); break;
            case STAT_FIELD_NICE:  nice_val = strtol (token, NULL, 10); break;
            default: break;
        }
        token = strtok(NULL, " \t");
    }

    long clk_tck = sysconf(_SC_CLK_TCK);
    info->cpu_time_seconds = (clk_tck > 0)
        ? (double)(utime + stime) / clk_tck
        : 0.0;
    info->nice = nice_val;

    return 1;
}

/**
 * resolve_username - Preenche info->username a partir de info->uid.
 *
 * Se getpwuid() falhar, usa a representação numérica do UID como fallback.
 */
static void resolve_username(ProcessInfo *info)
{
    struct passwd *pw = getpwuid(info->uid);
    if (pw) {
        strncpy(info->username, pw->pw_name, sizeof(info->username) - 1);
        info->username[sizeof(info->username) - 1] = '\0';
    } else {
        snprintf(info->username, sizeof(info->username), "%u", info->uid);
    }
}


ProcResult resolve_uid(const char *username, uid_t *out_uid)
{
    if (!username || !out_uid)
        return PROC_ERR_INVALID_ARG;

    struct passwd *pw = getpwnam(username);
    if (!pw)
        return PROC_ERR_NOT_FOUND;

    *out_uid = pw->pw_uid;
    return PROC_OK;
}

ProcResult list_system_users(void)
{
    printf("\n\033[96mUsuários cadastrados no sistema\033[0m\n");
    printf("%-20s %-8s %s\n", "USUÁRIO", "UID", "SHELL");
    printf("--------------------------------------------------\n");

    setpwent(); 

    struct passwd *pw;
    while ((pw = getpwent()) != NULL) {
        printf("%-20s %-8u %s\n", pw->pw_name, pw->pw_uid, pw->pw_shell);
    }

    endpwent(); 

    return PROC_OK;
}

ProcResult fill_process_info(pid_t pid, ProcessInfo *info)
{
    if (!info)
        return PROC_ERR_INVALID_ARG;

    char path_status[LINE_BUF_MAX];
    char path_stat[LINE_BUF_MAX];

    snprintf(path_status, sizeof(path_status), "/proc/%d/status", pid);
    snprintf(path_stat,   sizeof(path_stat),   "/proc/%d/stat",   pid);

    if (!parse_status_file(path_status, info))
        return PROC_ERR_NOT_FOUND;

    if (!parse_stat_file(path_stat, info))
        return PROC_ERR_IO;

    info->pid = pid;
    resolve_username(info);

    return PROC_OK;
}

ProcResult list_processes_by_uid(uid_t target_uid, const char *username)
{
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("Erro ao abrir /proc");
        return PROC_ERR_IO;
    }

    printf("\n\033[91mProcessos do usuário %s (UID: %u)\033[0m\n",
           username, target_uid);
    printf("%-10s %-25s\n", "PID", "NOME");
    printf("--------------------------------------\n");

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!isdigit(entry->d_name[0]))
            continue;

        pid_t pid = (pid_t)atoi(entry->d_name);
        ProcessInfo info = {0};

        if (fill_process_info(pid, &info) != PROC_OK)
            continue;

        if (info.uid != target_uid)
            continue;

        printf("%-10d %-25s\n", info.pid, info.name);
    }

    closedir(proc_dir);
    return PROC_OK;
}

ProcResult print_process_info(pid_t pid)
{
    ProcessInfo info = {0};
    ProcResult result = fill_process_info(pid, &info);

    if (result == PROC_ERR_NOT_FOUND || result == PROC_ERR_NO_PERM) {
        printf("\n\033[31m[Erro]\033[0m Processo %d não encontrado "
               "ou sem permissão de leitura.\n", pid);
        return result;
    }

    if (result != PROC_OK) {
        printf("\n\033[31m[Erro]\033[0m Falha ao ler dados do processo %d.\n", pid);
        return result;
    }

    printf("\n--- Informações do Processo (PID: %d) ---\n", info.pid);
    printf("PID    (Identificador de Processo):  %d\n",      info.pid);
    printf("NICE   (Prioridade de execução):     %ld\n",     info.nice);
    printf("USER   (Proprietário):               %s\n",      info.username);
    printf("TIME   (Tempo total de execução):    %.2f s\n",  info.cpu_time_seconds);
    printf("STATUS (Estado):                     %s\n",      info.state);
    printf("-----------------------------------------\n");

    return PROC_OK;
}