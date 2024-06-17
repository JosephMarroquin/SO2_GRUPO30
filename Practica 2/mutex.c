#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "cJSON.h"

pthread_mutex_t lock;
pthread_mutex_t report_lock;

// Estructura para representar una operación
typedef struct {
    int operacion;
    int cuenta1;
    int cuenta2;
    double monto;
} Operation;

// Variables globales para el reporte y conteo de operaciones
int total_operations = 0;
int thread_operations[4] = {0};
int total_retiros = 0;
int total_depositos = 0;
int total_transferencias = 0;
char error_log[1024] = "";

// Función para procesar una operación (simulación de proceso)
void process_operation(Operation *op) {
    // Simulación de procesamiento
    usleep(200000); // Simula una operación que tarda 200ms
}

// Función para registrar errores
void register_error(int line, const char *error_msg) {
    pthread_mutex_lock(&report_lock);
    char line_error[128];
    sprintf(line_error, "Linea #%d: %s\n", line, error_msg);
    strcat(error_log, line_error);
    pthread_mutex_unlock(&report_lock);
}

// Función para escribir el reporte de operaciones
void write_report() {
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "operaciones_%Y_%m_%d-%H_%M_%S.log", timeinfo);
    FILE *report_file = fopen(timestamp, "w");
    if (!report_file) {
        perror("Error al abrir el archivo de reporte");
        return;
    }
    pthread_mutex_lock(&report_lock);
    fprintf(report_file, "Resumen de operaciones\n");
    fprintf(report_file, "Fecha: %04d-%02d-%02d %02d:%02d:%02d\n",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    fprintf(report_file, "Operaciones realizadas: Retiros: %d Depositos: %d Transferencias: %d\n",
            total_retiros, total_depositos, total_transferencias);
    fprintf(report_file, "Total: %d\n", total_operations);
    fprintf(report_file, "Operaciones por hilo:\n");
    for (int i = 0; i < 4; ++i) {
        fprintf(report_file, "Hilo #%d: %d\n", i + 1, thread_operations[i]);
    }
    //fprintf(report_file, "Total: %d\n", total_operations);
    if (strlen(error_log) > 0) {
        fprintf(report_file, "Errores:\n");
        fprintf(report_file, "%s", error_log);
    }
    pthread_mutex_unlock(&report_lock);
    fclose(report_file);
}

// Función que ejecuta cada hilo
void *thread_function(void *arg) {
    char *filename = (char *)arg;
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error al abrir el archivo");
        pthread_exit(NULL);
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *json_data = (char *)malloc(file_size + 1);
    if (!json_data) {
        fclose(file);
        perror("Error al reservar memoria para json_data");
        pthread_exit(NULL);
    }
    fread(json_data, 1, file_size, file);
    fclose(file);
    json_data[file_size] = '\0'; // Añadir terminador nulo al final del JSON
    cJSON *root = cJSON_Parse(json_data);
    free(json_data);
    if (!root) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error antes de: %s\n", error_ptr);
        }
        pthread_exit(NULL);
    }
    int num_operations = cJSON_GetArraySize(root);
    if (num_operations <= 0) {
        cJSON_Delete(root);
        fprintf(stderr, "Error: El JSON no contiene operaciones válidas.\n");
        pthread_exit(NULL);
    }
    for (int i = 0; i < num_operations; ++i) {
        cJSON *op_item = cJSON_GetArrayItem(root, i);
        Operation op;
        cJSON *operacion = cJSON_GetObjectItem(op_item, "operacion");
        cJSON *cuenta1 = cJSON_GetObjectItem(op_item, "cuenta1");
        cJSON *cuenta2 = cJSON_GetObjectItem(op_item, "cuenta2");
        cJSON *monto = cJSON_GetObjectItem(op_item, "monto");
        if (!operacion || !cuenta1 || !cuenta2 || !monto) {
            cJSON_Delete(root);
            fprintf(stderr, "Error: Alguno de los campos de la operación es nulo.\n");
            pthread_exit(NULL);
        }
        op.operacion = operacion->valueint;
        op.cuenta1 = cuenta1->valueint;
        op.cuenta2 = cuenta2->valueint;
        op.monto = monto->valuedouble;
        
        // Validar la operación y procesarla
        pthread_mutex_lock(&lock);
        total_operations++;
        pthread_mutex_unlock(&lock);
        if (op.monto <= 0) {
            register_error(__LINE__, "Monto no es válido");
continue;
}
        switch (op.operacion) {
            case 1:
                // Depósito
                pthread_mutex_lock(&lock);
                total_depositos++;
                thread_operations[i % 4]++;
                pthread_mutex_unlock(&lock);
                break;
            case 2:
                // Retiro
                pthread_mutex_lock(&lock);
                total_retiros++;
                thread_operations[i % 4]++;
                pthread_mutex_unlock(&lock);
                break;
            case 3:
                // Transferencia
                pthread_mutex_lock(&lock);
                total_transferencias++;
                thread_operations[i % 4]++;
                pthread_mutex_unlock(&lock);
                break;
            default:
                // Operación inválida
                register_error(__LINE__, "Operación no válida");
                break;
        }
        
        // Simulamos un proceso que toma un tiempo
        //process_operation(&op);
        
        // Imprimir los valores de la operación de manera sincronizada
        pthread_mutex_lock(&lock);
        printf("Operacion: %d, Cuenta1: %d, Cuenta2: %d, Monto: %.2f\n",
                op.operacion, op.cuenta1, op.cuenta2, op.monto);
        pthread_mutex_unlock(&lock);
    }
    
    cJSON_Delete(root);
    pthread_exit(NULL);
}

// Función principal
int main() {
    pthread_t threads[4];
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&report_lock, NULL);
    char filename[100];
    
    printf("Ingrese la ubicacion del archivo JSON: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = 0; // Eliminar el salto de línea
    
    for (int i = 0; i < 4; ++i) {
        pthread_create(&threads[i], NULL, thread_function, filename);
    }
    
    for (int i = 0; i < 4; ++i) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&lock);
    write_report();
    
    return 0;
}
