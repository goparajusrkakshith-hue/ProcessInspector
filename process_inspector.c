#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

#define BUFFER_SIZE 512

typedef struct {
    unsigned long long utime;
    unsigned long long stime;
    long rss_pages;
} ProcessStat;

/* ---------------------------------------------------------
   Read /proc/<PID>/status
   --------------------------------------------------------- */
int read_process_status(pid_t pid,
                        char *name,
                        char *state,
                        pid_t *ppid,
                        long *rss_kb,
                        long *threads)
{
    char path[BUFFER_SIZE];
    char line[BUFFER_SIZE];

    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        fprintf(stderr,
                "Error: Cannot open %s: %s\n",
                path,
                strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {

        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line, "Name:\t%255[^\n]", name);
        }
        else if (strncmp(line, "State:", 6) == 0) {
            sscanf(line, "State:\t%255[^\n]", state);
        }
        else if (strncmp(line, "PPid:", 5) == 0) {
            sscanf(line, "PPid:\t%d", ppid);
        }
        else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS:\t%ld kB", rss_kb);
        }
        else if (strncmp(line, "Threads:", 8) == 0) {
            sscanf(line, "Threads:\t%ld", threads);
        }
    }
    fclose(fp);
    return 0;
}

/* ---------------------------------------------------------
   Read /proc/<PID>/stat
   --------------------------------------------------------- */
int read_process_stat(pid_t pid, ProcessStat *stat)
{
    char path[BUFFER_SIZE];
    char line[BUFFER_SIZE * 2];

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        return -1;
    }

    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    /*
       /proc/<PID>/stat format:

       field 1  = pid
       field 2  = process name
       field 3  = state
       ...
       field 14 = utime
       field 15 = stime
       field 24 = rss

       Process name is inside parentheses, so find
       the LAST ')' before parsing the remaining fields.
    */

    char *closing_paren = strrchr(line, ')');

    if (closing_paren == NULL) {
        return -1;
    }

    char *ptr = closing_paren + 2;

    int field = 3;
    char *token = strtok(ptr, " ");

    while (token != NULL) {

        if (field == 14) {
            stat->utime = strtoull(token, NULL, 10);
        }
        else if (field == 15) {
            stat->stime = strtoull(token, NULL, 10);
        }
        else if (field == 24) {
            stat->rss_pages = strtol(token, NULL, 10);
            break;
        }

        field++;
        token = strtok(NULL, " ");
    }

    return 0;
}

/* ---------------------------------------------------------
   Read total CPU ticks from /proc/stat
   --------------------------------------------------------- */
unsigned long long read_total_cpu_ticks(void)
{
    FILE *fp = fopen("/proc/stat", "r");

    if (fp == NULL) {
        return 0;
    }

    char line[BUFFER_SIZE];

    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;

    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return 0;
    }

    fclose(fp);

    if (sscanf(line,
               "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &user,
               &nice,
               &system,
               &idle,
               &iowait,
               &irq,
               &softirq,
               &steal) < 4) {
        return 0;
    }

    return user + nice + system + idle +
           iowait + irq + softirq + steal;
}

/* ---------------------------------------------------------
   Calculate CPU usage
   --------------------------------------------------------- */
double calculate_cpu_usage(pid_t pid)
{
    ProcessStat first;
    ProcessStat second;

    unsigned long long total_first;
    unsigned long long total_second;

    total_first = read_total_cpu_ticks();

    if (read_process_stat(pid, &first) != 0) {
        return -1.0;
    }

    /*
       Take second sample after 1 second.
    */
    sleep(1);

    total_second = read_total_cpu_ticks();

    if (read_process_stat(pid, &second) != 0) {
        return -1.0;
    }

    unsigned long long process_first =
        first.utime + first.stime;

    unsigned long long process_second =
        second.utime + second.stime;

    unsigned long long process_delta =
        process_second - process_first;

    unsigned long long total_delta =
        total_second - total_first;

    if (total_delta == 0) {
        return 0.0;
    }

    long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);

    if (cpu_count < 1) {
        cpu_count = 1;
    }

    double cpu_usage =
        100.0 *
        (double)process_delta /
        (double)total_delta *
        (double)cpu_count;

    return cpu_usage;
}

/* ---------------------------------------------------------
   Read system memory
   --------------------------------------------------------- */
int read_memory(long *total_kb,
                long *available_kb)
{
    FILE *fp = fopen("/proc/meminfo", "r");

    if (fp == NULL) {
        return -1;
    }

    char line[BUFFER_SIZE];

    *total_kb = 0;
    *available_kb = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {

        if (sscanf(line,
                   "MemTotal: %ld kB",
                   total_kb) == 1) {
            continue;
        }

        if (sscanf(line,
                   "MemAvailable: %ld kB",
                   available_kb) == 1) {
            continue;
        }
    }

    fclose(fp);

    return 0;
}

/* ---------------------------------------------------------
   Display system memory
   --------------------------------------------------------- */
void display_system_memory(void)
{
    long total_kb;
    long available_kb;

    if (read_memory(&total_kb, &available_kb) != 0) {
        printf("\nUnable to read system memory information.\n");
        return;
    }

    long used_kb = total_kb - available_kb;

    double used_percentage = 0.0;

    if (total_kb > 0) {
        used_percentage =
            100.0 * (double)used_kb /
            (double)total_kb;
    }

    printf("\n");
    printf("=============================================\n");
    printf("              SYSTEM MEMORY\n");
    printf("=============================================\n");

    printf("Total RAM     : %ld kB\n", total_kb);
    printf("Available RAM : %ld kB\n", available_kb);
    printf("Used RAM      : %ld kB\n", used_kb);
    printf("Usage         : %.2f %%\n", used_percentage);

    printf("=============================================\n");
}

/* ---------------------------------------------------------
   Inspect a process
   --------------------------------------------------------- */
void inspect_process(pid_t pid)
{
    char name[BUFFER_SIZE] = "Unknown";
    char state[BUFFER_SIZE] = "Unknown";

    pid_t ppid = 0;
    long rss_kb = 0;
    long threads = 0;

    ProcessStat stat;

    printf("\n");
    printf("=============================================\n");
    printf("             PROCESS INFORMATION\n");
    printf("=============================================\n");

    if (read_process_status(pid,
                            name,
                            state,
                            &ppid,
                            &rss_kb,
                            &threads) != 0) {

        printf("Could not inspect PID %d.\n", pid);
        printf("=============================================\n");
        return;
    }

    printf("PID          : %d\n", pid);
    printf("Name         : %s\n", name);
    printf("State        : %s\n", state);
    printf("Parent PID   : %d\n", ppid);
    printf("Threads      : %ld\n", threads);
    printf("VmRSS        : %ld kB\n", rss_kb);

    /* Read RSS from /proc/<PID>/stat */
    if (read_process_stat(pid, &stat) == 0) {

        long page_size = sysconf(_SC_PAGESIZE);

        if (page_size < 1) {
            page_size = 4096;
        }

        long rss_from_stat_kb =
            (stat.rss_pages * page_size) / 1024;

        printf("RSS          : %ld kB\n",
               rss_from_stat_kb);
    }
    else {
        printf("RSS          : Unable to read\n");
    }

    printf("\nCalculating CPU usage...\n");

    double cpu_usage = calculate_cpu_usage(pid);

    if (cpu_usage >= 0.0) {
        printf("CPU Usage    : %.2f %%\n",
               cpu_usage);
    }
    else {
        printf("CPU Usage    : Unable to calculate\n");
    }

    printf("=============================================\n");
}

/* ---------------------------------------------------------
   Get PID from user
   --------------------------------------------------------- */
int get_pid_from_user(pid_t *pid)
{
    char input[BUFFER_SIZE];

    printf("Enter PID: ");

    if (fgets(input, sizeof(input), stdin) == NULL) {
        return -1;
    }

    char *end;

    long value = strtol(input, &end, 10);

    if (end == input ||
        value <= 0 ||
        value > INT_MAX) {

        printf("Invalid PID.\n");
        return -1;
    }

    while (*end == ' ' || *end == '\t') {
        end++;
    }

    if (*end != '\n' && *end != '\0') {
        printf("Invalid PID.\n");
        return -1;
    }

    *pid = (pid_t)value;

    return 0;
}

/* ---------------------------------------------------------
   Main Menu
   --------------------------------------------------------- */
int main(void)
{
    int choice;

    while (1) {

        printf("\n");
        printf("=============================================\n");
        printf("       PROCESS CPU & MEMORY INSPECTOR\n");
        printf("=============================================\n");

        printf("1. Inspect Current Process\n");
        printf("2. Inspect a Process by PID\n");
        printf("3. View System Memory\n");
        printf("4. Test PID / Error Handling\n");
        printf("5. Exit\n");

        printf("=============================================\n");
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) {

            printf("\nInvalid choice. Please enter a number.\n");

            while (getchar() != '\n') {
                /* Clear input buffer */
            }

            continue;
        }

        /* Clear remaining input */
        while (getchar() != '\n') {
            /* Clear input buffer */
        }

        /* -------------------------------------------------
           OPTION 1
           Automatically inspect current process
           ------------------------------------------------- */
        if (choice == 1) {

            pid_t current_pid = getpid();

            printf("\n");
            printf("Inspecting CURRENT PROCESS...\n");
            printf("PID detected automatically: %d\n",
                   current_pid);

            inspect_process(current_pid);
        }

        /* -------------------------------------------------
           OPTION 2
           User enters a PID
           ------------------------------------------------- */
        else if (choice == 2) {

            pid_t pid;

            printf("\n");
            printf("=============================================\n");
            printf("           INSPECT PROCESS BY PID\n");
            printf("=============================================\n");

            if (get_pid_from_user(&pid) == 0) {
                inspect_process(pid);
            }
        }

        /* -------------------------------------------------
           OPTION 3
           System memory
           ------------------------------------------------- */
        else if (choice == 3) {

            display_system_memory();
        }

        /* -------------------------------------------------
           OPTION 4
           Error handling / invalid PID testing
           ------------------------------------------------- */
        else if (choice == 4) {

            pid_t pid;

            printf("\n");
            printf("=============================================\n");
            printf("          TEST PID / ERROR HANDLING\n");
            printf("=============================================\n");

            if (get_pid_from_user(&pid) == 0) {

                printf("\nTesting PID %d...\n", pid);

                inspect_process(pid);
            }
        }

        /* -------------------------------------------------
           OPTION 5
           Exit
           ------------------------------------------------- */
        else if (choice == 5) {

            printf("\nExiting Process Inspector...\n");
            printf("Thank you!\n");

            break;
        }

        else {

            printf("\nInvalid choice. Please select 1-5.\n");
        }
    }

    return 0;
}
