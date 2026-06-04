#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "../include/parchis_shared.h"

void proceso_jugador(int id, int fd_turno, int fd_resultado, Tablero *tablero) {
    (void)fd_turno;
    printf("[JUGADOR %d] proceso iniciado (stub)\n", id + 1);

    MensajePipe msg;
    memset(&msg, 0, sizeof(msg));
    msg.jugador  = id;
    msg.evento   = EVT_FIN_JUEGO;

    sem_wait(&tablero->sem_turno);
    write(fd_resultado, &msg, sizeof(MensajePipe));
}