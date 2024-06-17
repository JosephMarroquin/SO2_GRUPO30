## 202010316 Joseph Jeferson Marroquín Monroy
## 201902128 Jorge Mario Cano Blanco


# Práctica 2



## Código Proceso Padre

Código para crear los procesos hijos donde se obtiene su PID para posteriormente construir el comando para ejecutar el script de Systemtap y enviarle el PID como argumento para que lo pueda buscar e ir guardando sus datos en el archivo de syscalls.log, este código se utiliza para ambos procesos hijos

    id1 = fork();

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

    } 
    En este código se imprime el identificador del padre y se queda en espera a que los procesos hijos se terminen, cuando estos han terminado imprime que los procesos hijos han terminado y después termina el proceso padre.    

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

## Código Child

    En este código se abre el archivo de practica1.txt en modo escritura para que los procesos puedan leerlo y así simular los subprocesos de estos haciendo llamadas al sistema todo esto de manera aleatorio con un tiempo de 1 a 3 segundos cada llamada.

     int file_descriptor = open(FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (file_descriptor == -1) {
        perror("Error al abrir/crear el archivo");
        return 1;
    }

    
    while (1) {
        
        int syscall_type = rand() % 3; // 0: Open, 1: Write, 2: Read
        switch (syscall_type) {
            case 0:
                
                break;
            case 1:
               
                char buffer[BUFFER_SIZE];
                generate_random_string(buffer, BUFFER_SIZE - 1); // Evita escribir el salto de línea
                strcat(buffer, "\n"); // Agregar el salto de línea al final de la cadena
                write(file_descriptor, buffer, BUFFER_SIZE);
                break;
            case 2:
               
                char read_buffer[BUFFER_SIZE];
                read(file_descriptor, read_buffer, BUFFER_SIZE);
                break;
            default:
                break;
        }

        
        sleep(rand() % 3 + 1);
    }

   
    close(file_descriptor);

    return 0;

## Script SystemTap

    Script creado con systemtap donde hace llamadas de open, read, write al sistema enviandole el PID y al encontrar este proceso imprime la llamada que se está haciendo junto con la fecha en que se hizo.

        #!/usr/bin/stap

        probe syscall.open {
            if (pid() == $1) {
                printf("PID[%d] -> Open at %s\n", pid(), ctime(gettimeofday_s()));
            }
        }

        probe syscall.read {
            if(pid() == $1){
                printf("PID[%d] -> Read at %s\n", pid(), ctime(gettimeofday_s()));
            }
        }

        probe syscall.write {
            if(pid() == $1){
                printf("PID[%d] -> Write at %s\n", pid(), ctime(gettimeofday_s()));
            }
        }


## Archivo syscall.log

    Resultado de los logs de los procesos creados con la información de la llamada incluyendo el nombre del proceso, la fecha y hora.

        PID[12115] -> Read at Sat Jun  8 19:58:18 2024
        PID[12117] -> Read at Sat Jun  8 19:58:18 2024
        PID[12115] -> Read at Sat Jun  8 19:58:19 2024
        PID[12117] -> Read at Sat Jun  8 19:58:19 2024
        PID[12115] -> Write at Sat Jun  8 19:58:22 2024


## Archivo practica1.txt

    Archivo donde se crean cadenas de texto aleatorias para que después puedan ser leídas por los procesos.

        9wVIViMP
        8By51EIh
        fgScBCIm
        1sEZT2SJ
        QzAu9pQc
        SWXhuMxW
        MubqxwPz
        Xgok0PrD
