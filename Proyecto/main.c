#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdbool.h>


#define MAX_BUF_SIZE 1024

int main() {
    FILE *fp;
    char output[MAX_BUF_SIZE];

    char pid[20];       
    char nombre[100];   
    char llamada[50];   
    char size[20];      
    char tiempo[50]; 

    MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;

    char* server = "0.0.0.0";
	char* user = "joseph";
	char* password = "1234";
	char* database = "pruebaso2";
	
	conn = mysql_init(NULL);

    /* Conexión a la base de datos */
	if (!mysql_real_connect(conn, server, user, password, 
                                      database, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}	

    // Ejecutar el script de SystemTap y capturar su salida
    fp = popen("/home/joseph/Documents/Proyecto/trace.stp", "r");
    if (fp == NULL) {
        perror("popen failed");
        return 1;
    }

    // Leer la salida del SystemTap
    while (fgets(output, sizeof(output), fp) != NULL) {
        //printf(output);
        sscanf(output, "PID:%s NOMBRE:%s LLAMADA:%s SIZE:%s TIEMPO:%[^\n]",pid, nombre, llamada, size, tiempo);
        //printf("PID: %s\n", pid);
        //printf("NOMBRE: %s\n", nombre);
        //printf("LLAMADA: %s\n", llamada);
        //printf("SIZE: %s\n", size);
        //printf("TIEMPO: %s\n", tiempo);
        char sql_query[500];
        sprintf(sql_query, "INSERT INTO datos VALUES(%s, '%s', '%s', %s, '%s')",pid, nombre, llamada, size, tiempo);
        if (mysql_query(conn, sql_query)) {
            mysql_close(conn);
            exit(1);
        }
    }

    // Cerrar el descriptor de archivo
    pclose(fp);

    return 0;
}
