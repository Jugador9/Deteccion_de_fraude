#include "logger.h"
#include <stdlib.h>
#include <stdarg.h>

FILE *log_file = NULL;

void inicializar_logger(const char *nombreFichero)
{
  log_file = fopen(nombreFichero, "a");
  if (log_file == NULL)
  {
    perror("Error abriendo fichero de log");
    exit(EXIT_FAILURE);
  }
}

void log_mensaje(const char *formato, ...)
{
  if (log_file != NULL)
  {
    va_list args;
    va_start(args, formato);
    vprintf(formato, args); // imprimir por pantalla
    va_start(args, formato);
    vfprintf(log_file, formato, args); // imprimir por pantalla
    fflush(log_file); // fflush para asegurarnos que se escriben de inmediato
    va_end(args);
  }
}

void cerrar_logger()
{
  if (log_file != NULL)
  {
    fclose(log_file);
    log_file = NULL;
  }
}
