#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "../include/parchis_shared.h"

static const int SALIDA[NUM_JUGADORES] = {1, 18, 35, 52};

typedef struct {
    int              id_jugador;
    int              dado;
    Tablero         *tablero;
    int              fd_resultado;
    int              ya_movio;
    pthread_mutex_t  mtx;
} ContextoTurno;

typedef struct {
    ContextoTurno *turno;
    int            id_ficha;
} ContextoFicha;

static int lanzar_dado(void) {
    return (rand() % 6) + 1;
}

static int fichas_en_meta(const Tablero *t, int jug) {
    int cnt = 0;
    for (int f = 0; f < FICHAS_POR_JUG; f++)
        if (t->fichas[jug][f].estado == FICHA_EN_META) cnt++;
    return cnt;
}

static void *hilo_ficha(void *arg) {
    ContextoFicha *cf  = (ContextoFicha *)arg;
    ContextoTurno *ctx = cf->turno;
    int fid            = cf->id_ficha;
    int jug            = ctx->id_jugador;
    int dado           = ctx->dado;
    Tablero *t         = ctx->tablero;

    pthread_mutex_lock(&ctx->mtx);

    if (ctx->ya_movio) {
        pthread_mutex_unlock(&ctx->mtx);
        return NULL;
    }

    Ficha *ficha = &t->fichas[jug][fid];

    if (ficha->estado == FICHA_EN_BASE) {
        if (dado != 5) {
            pthread_mutex_unlock(&ctx->mtx);
            return NULL;
        }
        pthread_mutex_lock(&t->mutex);
        int salida = SALIDA[jug];
        for (int oj = 0; oj < NUM_JUGADORES; oj++) {
            if (oj == jug) continue;
            for (int of = 0; of < FICHAS_POR_JUG; of++) {
                if (t->fichas[oj][of].estado   == FICHA_EN_JUEGO &&
                    t->fichas[oj][of].posicion  == salida) {
                    t->fichas[oj][of].estado    = FICHA_EN_BASE;
                    t->fichas[oj][of].posicion  = 0;
                }
            }
        }
        ficha->estado   = FICHA_EN_JUEGO;
        ficha->posicion = salida;
        pthread_mutex_unlock(&t->mutex);

        MensajePipe msg = { jug, fid, dado, salida, EVT_SALIO_BASE };
        write(ctx->fd_resultado, &msg, sizeof(MensajePipe));
        ctx->ya_movio = 1;
        pthread_mutex_unlock(&ctx->mtx);
        return NULL;
    }

    if (ficha->estado == FICHA_EN_JUEGO) {
        int nueva_pos  = ficha->posicion + dado;
        int llego_meta = (nueva_pos >= TOTAL_CASILLAS);
        if (llego_meta) nueva_pos = TOTAL_CASILLAS;

        pthread_mutex_lock(&t->mutex);
        MensajePipe msg = { jug, fid, dado, nueva_pos, EVT_MOVIMIENTO };

        if (llego_meta) {
            ficha->estado   = FICHA_EN_META;
            ficha->posicion = TOTAL_CASILLAS;
            if (fichas_en_meta(t, jug) == FICHAS_POR_JUG) {
                t->juego_terminado = 1;
                t->ganador         = jug;
                msg.evento         = EVT_FIN_JUEGO;
            } else {
                msg.evento = EVT_META;
            }
        } else {
            for (int oj = 0; oj < NUM_JUGADORES; oj++) {
                if (oj == jug) continue;
                for (int of = 0; of < FICHAS_POR_JUG; of++) {
                    if (t->fichas[oj][of].estado   == FICHA_EN_JUEGO &&
                        t->fichas[oj][of].posicion == nueva_pos) {
                        t->fichas[oj][of].estado   = FICHA_EN_BASE;
                        t->fichas[oj][of].posicion = 0;
                        msg.evento = EVT_COMIO;
                    }
                }
            }
            ficha->posicion = nueva_pos;
        }

        write(ctx->fd_resultado, &msg, sizeof(MensajePipe));
        pthread_mutex_unlock(&t->mutex);
        ctx->ya_movio = 1;
        pthread_mutex_unlock(&ctx->mtx);
        return NULL;
    }

    pthread_mutex_unlock(&ctx->mtx);
    return NULL;
}

void proceso_jugador(int id, int fd_turno, int fd_resultado, Tablero *tablero) {
    (void)fd_turno;
    printf("[JUGADOR %d] proceso iniciado (PID=%d)\n", id + 1, getpid());

    srand((unsigned)(time(NULL) ^ ((unsigned)getpid() * 2654435761u)));

    pthread_mutex_t mutex_local = PTHREAD_MUTEX_INITIALIZER;

    while (1) {
        sem_wait(&tablero->sem_turno);

        if (tablero->juego_terminado) break;

        int dado = lanzar_dado();

        ContextoTurno ctx_turno = {
            .id_jugador   = id,
            .dado         = dado,
            .tablero      = tablero,
            .fd_resultado = fd_resultado,
            .ya_movio     = 0,
            .mtx          = mutex_local
        };

        pthread_t     hilos[FICHAS_POR_JUG];
        ContextoFicha ctx_ficha[FICHAS_POR_JUG];

        for (int f = 0; f < FICHAS_POR_JUG; f++) {
            ctx_ficha[f].turno    = &ctx_turno;
            ctx_ficha[f].id_ficha = f;
            if (pthread_create(&hilos[f], NULL, hilo_ficha, &ctx_ficha[f]) != 0)
                perror("[JUGADOR] pthread_create");
        }

        for (int f = 0; f < FICHAS_POR_JUG; f++) {
            if (pthread_join(hilos[f], NULL) != 0)
                perror("[JUGADOR] pthread_join");
        }

        if (!ctx_turno.ya_movio) {
            MensajePipe msg = { id, 0, dado, 0, EVT_MOVIMIENTO };
            write(fd_resultado, &msg, sizeof(MensajePipe));
        }

        if (tablero->juego_terminado) break;
    }

    pthread_mutex_destroy(&mutex_local);
    printf("[JUGADOR %d] PID=%d terminando.\n", id + 1, getpid());
}