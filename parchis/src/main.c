#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <errno.h>

#include "../include/parchis_shared.h"

static void   crear_memoria_compartida(int *shmid, Tablero **tablero);
static void   crear_pipes(int pipes_escritura[][2], int pipes_lectura[][2]);
static void   lanzar_jugadores(pid_t pids[], int pipes_escritura[][2],
                                int pipes_lectura[][2], Tablero *tablero, int shmid);
static void   scheduler_round_robin(pid_t pids[], int pipes_lectura[][2],
                                     Tablero *tablero);
static void   dar_turno(int jugador, Tablero *tablero);
static int    leer_mensaje(int fd_lectura, MensajePipe *msg);
static void   registrar_evento(const MensajePipe *msg);
static void   liberar_recursos(int shmid, Tablero *tablero,
                                pid_t pids[], int pipes_escritura[][2],
                                int pipes_lectura[][2]);

extern void   init_tablero(Tablero *tablero);

int main(void) {
    int     shmid;
    Tablero *tablero;
    pid_t   pids[NUM_JUGADORES];
   




    int pipes_escritura[NUM_JUGADORES][2];
    int pipes_lectura[NUM_JUGADORES][2];

    printf("[SCHEDULER] Iniciando simulador de parchís...\n");

    crear_memoria_compartida(&shmid, &tablero);
    init_tablero(tablero);                         
    crear_pipes(pipes_escritura, pipes_lectura);
    lanzar_jugadores(pids, pipes_escritura, pipes_lectura, tablero, shmid);
    scheduler_round_robin(pids, pipes_lectura, tablero);
    liberar_recursos(shmid, tablero, pids, pipes_escritura, pipes_lectura);

    printf("[SCHEDULER] Juego terminado. Ganador: Jugador %d\n",
           tablero->ganador + 1);
    return 0;
}

static void crear_memoria_compartida(int *shmid, Tablero **tablero) {
    *shmid = shmget(SHM_KEY, sizeof(Tablero), IPC_CREAT | 0666);
    if (*shmid < 0) {
        perror("[SCHEDULER] shmget");
        exit(EXIT_FAILURE);
    }
    *tablero = (Tablero *)shmat(*shmid, NULL, 0);
    if (*tablero == (Tablero *)-1) {
        perror("[SCHEDULER] shmat");
        shmctl(*shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    printf("[SCHEDULER] Memoria compartida creada (shmid=%d, %zu bytes)\n",
           *shmid, sizeof(Tablero));
}

static void crear_pipes(int pipes_escritura[][2], int pipes_lectura[][2]) {
    for (int i = 0; i < NUM_JUGADORES; i++) {
        if (pipe(pipes_escritura[i]) < 0) {
            perror("[SCHEDULER] pipe escritura");
            exit(EXIT_FAILURE);
        }
        if (pipe(pipes_lectura[i]) < 0) {
            perror("[SCHEDULER] pipe lectura");
            exit(EXIT_FAILURE);
        }
    }
    printf("[SCHEDULER] Pipes creados para %d jugadores\n", NUM_JUGADORES);
}

static void lanzar_jugadores(pid_t pids[], int pipes_escritura[][2],
                              int pipes_lectura[][2], Tablero *tablero, int shmid) {
    for (int i = 0; i < NUM_JUGADORES; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("[SCHEDULER] fork");
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0) {
            


            for (int j = 0; j < NUM_JUGADORES; j++) {
                close(pipes_escritura[j][1]);   
                close(pipes_lectura[j][0]);    
                if (j != i) {
                    close(pipes_escritura[j][0]);
                    close(pipes_lectura[j][1]);
                }
            }

            
            extern void proceso_jugador(int id, int fd_turno, int fd_resultado,
                                        Tablero *tablero);
            proceso_jugador(i,
                            pipes_escritura[i][0],  
                            pipes_lectura[i][1],    
                            tablero);
            exit(EXIT_SUCCESS);
        }


        close(pipes_escritura[i][0]);
        close(pipes_lectura[i][1]);

        printf("[SCHEDULER] Jugador %d lanzado (PID=%d)\n", i + 1, pids[i]);
    }
}

static void scheduler_round_robin(pid_t pids[], int pipes_lectura[][2],
                                   Tablero *tablero) {
    MensajePipe msg;
    int turno = 0;

    printf("[SCHEDULER] Iniciando Round Robin...\n\n");

    while (!tablero->juego_terminado) {
        tablero->turno_actual = turno;

        dar_turno(turno, tablero);

        if (leer_mensaje(pipes_lectura[turno][0], &msg) < 0) {
            fprintf(stderr, "[SCHEDULER] Error leyendo pipe jugador %d\n", turno + 1);
            break;
        }

        registrar_evento(&msg);
        imprimir_tablero(tablero);

        if (msg.evento == EVT_FIN_JUEGO) {
            tablero->juego_terminado = 1;
            tablero->ganador         = turno;
            break;
        }

        turno = (turno + 1) % NUM_JUGADORES;
    }
}

static void dar_turno(int jugador, Tablero *tablero) {
    printf("[SCHEDULER] >> Turno: Jugador %d\n", jugador + 1);
    sem_post(&tablero->sem_turno[jugador]);
}

static int leer_mensaje(int fd_lectura, MensajePipe *msg) {
    ssize_t n = read(fd_lectura, msg, sizeof(MensajePipe));
    if (n != sizeof(MensajePipe)) {
        if (n == 0)
            fprintf(stderr, "[SCHEDULER] Pipe cerrado inesperadamente\n");
        else
            perror("[SCHEDULER] read pipe");
        return -1;
    }
    return 0;
}

static void registrar_evento(const MensajePipe *msg) {
    const char *nombres_evento[] = {
        "movimiento", "salió de base", "comió ficha", "llegó a meta", "fin de juego"
    };
    int ev = (msg->evento >= 0 && msg->evento <= EVT_FIN_JUEGO)
             ? msg->evento : EVT_MOVIMIENTO;

    printf("[PIPE] Jugador %d | Ficha %d | Dado: %d | Pos: %d | Evento: %s\n",
           msg->jugador + 1,
           msg->ficha   + 1,
           msg->dado,
           msg->pos_nueva,
           nombres_evento[ev]);
}

static void liberar_recursos(int shmid, Tablero *tablero,
                              pid_t pids[], int pipes_escritura[][2],
                              int pipes_lectura[][2]) {
    for (int i = 0; i < NUM_JUGADORES; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0)
            perror("[SCHEDULER] waitpid");
        else if (WIFEXITED(status))
            printf("[SCHEDULER] Jugador %d (PID=%d) terminó (código %d).\n",
                   i + 1, pids[i], WEXITSTATUS(status));
        else
            printf("[SCHEDULER] Jugador %d (PID=%d) terminó anormalmente.\n",
                   i + 1, pids[i]);
    }

    for (int i = 0; i < NUM_JUGADORES; i++) {
        close(pipes_escritura[i][1]);
        close(pipes_lectura[i][0]);
    }

    pthread_mutex_destroy(&tablero->mutex);
    for (int i = 0; i < NUM_JUGADORES; i++)
        sem_destroy(&tablero->sem_turno[i]);

    if (shmdt(tablero) < 0)
        perror("[SCHEDULER] shmdt");
    if (shmctl(shmid, IPC_RMID, NULL) < 0)
        perror("[SCHEDULER] shmctl IPC_RMID");

    printf("[SCHEDULER] Recursos liberados correctamente.\n");
}