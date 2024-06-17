#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "cJSON.h"
#include <unistd.h>


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


void realizarDeposito(Nodo *listaClientes, int no_cuenta, double monto) {
    Nodo *actual = listaClientes;

    if (monto <= 0) {
        printf("Error: El monto indicado debe ser mayor que 0.\n");
        return;
    }

    

    // Buscar el cliente según el número de cuenta
    while (actual != NULL) {
        if (actual->cliente.no_cuenta == no_cuenta) {
            // Realizar el depósito
            actual->cliente.saldo += monto;
            printf("----------------------------------\n");
            printf("Depósito de %.2f realizado correctamente en la cuenta %d.\n", monto, no_cuenta);
            printf("----------------------------------\n");
            return;
        }
        actual = actual->siguiente;
    }

    // Si el número de cuenta no se encuentra en la lista
    printf("No se encontró la cuenta con número %d. El depósito no pudo ser realizado.\n", no_cuenta);
}


void realizarRetiro(Nodo *listaClientes, int no_cuenta, double monto) {
    Nodo *actual = listaClientes;

      if (monto <= 0) {
        printf("Error: El monto indicado debe ser mayor que 0.\n");
        return;
    }

    // Buscar el cliente según el número de cuenta
    while (actual != NULL) {
        if (actual->cliente.no_cuenta == no_cuenta) {
            // Verificar si el cliente tiene suficiente saldo para el retiro
            if (actual->cliente.saldo >= monto) {
                // Realizar el retiro
                actual->cliente.saldo -= monto;
                printf("----------------------------------\n");
                printf("Retiro de %.2f realizado correctamente en la cuenta %d.\n", monto, no_cuenta);
                printf("----------------------------------\n");
            } else {
                printf("El cliente con cuenta %d no tiene suficiente saldo para realizar el retiro de %.2f.\n", no_cuenta, monto);
            }
            return;
        }
        actual = actual->siguiente;
    }

    // Si el número de cuenta no se encuentra en la lista
    printf("No se encontró la cuenta con número %d. El retiro no pudo ser realizado.\n", no_cuenta);
}

void transferencia(Nodo *listaClientes, int cuenta_origen, int cuenta_destino, double monto) {
    Nodo *actual = listaClientes;
    Nodo *cliente_origen = NULL;
    Nodo *cliente_destino = NULL;

      if (monto <= 0) {
        printf("Error: El monto indicado debe ser mayor que 0.\n");
        return;
    }

    
    while (actual != NULL) {
        if (actual->cliente.no_cuenta == cuenta_origen) {
            cliente_origen = actual;
        }
        if (actual->cliente.no_cuenta == cuenta_destino) {
            cliente_destino = actual;
        }

       
        if (cliente_origen != NULL && cliente_destino != NULL) {
            break;
        }

        actual = actual->siguiente;
    }

    
    if (cliente_origen == NULL) {
        printf("No se encontró la cuenta de origen con número %d. La transferencia no pudo ser realizada.\n", cuenta_origen);
        return;
    }

    if (cliente_destino == NULL) {
        printf("No se encontró la cuenta de destino con número %d. La transferencia no pudo ser realizada.\n", cuenta_destino);
        return;
    }

    
    if (cliente_origen->cliente.saldo >= monto) {
        cliente_origen->cliente.saldo -= monto;
        printf("----------------------------------\n");
        printf("Retiro de %.2f realizado correctamente en la cuenta %d.\n", monto, cuenta_origen);
        printf("----------------------------------\n");

     
        cliente_destino->cliente.saldo += monto;
        printf("----------------------------------\n");
        printf("Depósito de %.2f realizado correctamente en la cuenta %d.\n", monto, cuenta_destino);
        printf("----------------------------------\n");

        
        printf("Transferencia de %.2f realizada con éxito desde la cuenta %d a la cuenta %d.\n", monto, cuenta_origen, cuenta_destino);
    } else {
        printf("La cuenta de origen con número %d no tiene suficiente saldo para realizar la transferencia de %.2f.\n", cuenta_origen, monto);
    }
}




void consultar_cuenta (Nodo *listaClientes, int no_cuenta) {
    Nodo *actual = listaClientes;

    // Buscar el cliente según el número de cuenta
    while (actual != NULL) {
        if (actual->cliente.no_cuenta == no_cuenta) {
            printf("----------------------------------\n");
            printf("Información de la cuenta:\n");
            printf("Número de cuenta: %d\n", actual->cliente.no_cuenta);
            printf("Nombre: %s\n", actual->cliente.nombre);
            printf("Saldo: %.2f\n", actual->cliente.saldo);
            printf("----------------------------------\n");
            
            return;
        }
        actual = actual->siguiente;
    }

    // Si el número de cuenta no se encuentra en la lista
    printf("No se encontró la cuenta con número %d. El retiro no pudo ser realizado.\n", no_cuenta);
}

void *thread_function(void *arg) {

    ThreadArgs *args = (ThreadArgs *)arg;
    
    Nodo **listaClientes = args->lista;

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
                //printf("Realizando depósito: cuenta1 = %d, monto = %.2f\n", op.cuenta1, op.monto);
                realizarDeposito(*listaClientes,op.cuenta1,op.monto);
                pthread_mutex_unlock(&lock);
                break;
            case 2:
                // Retiro
                pthread_mutex_lock(&lock);
                total_retiros++;
                thread_operations[i % 4]++;
                realizarRetiro(*listaClientes, op.cuenta1, op.monto);
                pthread_mutex_unlock(&lock);
                break;
            case 3:
                // Transferencia
                pthread_mutex_lock(&lock);
                total_transferencias++;
                thread_operations[i % 4]++;
                //realizarRetiro(*listaClientes, op.cuenta1, op.monto);
                //realizarDeposito(*listaClientes,op.cuenta2,op.monto);
                transferencia(*listaClientes, op.cuenta1,op.cuenta2, op.monto);
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
        //printf("Operacion: %d, Cuenta1: %d, Cuenta2: %d, Monto: %.2f\n",
        //        op.operacion, op.cuenta1, op.cuenta2, op.monto);
        pthread_mutex_unlock(&lock);
    }
    
    cJSON_Delete(root);
    pthread_exit(NULL);
}

int main() {
    int opcion;
    Nodo *listaClientes = NULL; // Lista para almacenar los clientes del JSON

    do {
        printf("\n++++++++++++++++++++++++++++++++++++++++\n");
        printf("\nMenú:\n");
        printf("\n1. Carga masiva de usuarios\n");
        printf("2. Generar reporte Estado de cuentas\n");
        printf("3. Operaciones\n");
        printf("4. Carga masiva de operaciones\n");
        printf("5. Salir\n");
        printf("\n+++++++++++++++++++++++++++++++++++++++++\n");
        printf("Selecciona una opción: ");
        scanf("%d", &opcion);
        getchar();

        switch(opcion) {
            case 1: {
                char ruta[100];
                printf("Introduce la ruta del archivo JSON: ");
                fgets(ruta, sizeof(ruta), stdin);
                ruta[strcspn(ruta, "\n")] = 0;

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
                printf("--------------------------------\n");
                printf("Archivo Cargado Correctamente\n");
                printf("--------------------------------\n");
                break;
            }
            case 2:
                imprimirLista(listaClientes);
                break;
            case 3:
                int opcionOperaciones;
                int no_cuenta;
                int no_cuenta2;
                double monto;

                do {
                    printf("*************************************\n");
                    printf("\nOperaciones:\n");
                    printf("\n1. Depósito\n");
                    printf("2. Retiro\n");
                    printf("3. Transferencia\n");
                    printf("4. Consultar cuenta\n");
                    printf("5. Regresar al menú principal\n");
                    printf("\n*************************************\n");
                    printf("Selecciona una opción: ");
                    scanf("%d", &opcionOperaciones);

                    switch (opcionOperaciones) {
                        case 1:
                            printf("Seleccionaste Depósito.\n");
                           
                            printf("Introduce el número de cuenta: ");
                            scanf("%d", &no_cuenta);
                            printf("Introduce el monto a depositar: ");
                            scanf("%lf", &monto);
                            realizarDeposito(listaClientes, no_cuenta, monto);
                            break;
                        case 2:
                            printf("Seleccionaste Retiro.\n");
                            

                            printf("Introduce el número de cuenta: ");
                            scanf("%d", &no_cuenta);
                            printf("Introduce el monto a retirar: ");
                            scanf("%lf", &monto);
                            realizarRetiro(listaClientes, no_cuenta, monto);
                            break;
                        case 3:
                            printf("Seleccionaste Transferencia\n");
                            
                            printf("Introduce el número de cuenta donde se retirará: ");
                            scanf("%d", &no_cuenta);
                            printf("Introduce el número de cuenta donde se depositará: ");
                            scanf("%d", &no_cuenta2);
                            printf("Introduce el monto a transferir: ");
                            scanf("%lf", &monto);
                            transferencia(listaClientes, no_cuenta, no_cuenta2,monto);
                            //realizarRetiro(listaClientes, no_cuenta, monto);
                            //realizarDeposito(listaClientes, no_cuenta2, monto);

                            break;
                        case 4:
                        printf("----------------------------------\n");
                           printf("Seleccionaste Consultar Cuenta\n");
                           printf("Introduce el numero de cuenta: ");
                            scanf("%d", &no_cuenta);
                            consultar_cuenta(listaClientes, no_cuenta);

                            
                            break;
                        case 5:
                            printf("Regresando al menú principal.\n");
                            break;
                        default:
                            printf("Opción no válida. Por favor, selecciona una opción válida.\n");
                            break;
                    }
                } while (opcionOperaciones != 5);
                break;
            case 4:
                pthread_t threads[4];
                ThreadArgs args[4];
                pthread_mutex_init(&lock, NULL);
                pthread_mutex_init(&report_lock, NULL);
                char filename[100];
                
                printf("Ingrese la ubicacion del archivo JSON: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = 0; // Eliminar el salto de línea
                
                
                for (int i = 0; i < 4; ++i) {
                    args[i].lista = &listaClientes;
                    strcpy(args[i].ruta, filename);
                    pthread_create(&threads[i], NULL, thread_function, &args[i]);
                }
                
                for (int i = 0; i < 4; ++i) {
                    pthread_join(threads[i], NULL);
                }
                
                pthread_mutex_destroy(&lock);
                write_report();
                break;    
            case 5:
                printf("Saliendo del programa.\n");
                break;  
            default:
                printf("Opción no válida. Por favor, selecciona una opción válida.\n");
        }
    } while(opcion != 5);

    // Liberamos la memoria de la lista antes de salir
    while (listaClientes != NULL) {
        Nodo *temp = listaClientes;
        listaClientes = listaClientes->siguiente;
        free(temp);
    }

    return 0;
}
