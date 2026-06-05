#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "../include/parchis_shared.h"

void init_tablero(Tablero *tablero) {
    memset(tablero, 0, sizeof(Tablero));
    tablero->turno_actual    = 0;
    tablero->juego_terminado = 0;
    tablero->ganador         = -1;

    for (int i = 0; i < NUM_JUGADORES; i++)
        for (int j = 0; j < FICHAS_POR_JUG; j++) {
            tablero->fichas[i][j].estado   = FICHA_EN_BASE;
            tablero->fichas[i][j].posicion = 0;
        }

    // Mutex process-shared
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&tablero->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    // Un semáforo por jugador (pshared=1 para entre procesos, valor inicial=0)
    for (int i = 0; i < NUM_JUGADORES; i++)
        sem_init(&tablero->sem_turno[i], 1, 0);

    printf("[TABLERO] Inicializado correctamente.\n");
}