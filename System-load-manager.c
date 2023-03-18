#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>

/* PROC_DIRECTORY_MAX_PATH_LENGTH is the maximum length of paths for /proc/[pid]/stat FILE that will be read in retrieveCpuPercentagesForProcesses(struct Process* processes, int maxNumProcesses) function where [pid] is currently examined entry's Process ID */
#define PROC_DIRECTORY_MAX_PATH_LENGTH 1024
/* PROC_DIRECTORY_MAX_COMMAND_LINE_LENGTH is the maximum length of paths for /proc/[pid]/cmdline file that will be read in retrieveCpuPercentagesForProcesses(struct Process* processes, int maxNumProcesses) function where [pid] is currently examined entry's Process ID */
#define PROC_DIRECTORY_MAX_COMMAND_LINE_LENGTH 2048

/* Create structure Process with 2 variables: pid (for Process ID of the process), cpuUsagePercent (for CPU usage percentage of the process) */
struct Process{
    pid_t pid;
    double cpuUsagePercent;
};

/* Send kill signals to kill appropriate processes */
void sendKillSignalsToProcesses(struct Process* processes, int numberOfProcesses) {
    int i = 0;
    while (i < numberOfProcesses) {
        /* Ensure that the process executing code for this program does not kill itself, then print message that shows Process ID of the process that is going to be killed and kill that process */
      if (processes[i].pid != getpid()) {
            printf("The program is sending kill signal to process whose Process ID is %d\n", processes[i].pid);
            kill(processes[i].pid, SIGKILL);
        }
        i++;
    }
}

/* Compare the CPU usage percentage of the first process and that of the second process */
int compareByCpuUsage(const void* first, const void* second) {
    /* firstProcess is for the first process */
    struct Process* firstProcess = (struct Process*) first;
    /* secondProcess is for the second process */
    struct Process* secondProcess = (struct Process*) second;
    /* difference stores the difference between CPU usage percentage of the second process and that of the first process */
    double difference = (secondProcess -> cpuUsagePercent) - (firstProcess -> cpuUsagePercent);
    /* Return the corresponding value based on whether difference is positive, equal to 0, or negative */
    if (difference > 0) return 1;
    else if (difference == 0) return 0;
    else return -1;
}

/* Retrieve CPU usage percentages for processes and return number of processes that have CPU usage percentages successfully retrieved */
int retrieveCpuPercentagesForProcesses(struct Process* processes, int maxNumProcesses) {
    /* numProcessesWithCpuPercentRetrieved is used to store number of processes that have CPU usage percentages successfully retrieved */
    int numProcessesWithCpuPercentRetrieved = 0;
    /* procDirectory is used to point to a DIR structure representing /proc directory */
    DIR* procDirectory = opendir("/proc");
    /* If the /proc directory can not be opened, return 0 (in this case, there are no processes with CPU usage percentages retrieved) and end the method */
    if (procDirectory == NULL) return 0;
    /* If the /proc directory can be opened, continue executing the following code */
    /* entry is used to point to a struct dirent structure representing each entry in the /proc directory */
    struct dirent* entry;
    /* Continue iterating over each entry in the /proc directory if number of processes with CPU usage percentage retrieved is smaller than the specified maximum number of processes and there are still more entries in the /proc directory */
    while (numProcessesWithCpuPercentRetrieved < maxNumProcesses && (entry = readdir(procDirectory)) != NULL) {
      /* If the currently examined entry is a directory, continue executing the following code */
        if (entry -> d_type == DT_DIR) {
        /* pid stores the currently examined entry's Process ID, which is found based on the currently examined entry's name */
            int pid = atoi(entry -> d_name);
        /* If the currently examined entry represents a running process (its Process ID is positive), continue executing the following code */
            if (pid > 0) {
            /* procPath is used to store the path to /proc/[pid]/stat file or /proc/[pid]/cmdline file where [pid] is the currently examined entry's Process ID */
                char procPath[PROC_DIRECTORY_MAX_PATH_LENGTH];
            /* Format a string containing the path to /proc/[pid]/stat file and store it in procPath */
                snprintf(procPath, PROC_DIRECTORY_MAX_PATH_LENGTH, "/proc/%d/stat", pid);
            /* statFile is used to point to a FILE structure representing /proc/[pid]/stat file */
                FILE* statFile = fopen(procPath, "r");
            /* If /proc/[pid]/stat file can be opened, continue executing the following code */
                if (statFile != NULL) {
                    /* Read process information from /proc/[pid]/stat file */
              /* userModeTime is used to store the amount of time that the currently examined entry spends in user mode */
                    unsigned int userModeTime;
              /* kernelModeTime is used to store the amount of time that the currently examined entry spends in kernel mode */
              unsigned int kernelModeTime;
              /* Read the amount of time that the currently examined entry spends in user mode and the amount of tume that the currently examined entry spends in kernel mode from the /proc/[pid]/stat file, then close that file */
                    fscanf(statFile, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %u %u", &userModeTime, &kernelModeTime);
                    fclose(statFile);
              /* commandLine is used to store the command line for the currently examined entry */
              char commandLine[PROC_DIRECTORY_MAX_COMMAND_LINE_LENGTH];
                    /* Format a string containing the path to /proc/[pid]/cmdline file and store it in procPath */
                    snprintf(procPath, PROC_DIRECTORY_MAX_PATH_LENGTH, "/proc/%d/cmdline", pid);
              /* commandLineFile is used to point to a struct FILE structure representing /proc/[pid]/cmdline file */
                    FILE* commandLineFile = fopen(procPath, "r");
              /* If /proc/[pid]/cmdline file can be opened, continue executing the following code */
                    if (commandLineFile != NULL) {
                /* Read /proc[pid]/cmdline file's first line (containing the command line) with maximum length of PROC_DIRECTORY_MAX_COMMAND_LINE_LENGTH, then close that file */
                        fgets(commandLine, PROC_DIRECTORY_MAX_COMMAND_LINE_LENGTH, commandLineFile);
                        fclose(commandLineFile);
                        /* Calculate the CPU usage percentage of the currently examined process (represented by currently examined entry) */
                        double total_time = (double)(userModeTime + kernelModeTime) / sysconf(_SC_CLK_TCK);
                        double percentCpuUsed = (total_time / 10.0) * 100.0;
                        /* Add the currently examined process's Process ID and the CPU usage percentage in processes */
                        processes[numProcessesWithCpuPercentRetrieved].pid = pid;
                        processes[numProcessesWithCpuPercentRetrieved].cpuUsagePercent = percentCpuUsed;
                /* Increment number of processes that have CPU usage percentages successfully retrieved */
                        numProcessesWithCpuPercentRetrieved++;
                    }
                }
            }
        }
    }
    /* Close /proc directory and return number of processes that have CPU usage percentages sucessfully retrieved */
    closedir(procDirectory);
    return numProcessesWithCpuPercentRetrieved;
}

/* Kill appropriate processes based on the ordering of CPU usage percentage */
void killProcesses(){
    /* processes is used to store up to 1000 struct Process structures */
    struct Process processes[1000];
    /* Retrieve CPU usage percentages for processes; numProcessesWithCpuPercentRetrieved stores number of processes that have CPU usage percentages successfully retrieved */
    int numProcessesWithCpuPercentRetrieved = retrieveCpuPercentagesForProcesses(processes, 1000);
    /* If there is at least 1 process that has CPU usage percentage successfully retrieved, continue executing the following code */
    if (numProcessesWithCpuPercentRetrieved >= 1) {
        /* Sort the processes by CPU usage percentage by using Quick Sort, then send kill signals to kill appropriate processes based on CPU usage percentage ordering */
        qsort(processes, numProcessesWithCpuPercentRetrieved, sizeof(struct Process), compareByCpuUsage);
        sendKillSignalsToProcesses(processes, numProcessesWithCpuPercentRetrieved);
    }
}

/* Examine the current system load; if the current system load over the last 1 minute is higher than 20, kill appropriate processes based on the ordering of CPU usage percentage */
void examineLoadAndKillProcesses() {
    /* loadAverage is used to store the system load over the last 1, 5, 15 minutes */
    double loadAverage[3];
    /* If the loadAverage array is filled with system load over the last 1, 5, 15 minutes and numbers of sample used to calculate average is returned successfully, continue executing the following code */
    if (getloadavg(loadAverage, 3) != -1) {
      /* Continue executing the following code only if the loadAverage array is filled with system load over the last 1, 5, 15 minutes and numbers of sample used to calculate average is returned successfully */
        /* currentSystemLoad stores the current system load over the last 1 minute */
        double currentSystemLoad = loadAverage[0];
        printf("Current system load over the last 1 minute is approximately equal to %.2f\n", currentSystemLoad);
        /* If the current system load over the last 1 minute is higher than 20, print messages and kill appropriate processes based on CPU usage percentage ordering */
      if (currentSystemLoad > 20) {
            printf("The current system load over the last 1 minute is higher than 20\n");
          printf("Kill the appropriate processes based on the ordering of CPU usage percentage\n");
            killProcesses();
        }
    }
}

/* Main method to execute the program */
int main() {
    /* Use fork() to create child process */
    pid_t pid = fork();
    /* If the value returned is negative (the daemon process cannot be forked), print message and exit */
    if (pid < 0) {
        printf("Cannot not fork a daemon process\n");
        exit(1);
    }
    /* If the value returned is positive (the parent process is running), print messages and parent process exits */
    else if (pid > 0) {
      printf("Parent process is running\n");
      printf("Parent process exits\n");
        exit(0);
    }
    /* If the value returned is 0, make the child process possible to run as a daemon process */
    setsid();
    /* Every 10 seconds, examine the current system load and kill appropriate processes based on CPU usage percentage ordering */
    while (1) {
      examineLoadAndKillProcesses();
        sleep(10);
    }
    return 0;
}