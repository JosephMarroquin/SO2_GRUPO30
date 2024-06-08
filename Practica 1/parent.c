#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

int total_calls = 0;

void sigint_handler(int sig) {
    printf("\nRecuento de llamadas al sistema:\n");

    FILE *file;
    char line[100];
    char *token;

    int read_count = 0;
    int write_count = 0;

    // Abrir el archivo para lectura
    file = fopen("syscalls.log", "r");
    if (file == NULL) {
        perror("Error al abrir el archivo syscalls.log");
        exit(1);
    }

    // Leer el archivo línea por línea
    while (fgets(line, sizeof(line), file)) {
        // Buscar la cadena "Read" en la línea
        if (strstr(line, "Read")) {
            read_count++;
        }
        // Buscar la cadena "Write" en la línea
        if (strstr(line, "Write")) {
            write_count++;
        }
    }

    // Cerrar el archivo
    fclose(file);

    // Imprimir el recuento de llamadas
    printf("Número de llamadas de lectura (read): %d\n", read_count);
    printf("Número de llamadas de escritura (write): %d\n", write_count);

    // Imprimir el número total de llamadas al sistema
    total_calls = read_count + write_count;
    printf("Número total de llamadas al sistema: %d\n", total_calls);

    // Salir del programa
    exit(0);
}


int main(){

    signal(SIGINT, sigint_handler);

    pid_t pid1, pid2;

    pid1 = fork();

    if(pid1 == -1){
        perror("fork");
        exit(1);
    }

    if(pid1 == 0){
        // Proceso hijo 1
        printf("Soy el proceso hijo 1\n");
        printf("Mi PID es: %d\n\n", getpid());

        // Obtener el PID del proceso hijo 1
        pid_t child_pid1 = getpid();

        // Construir el comando SystemTap con el PID del proceso hijo 1 como argumento
        char command1[100];
        sprintf(command1, "stap trace.stp %d > syscalls.log &", child_pid1);
        system(command1);

        char* arg_Ptr1[4];
        arg_Ptr1[0] = " child.bin";
        arg_Ptr1[1] = " Hola";
        arg_Ptr1[2] = " Soy el proceso hijo 1! ";
        arg_Ptr1[3] = NULL;

        execv("/home/jorge/Documentos/SO2_GRUPO30/Practica 1/child.bin", arg_Ptr1);

    } else {
        pid2 = fork();
        
        if(pid2 == -1){
            perror("fork");
            exit(1);
        }

        if(pid2 == 0){
            // Proceso hijo 2
            printf("Soy el proceso hijo 2\n");
            printf("Mi PID es: %d\n\n", getpid());

            // Obtener el PID del proceso hijo 2
            pid_t child_pid2 = getpid();

            // Construir el comando SystemTap con el PID del proceso hijo 2 como argumento
            char command2[100];
            sprintf(command2, "stap trace.stp %d > syscalls.log &", child_pid2);
            system(command2);

            char* arg_Ptr2[4];
            arg_Ptr2[0] = " child.bin";
            arg_Ptr2[1] = " Hola";
            arg_Ptr2[2] = " Soy el proceso hijo 2! ";
            arg_Ptr2[3] = NULL;

            execv("/home/jorge/Documentos/SO2_GRUPO30/Practica 1/child.bin", arg_Ptr2);

        } else {
            // Proceso padre
            printf("Soy el proceso padre\n");
            printf("Mi PID es: %d\n\n", getpid());

            int status1, status2;
            waitpid(pid1, &status1, 0);
            waitpid(pid2, &status2, 0);

            if(WIFEXITED(status1)){
                printf("\nEl proceso hijo 1 terminó con estado: %d\n", status1);
            } else {
                printf("\nError: El proceso hijo 1 terminó con estado: %d\n", status1);
            }

            if(WIFEXITED(status2)){
                printf("\nEl proceso hijo 2 terminó con estado: %d\n", status2);
            } else {
                printf("\nError: El proceso hijo 2 terminó con estado: %d\n", status2);
            }

            printf("Terminando el proceso padre\n");
        }
    }

    return 0;
}
