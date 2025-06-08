#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ruta_fichero> <cadena_busqueda>\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_RDONLY, 0644);

	//CONTROL DE ERRORES DE LA APERTURA DEL FICHERO
	if (fd < 0){
        perror("Error al abrir el fichero\n");
		//Compruebo si tengo permisos para abrir el fichero de entrada
        if (errno == EACCES){
            perror("Error permisos de lectura\n");
        }
		//Compruebo que existe el fichero
		if(errno == ENOENT){
			perror("El fichero que intentaste abrir no existe\n");
		}
        return -1;
    }

    int max_tam = 1024;
    char buffer[max_tam];
    char linea[max_tam];
    int i_linea = 0; //Me indica la posición dentro de la línea
    int nreads = 0; //Me indica el número de caracteres leídos de la línea
    int encontrado = 0; //Variable que me indica si he encontrado la palabra alguna vez

    //Leo todo el fichero y lo guardo en el buffer
    while ((nreads = read(fd, buffer, max_tam)) > 0) {

        //Recorro caracter a caracter el buffer para encontrar las líneas e imprimirlas si está la palabra buscada
        for (int i = 0; i < nreads; i++) {
            
            //Si encuentro el salto de línea
            if (buffer[i] == '\n') {
                linea[i_linea] = '\0'; //Pongo el último caracter a /0 para marcar el final de línea
                
                if (strstr(linea, argv[2]) != NULL) {
                    encontrado = 1;

                    /*IMPRIMIMOS LA LÍNEA CON LA PALABRA DE COLOR DE ROSA*/
                    char *p = linea; //puntero que me indica el caracter que imprimo
                    char *palabra; //puntero que apunta a la primera letra de la palabra que busco
                    while ((palabra = strstr(p, argv[2])) != NULL) {

                        //Imprimo letra a letra desde p hasta palabra (sin incluirla)
                        char *aux = p;
                        while (aux < palabra) {
                            printf("%c", *aux);
                            aux++;
                        }
                
                        // Imprimo la palabra en color rosa
                        printf("\033[1;35m"); //Cambio al color rosa
                        for (int j = 0; j < strlen(argv[2]); j++) {
                            printf("%c", palabra[j]);
                        }
                        printf("\033[0m"); //Restauro al color original
                
                        //Muevo el puntero hasta después de la palabra encontrada por si tengo que imprimir letras entre medias de palabras
                        p = palabra + strlen(argv[2]);
                    }
                
                    //Imprimo lo que queda por imprimir, desde la última palabra hasta el final de linea
                    while (*p != '\0') {
                        printf("%c", *p);
                        p++;
                    }
                
                    printf("\n");
                }

                //Reinicializo la línea
                i_linea = 0;
            } 

            //Si no he llegado al final de la línea (\n) y tampoco al final del buffer, guardar el caracter 
            else if (i_linea < max_tam - 1) {
                
                linea[i_linea] = buffer[i];
                i_linea++;
            }
        }
    }

    //Si el caracter es \0, he terminado de leer el archivo, busco mi palabra en la última línea
    if (i_linea > 0) {
        linea[i_linea] = '\0';
        if (strstr(linea, argv[2]) != NULL) {
            encontrado = 1;

            /*IMPRIMIMOS LA LÍNEA CON LA PALABRA DE COLOR DE ROSA*/
            char *p = linea; //puntero que me indica el caracter que imprimo
            char *palabra; //puntero que apunta a la primera letra de la palabra que busco
            while ((palabra = strstr(p, argv[2])) != NULL) {

                //Imprimo letra a letra desde p hasta palabra (sin incluirla)
                char *aux = p;
                while (aux < palabra) {
                    printf("%c", *aux);
                    aux++;
                }
        
                // Imprimo la palabra en color rosa
                printf("\033[1;35m"); //Cambio al color rosa
                for (int j = 0; j < strlen(argv[2]); j++) {
                    printf("%c", palabra[j]);
                }
                printf("\033[0m"); //Restauro al color original
        
                //Muevo el puntero hasta después de la palabra encontrada por si tengo que imprimir letras entre medias de palabras
                p = palabra + strlen(argv[2]);
            }
        
            //Imprimo lo que queda por imprimir, desde la última palabra hasta el final de linea
            while (*p != '\0') {
                printf("%c", *p);
                p++;
            }
        
            printf("\n");
        }
    }

    if (nreads == -1) {
        perror("Error al leer el fichero.\n");
        return -1;
    }

    //Si no se ha encontrado la palabra ninguna vez, imprimo not found
    if (!encontrado) {
        printf("%s not found.\n", argv[2]);
    }

    
    if(close(fd) == -1){
        perror("Error al cerrar el fichero\n");
        return -1;
    }

    return 0;
}