#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "cJSON.h"

// Definimos una estructura para almacenar los datos del JSON
typedef struct {
    int no_cuenta;
    char nombre[50];
    float saldo;
} Cliente;

// Nodo para la lista enlazada que almacenará los clientes del JSON
typedef struct Nodo {
    Cliente cliente;
    struct Nodo *siguiente;
} Nodo;

// Estructura para pasar argumentos a la función de hilo
typedef struct {
    char ruta[100];
    Nodo **lista;
    int start;
    int end;
    int *errorCount;
    char **errores;
    int cargados;
} ThreadArgs;

// Función para agregar un cliente a la lista en memoria
void agregarCliente(Nodo **lista, Cliente cliente) {
    Nodo *nuevoNodo = (Nodo*)malloc(sizeof(Nodo));
    nuevoNodo->cliente = cliente;
    nuevoNodo->siguiente = NULL;

    if (*lista == NULL) {
        *lista = nuevoNodo;
    } else {
        Nodo *actual = *lista;
        while (actual->siguiente != NULL) {
            actual = actual->siguiente;
        }
        actual->siguiente = nuevoNodo;
    }
}

// Función para leer el contenido del archivo JSON y almacenar los datos en la lista
void *leerJSON(void *arg) {
    ThreadArgs *threadArgs = (ThreadArgs*)arg;

    FILE *archivo;
    archivo = fopen(threadArgs->ruta, "r");

    if (archivo == NULL) {
        printf("No se pudo abrir el archivo.\n");
        pthread_exit(NULL);
    }

    fseek(archivo, 0, SEEK_END);
    long fileSize = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    char *jsonData = (char *)malloc(fileSize + 1);
    fread(jsonData, 1, fileSize, archivo);
    fclose(archivo);

    cJSON *json = cJSON_Parse(jsonData);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error antes: %s\n", error_ptr);
        }
        pthread_exit(NULL);
    }

    cJSON *clientes = json;
    cJSON *cliente;
    int i = 0;
    cJSON_ArrayForEach(cliente, clientes) {
        if (i >= threadArgs->start && i <= threadArgs->end) {
            cJSON *no_cuenta = cJSON_GetObjectItem(cliente, "no_cuenta");
            cJSON *nombre = cJSON_GetObjectItem(cliente, "nombre");
            cJSON *saldo = cJSON_GetObjectItem(cliente, "saldo");

            if (no_cuenta == NULL || nombre == NULL || saldo == NULL || !cJSON_IsNumber(no_cuenta) || !cJSON_IsString(nombre) || !cJSON_IsNumber(saldo)) {
                char error[100];
                snprintf(error, sizeof(error), "Linea #%d: Formato de datos incorrecto en el JSON", i+1);
                threadArgs->errores[*threadArgs->errorCount] = strdup(error);
                (*threadArgs->errorCount)++;
                continue; // Omitir el registro con error
            }

            int cuenta = no_cuenta->valueint;
            if (cuenta <= 0) {
                char error[100];
                snprintf(error, sizeof(error), "Linea #%d: Número de cuenta no es un entero positivo", i+1);
                threadArgs->errores[*threadArgs->errorCount] = strdup(error);
                (*threadArgs->errorCount)++;
                continue;
            }

            float saldoValue = saldo->valuedouble;
            if (saldoValue <= 0) {
                char error[100];
                snprintf(error, sizeof(error), "Linea #%d: Saldo introducido no es un número real positivo", i+1);
                threadArgs->errores[*threadArgs->errorCount] = strdup(error);
                (*threadArgs->errorCount)++;
                continue;
            }

            // Verificar si el número de cuenta ya existe
            Nodo *actual = *threadArgs->lista;
            while (actual != NULL) {
                if (actual->cliente.no_cuenta == cuenta) {
                    char error[100];
                    snprintf(error, sizeof(error), "Linea #%d: Número de cuenta duplicado", i+1);
                    threadArgs->errores[*threadArgs->errorCount] = strdup(error);
                    (*threadArgs->errorCount)++;
                    break;
                }
                actual = actual->siguiente;
            }

            if (actual == NULL) {
                Cliente nuevoCliente;
                nuevoCliente.no_cuenta = cuenta;
                strcpy(nuevoCliente.nombre, nombre->valuestring);
                nuevoCliente.saldo = saldoValue;
                agregarCliente(threadArgs->lista, nuevoCliente);
                threadArgs->cargados++; // Incrementar contador de usuarios cargados
            }
        }
        i++;
    }

    cJSON_Delete(json);
    pthread_exit(NULL);
}

// Función para escribir la lista de clientes en un archivo JSON 
void escribirListaEnJSON(Nodo *lista) {
    const char *nombreArchivo = "estado_cuentas.json"; 
    FILE *archivo = fopen(nombreArchivo, "w");
    if (archivo == NULL) {
        printf("No se pudo abrir el archivo para escritura.\n");
        return;
    }

    fprintf(archivo, "[\n");
    Nodo *actual = lista;
    while (actual != NULL) {
        fprintf(archivo, "  {\n");
        fprintf(archivo, "    \"no_cuenta\": %d,\n", actual->cliente.no_cuenta);
        fprintf(archivo, "    \"nombre\": \"%s\",\n", actual->cliente.nombre);
        fprintf(archivo, "    \"saldo\": %.2f\n", actual->cliente.saldo);
        fprintf(archivo, "  }%s\n", (actual->siguiente != NULL) ? "," : "");
        actual = actual->siguiente;
    }
    fprintf(archivo, "]\n");

    fclose(archivo);
}

// Función para imprimir la lista de clientes
void imprimirLista(Nodo *lista) {
    printf("Guardando la lista de clientes en el archivo JSON 'estado_cuentas.json'...\n");
    escribirListaEnJSON(lista);
    printf("La lista de clientes se ha guardado correctamente en el archivo JSON 'estado_cuentas.json'.\n");
}

// Función para generar el reporte de carga
void generarReporte(int totalCargados, int *cargadosPorHilo, int errorCount, char **errores) {
    char filename[100];
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(filename, sizeof(filename), "carga_%Y_%m_%d-%H_%M_%S.log", timeinfo);

    FILE *reporte = fopen(filename, "w");
    if (reporte == NULL) {
        printf("No se pudo crear el archivo de reporte.\n");
        return;
    }

    fprintf(reporte, "Fecha: %s", asctime(timeinfo));
    fprintf(reporte, "Usuarios Cargados:\n");
    for (int i = 0; i < 3; i++) {
        fprintf(reporte, "Hilo %d: %d\n", i+1, cargadosPorHilo[i]);
    }
    fprintf(reporte, "Total de Usuarios Cargados: %d\n", totalCargados);
    fprintf(reporte, "Errores:\n");
    for (int i = 0; i < errorCount; i++) {
        fprintf(reporte, "- %s\n", errores[i]);
    }

    fclose(reporte);
}

int main() {
    int opcion;
    Nodo *listaClientes = NULL; // Lista para almacenar los clientes del JSON

    do {
        printf("\nMenú:\n");
        printf("1. Carga masiva de usuarios\n");
        printf("2. Generar reporte Estado de cuentas\n");
        printf("3. Salir\n");
        printf("Selecciona una opción: ");
        scanf("%d", &opcion);

        switch(opcion) {
            case 1: {
                char ruta[100];
                printf("Introduce la ruta del archivo JSON: ");
                scanf("%s", ruta);

                FILE *archivo;
                archivo = fopen(ruta, "r");

                if (archivo == NULL) {
                    printf("No se pudo abrir el archivo.\n");
                    break;
                }

                fseek(archivo, 0, SEEK_END);
                long fileSize = ftell(archivo);
                fseek(archivo, 0, SEEK_SET);

                char *jsonData = (char *)malloc(fileSize + 1);
                fread(jsonData, 1, fileSize, archivo);
                fclose(archivo);

                // Parsear el JSON
                cJSON *json = cJSON_Parse(jsonData);
                if (json == NULL) {
                    // Manejar el error de parsing
                    printf("Error al parsear el JSON.\n");
                    free(jsonData); // Liberar la memoria
                    return 1;
                }

                // Obtener el número de elementos en el array JSON
                int totalUsuarios = cJSON_GetArraySize(json);
                //printf("Total de usuarios en el archivo: %d\n", totalUsuarios);
                cJSON_Delete(json);
                free(jsonData);

                // Calcular cuántos usuarios debe cargar cada hilo
                int usuariosPorHilo = totalUsuarios / 3;

                // Crear argumentos para cada hilo
                pthread_t threads[3];
                ThreadArgs args[3];
                int i;
                int errorCount = 0;
                char *errores[100]; // Suponemos un máximo de 100 errores

                for (i = 0; i < 3; i++) {
                    args[i].lista = &listaClientes;
                    strcpy(args[i].ruta, ruta);
                    args[i].start = i * usuariosPorHilo;
                    args[i].end = (i + 1) * usuariosPorHilo - 1;
                    args[i].errorCount = &errorCount;
                    args[i].errores = errores;
                    args[i].cargados = 0; // Inicializar contador de usuarios cargados
                    pthread_create(&threads[i], NULL, leerJSON, (void*)&args[i]);
                }

                // Esperar a que todos los hilos terminen
                for (i = 0; i < 3; i++) {
                    pthread_join(threads[i], NULL);
                }

                // Calcular total de usuarios cargados
                int totalCargados = args[0].cargados + args[1].cargados + args[2].cargados;

                // Generar el reporte de carga
                generarReporte(totalCargados, (int[]){args[0].cargados, args[1].cargados, args[2].cargados}, errorCount, errores);
                break;
            }
            case 2:
                imprimirLista(listaClientes);
                break;
            case 3:
                printf("Saliendo del programa.\n");
                break;
            default:
                printf("Opción no válida. Por favor, selecciona una opción válida.\n");
        }
    } while(opcion != 3);

    // Liberamos la memoria de la lista antes de salir
    while (listaClientes != NULL) {
        Nodo *temp = listaClientes;
        listaClientes = listaClientes->siguiente;
        free(temp);
    }

    return 0;
}
