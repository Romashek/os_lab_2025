#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    printf("=== CLEAR ZOMBIE PROCESS DEMONSTRATION ===\n\n");
    
    printf("Parent process PID: %d\n", getpid());
    printf("Creating 3 child processes...\n\n");
    
    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process
            printf("Child %d (PID: %d) started - will exit in %d seconds\n", 
                   i, getpid(), i + 1);
            sleep(i + 1); // Different exit times
            printf("Child %d (PID: %d) EXITING NOW!\n", i, getpid());
            exit(0);
        } else if (pid > 0) {
            printf("Created child %d with PID: %d\n", i, pid);
        }
    }
    
    printf("\n*** ALL CHILD PROCESSES CREATED ***\n");
    printf("They will become zombies in 1-3 seconds\n\n");
    
    printf("Commands to check (run in ANOTHER terminal):\n");
    printf("1. Quick check: ps aux | grep '%s'\n", "zombie_proc");
    printf("2. Detailed check: ps -eo pid,ppid,state,comm | grep zombie_proc\n");
    printf("3. Zombies only: ps -eo pid,ppid,state,comm | awk '$3==\"Z\"'\n\n");
    
    printf("Parent process will sleep for 15 seconds...\n");
    printf("Check for zombies in another terminal during this time!\n\n");
    
    for (int i = 1; i <= 15; i++) {
        printf("Elapsed time: %d seconds...\n", i);
        sleep(1);
    }
    
    printf("\nNow collecting zombie processes...\n");
    
    int collected = 0;
    while (1) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        
        if (pid > 0) {
            printf("Collected zombie process PID: %d\n", pid);
            collected++;
        } else if (pid == 0) {
            break;
        } else {
            break;
        }
    }
    
    printf("Collected %d zombie processes\n", collected);
    printf("Program finished.\n");
    
    return 0;
}