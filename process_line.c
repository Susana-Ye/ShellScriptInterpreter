#include "process_line.h"

/* VARS TO BE USED */
char *argvv[max_args];
char *filev[max_redirections];
int background = 0;
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