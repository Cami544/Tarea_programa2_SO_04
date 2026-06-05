#ifndef PARCHIS_SHARED_H
#define PARCHIS_SHARED_H

#include <pthread.h>
#include <semaphore.h>

#define NUM_JUGADORES   4
#define FICHAS_POR_JUG  4
#define TOTAL_CASILLAS  68

#define FICHA_EN_BASE   0
#define FICHA_EN_JUEGO  1
#define FICHA_EN_META   2

#define SHM_KEY         0x1234

typedef struct {
    int estado;
    int posicion;
} Ficha;

typedef struct {
    Ficha fichas[NUM_JUGADORES][FICHAS_POR_JUG];
    int   turno_actual;
    int   juego_terminado;
    int   ganador;
    pthread_mutex_t mutex;
    sem_t           sem_turno;
} Tablero;

typedef struct {
    int jugador;
    int ficha;
    int dado;
    int pos_nueva;
    int evento;
} MensajePipe;

#define EVT_MOVIMIENTO  0
#define EVT_SALIO_BASE  1
#define EVT_COMIO       2
#define EVT_META        3
#define EVT_FIN_JUEGO   4

/* Visualizacion */
void imprimir_tablero(const Tablero *tablero);

#endif