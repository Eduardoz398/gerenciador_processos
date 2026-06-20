#ifndef MENU_H
#define MENU_H


typedef enum {
    MENU_LIST_USERS        = 1,  
    MENU_LIST_USER_PROCS   = 2,  
    MENU_QUERY_PID         = 3, 
    MENU_CONTROL_PROCESS   = 4,  
    MENU_CHANGE_PRIORITY   = 5, 
    MENU_RUN_WITH_PRIORITY = 6,  
    MENU_EXIT              = 7   
} MenuOption;


#define MAX_ARGV_TOKENS 64
void run_menu_loop(void);

#endif