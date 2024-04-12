#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>
#include "logger.h"

#define LONGITUD_MAX_CLAVE_CONFIG 50
#define LONGITUD_MAX_VALOR_CONFIG 4096
#define LONGITUD_MAX_POR_LINEA_CONFIG 4147
#define LONGITUD_MAX_POR_LINEA 1000

// Variables globales configuracion
char PATH_FILES[4096];
char INVENTORY_FILE[256];
char LOG_FILE[256];
int NUM_PROCESOS;
int SIMULATE_SLEEP_MIN;
int SIMULATE_SLEEP_MAX;

typedef struct
{ // Struct para guardar cada registro de los ficheros data
  char idOperacion[7];
  char fechaInicio[17];
  char fechaFin[17];
  char idUsuario[8];
  char idTipoOperacion[15];
  int numOperacion;
  double importe;
  char estado[30];
} Transaccion;

typedef struct
{
  int numSucursal;
  int segundosRetardo;
} ParametrosHiloSucursal;

// Función para bloquear un fichero a través de su descriptor
void lock(FILE *file)
{
  int fd = fileno(file);
  flock(fd, LOCK_EX);
}

// Función para desbloquear un fichero a través de su descriptor
void unlock(FILE *file)
{
  int fd = fileno(file);
  flock(fd, LOCK_UN);
}

void leerFicheroConfiguracion(const char *nombreFichero)
{
  FILE *fichero = fopen(nombreFichero, "r");
  if (fichero == NULL)
  {
    log_mensaje("No se pudo abrir el archivo de configuracion\n");
    return;
  }

  char linea[LONGITUD_MAX_POR_LINEA_CONFIG];
  while (fgets(linea, sizeof(linea), fichero))
  {
    char *token = strtok(linea, "="); // Divide la línea en dos partes usando el signo '='
    if (token != NULL)
    {
      char key[LONGITUD_MAX_CLAVE_CONFIG];
      strcpy(key, token);        // La primera parte es la clave
      token = strtok(NULL, "="); // La segunda parte es el valor
      if (token != NULL)
      {
        if (strcmp("PATH_FILES", key) == 0)
        {
          strcpy(PATH_FILES, token);
          // Eliminar el salto de línea al final del valor si existe
          if (PATH_FILES[strlen(PATH_FILES) - 1] == '\n')
          {
            PATH_FILES[strlen(PATH_FILES) - 1] = '\0';
          }
        }
        else if (strcmp("INVENTORY_FILE", key) == 0)
        {
          strcpy(INVENTORY_FILE, token);
          // Eliminar el salto de línea al final del valor si existe
          if (INVENTORY_FILE[strlen(INVENTORY_FILE) - 1] == '\n')
          {
            INVENTORY_FILE[strlen(INVENTORY_FILE) - 1] = '\0';
          }
        }
        else if (strcmp("LOG_FILE", key) == 0)
        {
          strcpy(LOG_FILE, token);
          // Eliminar el salto de línea al final del valor si existe
          if (LOG_FILE[strlen(LOG_FILE) - 1] == '\n')
          {
            LOG_FILE[strlen(LOG_FILE) - 1] = '\0';
          }
        }
        else if (strcmp("NUM_PROCESOS", key) == 0)
        {
          char numProcesos[3];
          strcpy(numProcesos, token);
          // Eliminar el salto de línea al final del valor si existe
          if (numProcesos[strlen(numProcesos) - 1] == '\n')
          {
            numProcesos[strlen(numProcesos) - 1] = '\0';
          }

          NUM_PROCESOS = atoi(numProcesos);
        }
        else if (strcmp("SIMULATE_SLEEP_MIN", key) == 0)
        {
          char simulateSleepMin[3];
          strcpy(simulateSleepMin, token);
          // Eliminar el salto de línea al final del valor si existe
          if (simulateSleepMin[strlen(simulateSleepMin) - 1] == '\n')
          {
            simulateSleepMin[strlen(simulateSleepMin) - 1] = '\0';
          }

          SIMULATE_SLEEP_MIN = atoi(simulateSleepMin);
        }
        else if (strcmp("SIMULATE_SLEEP_MAX", key) == 0)
        {
          char simulateSleepMax[3];
          strcpy(simulateSleepMax, token);
          // Eliminar el salto de línea al final del valor si existe
          if (simulateSleepMax[strlen(simulateSleepMax) - 1] == '\n')
          {
            simulateSleepMax[strlen(simulateSleepMax) - 1] = '\0';
          }

          SIMULATE_SLEEP_MAX = atoi(simulateSleepMax);
        }
      }
    }
  }

  fclose(fichero);
}

void mostrarConfiguracion()
{
  log_mensaje("PATH_FILES:%s\n", PATH_FILES);
  log_mensaje("INVENTORY_FILE:%s\n", INVENTORY_FILE);
  log_mensaje("LOG_FILE:%s\n", LOG_FILE);
  log_mensaje("NUM_PROCESOS:%d\n", NUM_PROCESOS);
  log_mensaje("SIMULATE_SLEEP_MIN:%d\n", SIMULATE_SLEEP_MIN);
  log_mensaje("SIMULATE_SLEEP_MAX:%d\n", SIMULATE_SLEEP_MAX);
}

int numeroAleatorio(int min, int max)
{
  return min + rand() % (max - min + 1);
}

int compararNombres(const void *a, const void *b)
{
  return strcmp(*(const char **)a, *(const char **)b);
}

char **listarArchivosPorSucursal(const char *directorio, int numSucursal)
{
  DIR *dir;
  struct dirent *ent;
  char **archivos = NULL;
  int num_archivos = 0;

  // Abrir el directorio
  dir = opendir(directorio);
  if (dir == NULL)
  {
    log_mensaje("Error al abrir el directorio\n");
    exit(EXIT_FAILURE);
  }

  // Recorrer los archivos del directorio
  while ((ent = readdir(dir)) != NULL)
  {
    // Verificar si el archivo pertenece a la sucursal deseada
    // prefijo SU001
    char bufferPrefijoSucursal[6];
    if (numSucursal < 10)
    {
      snprintf(bufferPrefijoSucursal, sizeof(bufferPrefijoSucursal), "SU00%d", numSucursal);
    }
    else if (numSucursal < 100)
    {
      snprintf(bufferPrefijoSucursal, sizeof(bufferPrefijoSucursal), "SU0%d", numSucursal);
    }
    else
    {
      snprintf(bufferPrefijoSucursal, sizeof(bufferPrefijoSucursal), "SU%d", numSucursal);
    }

    // Si es un fichero y empieza por el prefijo de la sucursal...
    if (ent->d_type == DT_REG &&
        strncmp(ent->d_name, bufferPrefijoSucursal, strlen(bufferPrefijoSucursal)) == 0)
    {
      // Guardar el nombre del archivo en la lista
      archivos = realloc(archivos, (num_archivos + 1) * sizeof(char *));
      archivos[num_archivos] = strdup(ent->d_name);
      log_mensaje("**** Fichero recibido %s ****\n", archivos[num_archivos]);
      num_archivos++;
    }
  }

  closedir(dir);

  qsort(archivos, num_archivos, sizeof(char *), compararNombres);
  // Terminar la lista con un puntero nulo
  archivos = realloc(archivos, (num_archivos + 1) * sizeof(char *));
  archivos[num_archivos] = NULL;

  return archivos;
}

void procesarArchivoData(const char *nombreFichero)
{
  FILE *fichero = fopen(nombreFichero, "r");
  if (fichero == NULL)
  {
    log_mensaje("No se pudo abrir el archivo %s\n", nombreFichero);
    return;
  }

  FILE *ficheroConsolidado = fopen(INVENTORY_FILE, "a");
  if (ficheroConsolidado == NULL)
  {
    log_mensaje("No se pudo abrir el archivo de consolidacion %s\n", INVENTORY_FILE);
    fclose(fichero); // cierro el fichero de entrada si no he podido abrir el fichero de salida...
    return;
  }

  lock(ficheroConsolidado);

  char linea[LONGITUD_MAX_POR_LINEA];
  while (fgets(linea, LONGITUD_MAX_POR_LINEA, fichero) != NULL)
  {
    Transaccion transaccion;

    strcpy(transaccion.idOperacion, strtok(linea, ";"));
    strcpy(transaccion.fechaInicio, strtok(NULL, ";"));
    strcpy(transaccion.fechaFin, strtok(NULL, ";"));
    strcpy(transaccion.idUsuario, strtok(NULL, ";"));
    strcpy(transaccion.idTipoOperacion, strtok(NULL, ";"));
    transaccion.numOperacion = atoi(strtok(NULL, ";"));
    transaccion.importe = atof(strtok(NULL, ";"));
    strcpy(transaccion.estado, strtok(NULL, ";"));
    // como el estado es el ultimo campo intento eliminar el salto de linea.
    if (transaccion.estado[strlen(transaccion.estado) - 1] == '\n')
    {
      transaccion.estado[strlen(transaccion.estado) - 1] = '\0';
    }

    // volcar al fichero de consolidacion
    fprintf(ficheroConsolidado, 
      "%s;%s;%s;%s;%s;%d;%f;%s\n",
      transaccion.idOperacion,
      transaccion.fechaInicio,
      transaccion.fechaFin,
      transaccion.idUsuario,
      transaccion.idTipoOperacion,
      transaccion.numOperacion,
      transaccion.importe,
      transaccion.estado
    );
  }

  fclose(fichero);

  unlock(ficheroConsolidado);
  fclose(ficheroConsolidado);
}

void procesarArchivos(char **archivos)
{
  if (archivos == NULL) // no hay ficheros para procesar
  {
    return;
  }

  // Iterar sobre la lista de archivos
  for (int i = 0; archivos[i] != NULL; i++)
  {
    log_mensaje("Procesando archivo: %s\n", archivos[i]);
    char ruta_archivo[PATH_MAX + 2];
    // Construimos la ruta completa del archivo
    snprintf(ruta_archivo, sizeof(ruta_archivo), "%s/%s", PATH_FILES, archivos[i]);
    procesarArchivoData(ruta_archivo);
    // Eliminamos el archivo
    if (remove(ruta_archivo) != 0)
    {
      log_mensaje("Error al borrar el fichero %s\n", archivos[i]);
    }
    else
    {
      log_mensaje("Fichero %s procesado correctamente\n", archivos[i]);
    }
  }
}

void *tratarPeticionSucursal(void *arg)
{
  ParametrosHiloSucursal *parametrosHiloSucursal = (ParametrosHiloSucursal *)arg;

  while (1)
  {
    // Realizar la tarea
    char **archivos = listarArchivosPorSucursal(PATH_FILES, parametrosHiloSucursal->numSucursal);
    procesarArchivos(archivos);
    // Liberar memoria asignada a la lista de archivos
    for (int i = 0; archivos[i] != NULL; i++)
    {
      free(archivos[i]);
    }
    free(archivos);

    // dormir el retardo de segundos
    sleep(parametrosHiloSucursal->segundosRetardo);
  }

  pthread_exit(NULL);
}



int main()
{
  leerFicheroConfiguracion("fp.conf");
  inicializar_logger(LOG_FILE);
  mostrarConfiguracion();

  // pool de hilos (procesos ligeros)
  pthread_t threads[NUM_PROCESOS];
  ParametrosHiloSucursal params_hilos[NUM_PROCESOS];

  // Semilla para la generación de números aleatorios
  srand(time(NULL));

  // Crear los hilos
  for (int i = 0; i < NUM_PROCESOS; i++)
  {
    // i+1 (es el numero de la sucursal que se encargara ese hilo)
    // ejemplo si tenemos 5 hilos
    // hilo0 se encarga de sucursal 1
    // hilo1 se encarga de sucursal 2 y asi sucesivamente...
    params_hilos[i].numSucursal = i + 1;
    int segundosRetardoHilo = numeroAleatorio(SIMULATE_SLEEP_MIN, SIMULATE_SLEEP_MAX);
    params_hilos[i].segundosRetardo = segundosRetardoHilo;

    // importante pasarle un struct como parametro al hilo nuevo
    if (pthread_create(&threads[i], NULL, tratarPeticionSucursal, (void *)&params_hilos[i]) != 0)
    {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  // Esperar a que todos los hilos terminen (esto nunca ocurrirá, a menos que los hilos tengan un fin)
  for (int i = 0; i < NUM_PROCESOS; i++)
  {
    if (pthread_join(threads[i], NULL) != 0)
    {
      perror("pthread_join");
      exit(EXIT_FAILURE);
    }
  }

  cerrar_logger();
  return 0;
}
