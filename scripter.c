#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

/* CONST VARS */
const int max_line = 1024; //Máximo de caracteres en una línea
const int max_commands = 10;
const int max_numlines = 100; //Máximo número de líneas en el fichero
#define max_redirections 3 //stdin, stdout, stderr
#define max_args 15

/* VARS TO BE USED FOR THE STUDENTS */
char * argvv[max_args];
char * filev[max_redirections];
int background = 0; 

//Creo una lista de pids para guardar los pids de los hijos en background que el padre debe imprimir
#define max_pids 100
int bg_pids[max_pids];
int num_bg_pids = 0;

/**
 * This function splits a char* line into different tokens based on a given character
 * @return Number of tokens
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {
    int i = 0;
    char *token = strtok(linea, delim);
    while (token != NULL && i < max_tokens - 1) {
        //RESOLVEMOS EL PROBLEMA DE COMILLAS
        int final = strlen(token) - 1;
        if (token[0] == '\"' && token[final] == '\"'){
            token[final] = '\0';
            token = &token[1];
        }
        //HACEMOS QUE SE EJECUTE ./mygrep SI SOLO PONEN mygrep
        if (strcmp(token, "mygrep") == 0){
            char * mygrep = "./mygrep";
            token = mygrep;
        }
        tokens[i++] = token;
        token = strtok(NULL, delim);
    }
    tokens[i] = NULL;
    return i;
}

/**
 * This function processes the command line to evaluate if there are redirections. 
 * If any redirection is detected, the destination file is indicated in filev[i] array.
 * filev[0] for STDIN
 * filev[1] for STDOUT
 * filev[2] for STDERR
 */
void procesar_redirecciones(char *args[]) {
    //initialization for every command
    filev[0] = NULL;
    filev[1] = NULL;
    filev[2] = NULL;
    //Store the pointer to the filename if needed.
    //args[i] set to NULL once redirection is processed
    int i = 0;
    while(args[i] != NULL){
        if (strcmp(args[i], "<") == 0) {
            filev[0] = args[i+1];
            args[i] = NULL;
            args[i + 1] = NULL;
            i+=2;
        } else if (strcmp(args[i], ">") == 0) {
            filev[1] = args[i+1];
            args[i] = NULL;
            args[i + 1] = NULL;
            i+=2;
        } else if (strcmp(args[i], "!>") == 0) {
            filev[2] = args[i+1];
            args[i] = NULL; 
            args[i + 1] = NULL;
            i+=2;
        } else{
            i++;
        }
    }
}

/**
 * This function processes the input command line and returns in global variables: 
 * argvv -- command an args as argv 
 * filev -- files for redirections. NULL value means no redirection. 
 * background -- 0 means foreground; 1 background.
 */
int procesar_linea(char *linea, char* buffer) {

    //Separamos la línea en los comandos que lo conforman cuando hay pipes
    char *comandos[max_commands];
    int num_comandos = tokenizar_linea(linea, "|", comandos, max_commands);
 
    //Creamos una lista de pipes
    int pipes[max_commands - 1][2];
    if (num_comandos > 1){
        //Creo num_comandos-1 pipes
        for(int i = 0; i < num_comandos -1; i++){
            if(pipe(pipes[i]) == -1){
                perror("Error en pipe\n");
                free(buffer);
                exit(-1);
            }
        }
    }

    //Finish processing
    for (int i = 0; i < num_comandos; i++) {
              
        //Comprobamos si background está indicado y lo quitamos si lo está
        if (strchr(comandos[num_comandos - 1], '&')) {
            background = 1;
            char *pos = strchr(comandos[num_comandos - 1], '&'); 
            *pos = '\0'; //remove character 
        }
        
        //Tokeniza los argumentos del comando insertándolo en argvv
        int args_count = tokenizar_linea(comandos[i], " \t\n", argvv, max_args);

        //Procesamos las redirecciones guardando los ficheros de redireccion en filev y quitandolos de la linea        
        procesar_redirecciones(argvv);

        //Procesamos el comando ya correctamente segmentado en argvv
        int pid_line = fork();
       
        if (pid_line == -1){ //Error del fork
            perror("Fork failed\n");
            return -1;
        }
        else if (pid_line == 0){ //Hijo
            
            /*EJECUCIÓN PIPES para caso genérico n*/
            if(num_comandos > 1){
                
                if(i > 0){ //Redirección del pipe de lectura para todos los comandos menos el primero
                    if(filev[0] != NULL){
                        //Error, no puedo hacer redirección de lectura y leer del pipe a la vez
                        perror("Error redirección lectura y pipe lectura simultáneo\n");
                        free(buffer);
                        exit(1);
                    }
                    if(close(STDIN_FILENO) == -1){
                        perror("Error al cerrar el descriptor de fichero stdin\n");
                        free(buffer);
                        exit(-1);
                    }
                    if(dup(pipes[i-1][0]) == -1){
                        perror("Error al duplicar el pipe\n");
                        free(buffer);
                        exit(-1);
                    }
                }
                if( i < num_comandos - 1){ //Redirección del pipe de escritura para todos los comandos menos el último
                    if(filev[1] != NULL){
                        //Error, no puedo hacer redirección de salida y escribir en el pipe a la vez
                        perror("Error redirección salida y pipe escritura simultáneo\n");
                        free(buffer);
                        exit(1);
                    }
                    if(close(STDOUT_FILENO) == -1){
                        perror("Error al cerrar el descriptor de fichero stdout\n");
                        free(buffer);
                        exit(-1);
                    }
                    if(dup(pipes[i][1]) == -1){
                        perror("Error al duplicar el pipe\n");
                        free(buffer);
                        exit(-1);
                    }
                }
                
                //Cerramos todos los descriptores de ficheros de los pipes
                for(int j = 0; j < num_comandos - 1; j++){ 
                    if(close(pipes[j][0]) == -1){
                        perror("Error al cerrar el descriptor de pipes[j][0]\n");
                        free(buffer);
                        exit(-1);
                    }
                    if(close(pipes[j][1]) == -1){
                        perror("Error al cerrar el descriptor de pipes[j][1]\n");
                        free(buffer);
                        exit(-1);
                    }  
                }
            }
           
            /*EJECUCIÓN REDIRECCIONES*/
            if (filev[0] != NULL || filev[1] != NULL || filev[2] != NULL){

                if (filev[0] != NULL){ //Redirección de entrada
                    if (i > 0){
                        //Error, no puedo hacer redirección de lectura y leer del pipe a la vez
                        perror("Error redirección lectura y pipe lectura simultáneo\n");
                        free(buffer);
                        exit(1);
                    }
                    int fd0 = open(filev[0], O_RDONLY);
                    if (fd0 < 0){
                        perror("Error al abrir el fichero de redirección\n");
                        if (errno == EACCES){ //Compruebo si tengo permisos para abrir el fichero de entrada
                            perror("Error permisos de lectura\n");
                        }
                        if(errno == ENOENT){//Compruebo que existe el fichero
                            perror("El fichero de redirección de lectura no existe\n");
                        }
                        free(buffer);
                        exit(-1);
                    }

                    if(close(0) == -1){
                        perror("Error al cerrar el descriptor de stdin\n");
                        free(buffer);
                        exit(-1);
                    }
                    if(dup(fd0) == -1){
                        perror("Error al duplicar el fichero\n");
                        free(buffer);
                        exit(-1);
                    }
                    if(close(fd0) == -1){
                        perror("Error al cerrar el fichero\n");
                        free(buffer);
                        exit(-1);
                    }
                }

                if(filev[1] != NULL){ //Redirección de salida                   
                    if (i < num_comandos - 1){
                        //Error, no puedo hacer redirección de escritura y escribir en el pipe a la vez
                        perror("Error redirección escritura y pipe escritura simultáneo\n");
                        free(buffer);
                        exit(1);
                    }
                    //Abrimos el fichero en el que queremos escribir
                    int fd1 = open(filev[1], O_CREAT | O_WRONLY | O_TRUNC, 0644); //Si ya existe el fichero, lo sobreescribimos, sino, lo creamos
                    if (fd1 < 0){
                        perror("Error al crear el fichero de redirección de salida\n");                        
                        if (errno == ENAMETOOLONG){//Comprobamos que el nombre del fichero no sea demasiado largo
                            perror("El nombre del fichero de salida no puede superar 255 caracteres\n");
                        }
                        free(buffer);
                        exit(-1);
                    }                
                    if(close(1) == -1){
                        perror("Error al cerrar el descriptor de stdout\n");
                        free(buffer);
                        exit(-1);
                    }
                    if(dup(fd1) == -1){
                        perror("Error al duplicar el fichero\n");
                        free(buffer);
                        exit(-1);
                    }
                    if(close(fd1) == -1){
                        perror("Error al cerrar el fichero\n");
                        free(buffer);
                        exit(-1);
                    }
                }

                if(filev[2] != NULL){ //Redirección de error                    
                    //Abrimos el fichero en el que queremos escribir
                    int fd2 = open(filev[2], O_CREAT | O_WRONLY | O_TRUNC, 0644); //Si ya existe el fichero, lo sobreescribimos, sino, lo creamos                    
                    if (fd2 < 0){
                        perror("Error al crear el fichero de redirección de errores\n");                        
                        if (errno == ENAMETOOLONG){ //Comprobamos que el nombre del fichero no sea demasiado largo
                            perror("El nombre del fichero de errores no puede superar 255 caracteres\n");
                        }
                        free(buffer);
                        exit(-1);
                    }

                    if(close(2) == -1){
                        perror("Error al cerrar el descriptor de stderr\n");
                        free(buffer);
                        exit(-1);
                    }
                    if(dup(fd2) == -1){
                        perror("Error al duplicar el fichero\n");
                        free(buffer);
                        exit(-1);
                    }
                    if(close(fd2) == -1){
                        perror("Error al cerrar el fichero\n");
                        free(buffer);
                        exit(-1);
                    }
                }
            }
            
            /*EJECUCIÓN SIMPLE DEL COMANDO*/
            execvp(argvv[0], argvv);
            perror("Error exec\n"); //Error en el exec
            free(buffer);
            exit(-1);
        }
        else{//Padre
            //Guardo el pid del último proceso de una línea en background, incluye el caso de ser un comando simple
            if (background && i == num_comandos - 1) {
                if (num_bg_pids < max_pids) {
                    bg_pids[num_bg_pids++] = pid_line;
                }
            }
            
        }
    }   
    
    if (num_comandos > 1){ //Cerramos los descriptores de ficheros de los pipes
        for(int j = 0; j < num_comandos -1; j++){ 
            if(close(pipes[j][0]) == -1){
                perror("Error al cerrar el descriptor de pipes[j][0]\n");
                return -1;
            }
            if(close(pipes[j][1]) == -1){
                perror("Error al cerrar el descriptor de pipes[j][1]\n");
                return -1;
            }
        }
    }

    if (!background) {//Si no es background el padre espera al hijo y lo recoge

        //CONTROL CUANDO RECOGE AL HIJO POR UN ERROR:
        //-1 es por error del exec, fork, dup... --> parar el scripter
        //1 es por error del comando --> falla el comando pero el scripter sigue
        int status = 0;
        wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 255){
            //Si el hijo que recojo hizo exit(-1) es por fallo de llamada al sistema, finaliza el scripter
            perror("Algún hijo ha salido por fallo de llamada al sistema\n");
            return -1;
        }
    }

    return num_comandos;
}

int main(int argc, char *argv[]) {     
    /*CONTROL DE ERROR PARÁMETROS DE ENTRADA*/
    //Comprueba que esté bien el formato ./scripter <fichero_de_comandos>
    if (argc != 2) {
        perror("Error formato de uso: ./scripter <fichero_de_comandos>\n");
        return -1;
    }
  
	int fd = open(argv[1], O_RDONLY, 0644);//Abro el fichero
	//CONTROL DE ERRORES DE LA APERTURA DEL FICHERO
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

    //Reservo memoria para el buffer
	int tam_buffer = 0;
    char *buffer = (char*)malloc(max_line*max_numlines);
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
        inicio = final + 1;  //Avanzamos al inicio de la siguiente línea en el buffer
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
    
    free(buffer);
    return 0;
}
