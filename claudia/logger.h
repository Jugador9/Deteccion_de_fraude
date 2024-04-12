#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

// Función para inicializar el log
void inicializar_logger(const char *nombreFichero);

// Función para imprimir en el registro y en la pantalla
void log_mensaje(const char *formato, ...);

// Función para cerrar el registro
void cerrar_logger();

#endif