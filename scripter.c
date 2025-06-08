#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "process_line.h"


int main(int argc, char *argv[]) {    

    /*CONTROL DE ERROR DE PARÁMETROS DE ENTRADA*/
    if (argc != 2) { //Comprueba que esté bien el formato ./scripter <fichero_de_comandos>
        perror("Error formato de uso: ./scripter <fichero_de_comandos>\n");
        return -1;
    }
  
	int fd = open(argv[1], O_RDONLY, 0644);//Abro el fichero

	/*CONTROL DE ERRORES DE LA APERTURA DEL FICHERO*/
	if (fd < 0){
        perror("Error al abrir el fichero\n");		
        if (errno == EACCES){ //Compruebo si tengo permisos para abrir el fichero de entrada
            perror("Error permisos de lectura\n");
        }		
		if(errno == ENOENT){ //Compruebo que existe el fichero
			perror("El fichero que intentaste abrir no existe\n");
		}
        return -1;
    }

    /* LECTURA DEL FICHERO DE COMANDOS */

	int tam_buffer = 0;
    char *buffer = (char*)malloc(max_line*max_numlines); //Reservo memoria para el buffer
    if (buffer == NULL) {
        perror("Error al reservar memoria\n");
        return -1;
    }
    //Leemos todo el fichero y lo guardamos en el buffer
    if ((tam_buffer = read(fd, buffer, max_line*max_numlines)) < 0){
        perror("Error al leer el fichero\n");
        if(close(fd) == -1){
            perror("Error al cerrar el fichero\n");
        }
        free(buffer);
        return -1;
    }

    char line[max_line];
    int num_lines = 0;

    //Leo el título y lo guardo en line[]
    for (int i = 0; i < 18; i++){line[i] = buffer[i];}
    line[18] = '\0';   
    if(strcmp(line, "## Script de SSOO\n") != 0){
        //Título mal escrito, incluye el caso donde el fichero solo contiene el título "## Script de SSOO\0"
        //Si el fichero solo contiene el título pero está bien escrito acabando en \n sí lo ejecuta bien        
        perror("El fichero no empieza con ## Script de SSOO\n");
        if(close(fd) == -1){
            perror("Error al cerrar el fichero\n");
        }
        free(buffer);
        return -1;
    }
    num_lines++; //Contabilizo el título bien escrito

    int totalreads = 18; //Contador de caracteres totales leídos (incluyo los 18 caracteres del título)
    int nreads = 0; //Contador de caracteres leídos en la línea
    char *inicio = &buffer[18]; //Puntero que indica el inicio de la línea a leer
    char *final = NULL; //Puntero que me indica el final de la línea
    line[0] = '\0'; //Reinicializo la línea
    
    //Proceso cada línea buscando el primer símbolo '\n'
    while ((final = strchr(inicio, '\n')) != NULL) {
        nreads = final - inicio;
        //Recorro desde &inicio hasta &final para ir guardando los caracteres de la línea en line[]
        for (int i = 0; i < nreads; i++){
            line[i] = buffer[totalreads + i];
        }
        //Línea vacía intermedia, control de error
        if(nreads == 1 && line[0] == '\n'){
            perror("El fichero no puede contener líneas vacías\n");
            if(close(fd) == -1){
                perror("Error al cerrar el fichero\n");
            }
            free(buffer);
            return -1;
        }

        line[nreads] = '\0'; //Añadimos el símbolo '\0' al final de la línea
        num_lines++;

        //Procesamos la línea
        int n_commands = procesar_linea(line, buffer);
        if(n_commands == -1){
            //Si procesar linea devuelve -1 es porque ha fallado alguna llamada al sistema, paro el scripter
            perror("Scripter ha fallado por alguna llamada al sistema\n");
            free(buffer);
            return -1;
        }
        //Recojo a todos los hijos que estaban en background para no tener hijos zombies después de ejecutar una línea en foreground
        if (!background){
            int status;
            pid_t pid_recog;
            while ((pid_recog = waitpid(-1, &status, WNOHANG)) > 0) {
                if (WIFEXITED(status) && WEXITSTATUS(status)== 1){
                    //Si el hijo que recojo hizo exit(-1) es por fallo de llamada al sistema, finaliza el scripter
                    free(buffer);
                    return -1;
                }
                
                //Imprimo solo si es un pid de la lista de background
                int pid_encontrado = 0;
                for (int i = 0; i < num_bg_pids; i++) {
                    if (bg_pids[i] == pid_recog) {
                        printf("%d", pid_recog);
                        pid_encontrado = 1;  //Indicamos que ya hemos encontrado el pid
                    }
                    
                    //Si ya hemos encontrado el pid, desplazamos los elementos una posicion a la izquierda
                    if (pid_encontrado && i < num_bg_pids - 1) {
                        bg_pids[i] = bg_pids[i + 1];
                    }
                }

                //Si encontramos el pid, reducimos el número de procesos en background
                if (pid_encontrado) {
                    num_bg_pids--;
                }
            }
        }
        //Reinicializamos las variables globales
        argvv[max_args] = NULL;
        filev[max_redirections] = NULL;
        background = 0;

        //Movemos los punteros y contabilizamos los caracteres leidos
        totalreads += nreads;
        totalreads++;
        inicio = final + 1; //Avanzamos al inicio de la siguiente línea en el buffer
    }
  
    //Sale del bucle cuando no encuentra más saltos de línea, se topa el símbolo '\0'
    //Damos como válido cuando acaba con \n, que tenga líneas vacías al final del fichero
    if (*inicio != '\0') {
        
        final = &buffer[strlen(buffer)];
        nreads = final - inicio;
        for (int i = 0; i < nreads; i++){
            line[i] = buffer[totalreads + i];
        }
        
        //Procesamos la última línea
        if (nreads > 0){
            line[nreads] = '\0'; //Añadimos el símbolo '\0' al final de la línea
            num_lines++;
            if(procesar_linea(line, buffer) == -1){
                //Si procesar linea devuelve -1 es porque ha fallado alguna llamada al sistema, paro el scripter
                perror("Scripter ha fallado por alguna llamada al sistema\n");
                free(buffer);
                return -1;
            }
        }
    }

    /*CONTROL DE HIJOS EN BACKGROUND*/
    int status;
    pid_t pid_recog;
    //Recojo a todos los hijos que estaban en background para no tener hijos zombies
    while ((pid_recog = waitpid(-1, &status, 0)) > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status)== 1){
            //Si el hijo que recojo hizo exit(-1) es por fallo de llamada al sistema, finaliza el scripter
            free(buffer);
            return -1;
        }
        
        //Imprimo solo si es un pid de la lista de background
        int pid_encontrado = 0;
        for (int i = 0; i < num_bg_pids; i++) {
            if (bg_pids[i] == pid_recog) {
                printf("Pid recogido: %d\n", pid_recog);
                pid_encontrado = 1;  //Indicamos que ya hemos encontrado el pid
            }
    
            //Si ya hemos encontrado el pid, desplazamos los elementos una posicion a la izquierda
            if (pid_encontrado && i < num_bg_pids - 1) {
                bg_pids[i] = bg_pids[i + 1];
            }
        }

        //Si encontramos el pid, reducimos el número de procesos en background
        if (pid_encontrado) {
            num_bg_pids--;
        }
    }
    
    free(buffer);
    return 0;
}
