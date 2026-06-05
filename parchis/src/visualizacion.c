#include <stdio.h>
#include "../include/parchis_shared.h"

#define C_RESET  "\033[0m"
#define C_J1     "\033[1;31m"
#define C_J2     "\033[1;34m"
#define C_J3     "\033[1;32m"
#define C_J4     "\033[1;33m"

static const char *colores[NUM_JUGADORES] = { C_J1, C_J2, C_J3, C_J4 };
static const char *nombres[NUM_JUGADORES] = {
    "Jugador 1 (Rojo)", "Jugador 2 (Azul)",
    "Jugador 3 (Verde)", "Jugador 4 (Amarillo)"
};

static const char *estado_str(int e) {
    if (e == FICHA_EN_BASE)  return "BASE ";
    if (e == FICHA_EN_JUEGO) return "JUEGO";
    if (e == FICHA_EN_META)  return "META ";
    return "?????";
}

void imprimir_tablero(const Tablero *tablero) {
    printf("\n+--------------------------------------------------+\n");
    printf("|          SIMULADOR DE PARCHIS - ESTADO           |\n");
    printf("+--------------------------------------------------+\n");
    printf("| Turno actual: Jugador %-2d   Terminado: %s         |\n",
           tablero->turno_actual + 1,
           tablero->juego_terminado ? "SI " : "NO ");
    printf("+--------------------------------------------------+\n");

    for (int j = 0; j < NUM_JUGADORES; j++) {
        int activo = (j == tablero->turno_actual && !tablero->juego_terminado);
        printf("| %s%-20s%s %s|\n",
               colores[j], nombres[j], C_RESET,
               activo ? " <<< TURNO ACTIVO >>>" : "                    ");
        for (int f = 0; f < FICHAS_POR_JUG; f++) {
            printf("|   Ficha %d: [%s]  pos=%-3d                        |\n",
                   f + 1,
                   estado_str(tablero->fichas[j][f].estado),
                   tablero->fichas[j][f].posicion);
        }
        printf("|                                                  |\n");
    }
    printf("+--------------------------------------------------+\n\n");
    fflush(stdout);
}