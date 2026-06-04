#include <stdio.h>
#include "../include/parchis_shared.h"

void imprimir_tablero(const Tablero *tablero) {
    printf("[TABLERO] turno=%d terminado=%d\n",
           tablero->turno_actual, tablero->juego_terminado);
}
