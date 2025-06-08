#ifndef PROCESS_LINE_H
#define PROCESS_LINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

/* CONST VARS */
#define max_line 1024 //Máximo de caracteres en una línea
#define max_commands 10
#define max_numlines 100 //Máximo número de líneas en el fichero
#define max_redirections 3 // stdin, stdout, stderr
#define max_args 15
#define max_pids 100

/* VARS TO BE USED */
extern char *argvv[max_args];
extern char *filev[max_redirections];
extern int background;
extern int bg_pids[max_pids];
extern int num_bg_pids;

int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens);
void procesar_redirecciones(char *args[]);
int procesar_linea(char *linea, char* buffer);


#endif