#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#define FILE_NAME "practica1.txt"
#define BUFFER_SIZE 9

// Función para generar un carácter alfanumérico aleatorio
char generate_random_char() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int index = rand() % (sizeof(charset) - 1);
    return charset[index];
}

// Función para generar una cadena de texto aleatoria
void generate_random_string(char *buffer, int length) {
    for (int i = 0; i < length; ++i) {
        buffer[i] = generate_random_char();
    }
    buffer[length] = '\0';
}

int main() {
    srand(time(NULL)); // Inicializar la generación de números aleatorios
    
    // Abrir o crear el archivo en modo de escritura
    int file_descriptor = open(FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (file_descriptor == -1) {
        perror("Error al abrir/crear el archivo");
        return 1;
    }

    // Ciclo infinito para simular llamadas de sistema
    while (1) {
        // Realizar una llamada de sistema aleatoria
        int syscall_type = rand() % 3; // 0: Open, 1: Write, 2: Read
        switch (syscall_type) {
            case 0:
                // Llamada de sistema Open
                break;
            case 1:
                // Llamada de sistema Write
                char buffer[BUFFER_SIZE];
                generate_random_string(buffer, BUFFER_SIZE - 1); // Evita escribir el salto de línea
                strcat(buffer, "\n"); // Agregar el salto de línea al final de la cadena
                write(file_descriptor, buffer, BUFFER_SIZE);
                break;
            case 2:
                // Llamada de sistema Read
                char read_buffer[BUFFER_SIZE];
                read(file_descriptor, read_buffer, BUFFER_SIZE);
                break;
            default:
                break;
        }

        // Esperar un tiempo aleatorio de 1 a 3 segundos antes de la próxima llamada de sistema
        sleep(rand() % 3 + 1);
    }

    // Cerrar el archivo
    close(file_descriptor);

    return 0;
}
