#include <ncurses.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
// #include <time.h>
#include <unistd.h>

#define N_MINEIROS 5
#define N_FABRICAS 2
#define MAX_MINEIROS_NA_MINA 3
#define MAX_GEMAS_NA_SACOLA 4
#define MIN_GEMAS_NA_ESTEIRA 5

typedef enum mine_object {
    NOTHING,
    GEM,
    MINER
} mine_object;

typedef enum Direction {
    N,
    E,
    S,
    W
} Direction;

typedef struct {
    int i, j;
} MinerPos;

/* MAP FUNCTIONS */
void cria_mapa(int N);
void free_mapa(int N);
void print_mapa(int N);

/* MINER FUNCTIONS */
mine_object move_miner(int id, Direction dir);
void enter_mine(int id);
void leave_mine(int id);
void* f_miner(void* v);

/* FABRIC FUNCTIONS */
void* f_fabric(void* v);

/* Global Variables */
mine_object** map;                 // Map matrix grid of dynamic size
int map_size;                      // Final size is map_size x map_size
int gems = 0;                      // Number of gems on map (generated randomly)
int gems_in_fabric = 0;            // Number of gems in fabric belt
int miners_running = N_MINEIROS;   // Number of miner threads still running
int fabrics_running = N_FABRICAS;  // Number of fabric threads still running
int miners_in_mineshaft = 0;       // Current number of miners in mineshaft
MinerPos minerPos[N_MINEIROS];     // Array for tracking miner position in matrix map

/* Syncronization Tools */
pthread_cond_t min_gemas_na_esteira = PTHREAD_COND_INITIALIZER;
pthread_mutex_t gem_mutex = PTHREAD_MUTEX_INITIALIZER;                  // For changing 'gems' variable safelly
pthread_mutex_t fabric_gem_mutex = PTHREAD_MUTEX_INITIALIZER;           // For changing 'gems_in_fabric' variable safelly
pthread_mutex_t mapa_mutex = PTHREAD_MUTEX_INITIALIZER;                 // For changing data on map safely
pthread_mutex_t miners_in_mineshaft_mutex = PTHREAD_MUTEX_INITIALIZER;  // For changing 'miners_in_mineshaft' variable safelly
pthread_mutex_t fabrics_mutex = PTHREAD_MUTEX_INITIALIZER;              // For changing 'fabrics_running' variable safelly
pthread_mutex_t miners_mutex = PTHREAD_MUTEX_INITIALIZER;               // For changing 'miners_running' variable safelly
pthread_mutex_t scr_mutex = PTHREAD_MUTEX_INITIALIZER;                  // For printing to screen safely
sem_t* mineshaft;                                                       // For mantaining maximum number of miners in mineshaft

int main(int argc, char const* argv[]) {
    pthread_t thr_miners[N_MINEIROS], thr_fabric[N_FABRICAS];
    int* id;

    sem_unlink("mineshaft");
    mineshaft = sem_open("mineshaft", O_CREAT, 0777, MAX_MINEIROS_NA_MINA);
    if (mineshaft == SEM_FAILED) {
        fprintf(stderr, "%s\n", "ERROR creating semaphore mineshaft");
        exit(EXIT_FAILURE);
    }

    srand(time(0));

    printf("Tamanho do mapa: ");
    scanf("%d", &map_size);

    // Alocar tamanho da mina
    map = (mine_object**)malloc(map_size * sizeof(mine_object*));
    for (int i = 0; i < map_size; i++) {
        map[i] = (mine_object*)malloc(map_size * sizeof(mine_object));
    }

    // Coloca gemas aleatóriamente pela mina
    cria_mapa(map_size);

    // Init screen for ncurses
    initscr();

    // create miner threads
    for (int i = 0; i < N_MINEIROS; i++) {
        id = (int*)malloc(sizeof(int));
        *id = i;

        int bad_create = pthread_create(&thr_miners[i], NULL, f_miner, (void*)(id));
        if (bad_create) {
            printf("erro na criacao da thread %d\n", i);
            exit(1);
        }
    }

    // craete fabric thread
    for (int i = 0; i < N_FABRICAS; i++) {
        id = (int*)malloc(sizeof(int));
        *id = i;

        int bad_create = pthread_create(&thr_fabric[i], NULL, f_fabric, (void*)(id));
        if (bad_create) {
            printf("erro na criacao da thread %d\n", i);
            exit(1);
        }
    }

    // Continuously print map
    while (1) {
        print_mapa(map_size);
        refresh();
        if (!fabrics_running) {
            print_mapa(map_size);
            refresh();
            break;
        }
    }

    endwin();

    printf("End\n");

    // espera mineiros
    for (int i = 0; i < N_MINEIROS; i++)
        pthread_join(thr_miners[i], NULL);

    // espera fabricas
    for (int i = 0; i < N_FABRICAS; i++)
        pthread_join(thr_fabric[i], NULL);

    free_mapa(map_size);

    return 0;
}

void cria_mapa(int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int gema = rand() % 5;
            if (gema == 1) {
                map[i][j] = GEM;
                gems++;
            } else {
                map[i][j] = NOTHING;
            }
        }
    }

    printf("Mapa %d X %d criado com %d gemas\n", N, N, gems);
}

void print_mapa(int N) {
    pthread_mutex_lock(&scr_mutex);
    pthread_mutex_lock(&gem_mutex);
    mvprintw(0, 0, "Gemas: %d\t", gems);
    pthread_mutex_unlock(&gem_mutex);

    printw("Max Miners in mineshaft: %d\t Max Gems in Bag: %d\tMiners Running: %d\t Fabrics Running: %d\n", MAX_MINEIROS_NA_MINA, MAX_GEMAS_NA_SACOLA, miners_running, fabrics_running);

    printw("\n");
    for (int i = 0; i < N * 5; i++) {
        printw("-");
    }
    printw("\n");
    pthread_mutex_lock(&mapa_mutex);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            switch (map[i][j]) {
                case NOTHING:
                    printw("|   |");
                    break;
                case GEM:
                    printw("| G |");
                    break;
                case MINER:
                    printw("| M |");
                    break;
                default:
                    break;
            }
        }
        printw("\n");
    }
    pthread_mutex_unlock(&mapa_mutex);
    for (int i = 0; i < N * 5; i++) {
        printw("-");
    }
    printw("\n");
    pthread_mutex_unlock(&scr_mutex);
}

void free_mapa(int N) {
    for (int i = 0; i < N; i++)
        free(map[i]);
    free(map);
}

mine_object move_miner(int id, Direction dir) {
    pthread_mutex_lock(&mapa_mutex);
    mine_object prev = NOTHING;

    // Clear M from Miner
    map[minerPos[id].i][minerPos[id].j] = NOTHING;

    do {
        switch (dir) {
            case N:
                minerPos[id].i = (minerPos[id].i + map_size - 1) % map_size;
                break;
            case E:
                minerPos[id].j = (minerPos[id].j + 1) % map_size;
                break;
            case S:
                minerPos[id].i = (minerPos[id].i + 1) % map_size;
                break;
            case W:
                minerPos[id].j = (minerPos[id].j + map_size - 1) % map_size;
                break;
        }
        prev = map[minerPos[id].i][minerPos[id].j];
        dir = rand() % 4;
    } while (prev == MINER);

    map[minerPos[id].i][minerPos[id].j] = MINER;
    pthread_mutex_unlock(&mapa_mutex);

    return prev;
}

void leave_mine(int id) {
    usleep(((rand() % 500) + 500) * 1000);  // sleep for [0,5 - 1,0] seconds

    pthread_mutex_lock(&mapa_mutex);
    map[minerPos[id].i][minerPos[id].j] = NOTHING;
    pthread_mutex_unlock(&mapa_mutex);
    pthread_mutex_lock(&miners_in_mineshaft_mutex);
    miners_in_mineshaft--;
    pthread_mutex_unlock(&miners_in_mineshaft_mutex);

    usleep((500) * 1000);  // sleep for 0,5 second

    // Clear miner from screen
    pthread_mutex_lock(&scr_mutex);
    mvprintw(map_size + 5, (id * 13), "             ");
    pthread_mutex_unlock(&scr_mutex);
}

void enter_mine(int id) {
    usleep(((rand() % 1000) + 500) * 1000);  // sleep for [0,5 - 1,5] seconds

    // Start miner on a random unoccupied position
    minerPos[id].i = rand() % map_size;
    minerPos[id].j = rand() % map_size;
    while (map[minerPos[id].i][minerPos[id].j] == GEM) {
        minerPos[id].i = rand() % map_size;
        minerPos[id].j = rand() % map_size;
    }

    pthread_mutex_lock(&mapa_mutex);
    map[minerPos[id].i][minerPos[id].j] = MINER;
    pthread_mutex_unlock(&mapa_mutex);
}

void* f_miner(void* v) {
    int id = *(int*)v;
    int pos = 0;
    int gemas_na_sacola = 0;

    while (1) {
        sleep((rand() % 4) + 1);
        sem_wait(mineshaft);  // espera poder entrar na mina

        pthread_mutex_lock(&gem_mutex);
        if (!gems) {
            sem_post(mineshaft);
            pthread_mutex_unlock(&gem_mutex);
            pthread_cond_broadcast(&min_gemas_na_esteira);  // broadcast for any fabrics waiting
            pthread_mutex_lock(&miners_mutex);
            miners_running--;
            pthread_mutex_unlock(&miners_mutex);
            pthread_exit(0);
        }
        pthread_mutex_unlock(&gem_mutex);

        enter_mine(id);

        pthread_mutex_lock(&miners_in_mineshaft_mutex);
        miners_in_mineshaft++;
        pthread_mutex_unlock(&miners_in_mineshaft_mutex);

        pthread_mutex_lock(&scr_mutex);
        mvprintw(map_size + 5, (id * 13), "Miner-%d: %d", id, gemas_na_sacola);
        pthread_mutex_unlock(&scr_mutex);

        // Randomly scavange mine for gems and leave if bag is full or there are no more gems
        // in mine
        while (gemas_na_sacola != MAX_GEMAS_NA_SACOLA) {
            pthread_mutex_lock(&gem_mutex);
            if (!gems) {
                pthread_mutex_unlock(&gem_mutex);
                break;
            }
            pthread_mutex_unlock(&gem_mutex);

            usleep(((rand() % 200) + 200) * 1000);  // sleep for [200 - 400] milliseconds

            Direction dir = rand() % 4;  // 0:North 1:East 2:South 3:West

            if (move_miner(id, dir) == GEM) {
                pthread_mutex_lock(&gem_mutex);
                gems--;
                pthread_mutex_unlock(&gem_mutex);
                gemas_na_sacola++;
                pthread_mutex_lock(&scr_mutex);
                mvprintw(map_size + 5, (id * 13), "Miner-%d: %d", id, gemas_na_sacola);
                pthread_mutex_unlock(&scr_mutex);
            }
        }

        // Miner leaves mine
        leave_mine(id);

        pthread_mutex_lock(&fabric_gem_mutex);
        gems_in_fabric += gemas_na_sacola;
        gemas_na_sacola = 0;

        pthread_mutex_lock(&scr_mutex);
        mvprintw(map_size + 8, 0, "Gems waiting to be processed: %d ", gems_in_fabric);
        pthread_mutex_unlock(&scr_mutex);

        // Caso tenha mais gemas na esteria que o minimo preciso, inicia fábricas
        if (gems_in_fabric >= MIN_GEMAS_NA_ESTEIRA) {
            pthread_mutex_unlock(&fabric_gem_mutex);
            pthread_cond_broadcast(&min_gemas_na_esteira);
        } else {
            pthread_mutex_unlock(&fabric_gem_mutex);
        }

        sem_post(mineshaft);
    }
}

void* f_fabric(void* v) {
    int id = *(int*)v;

    while (1) {
        pthread_mutex_lock(&gem_mutex);
        if (!gems) {
            pthread_mutex_unlock(&gem_mutex);
            usleep(((rand() % 1000) + 500) * 1000);  // sleep for [0,5 - 1,5] seconds
            pthread_mutex_lock(&scr_mutex);
            mvprintw(map_size + 10, (id * 27), "[%d] Fab Status: %s", id, "finished  ");
            pthread_mutex_unlock(&scr_mutex);
            usleep(((rand() % 1000) + 500) * 1000);  // sleep for [0,5 - 1,5] seconds
            pthread_mutex_lock(&fabrics_mutex);
            fabrics_running--;
            pthread_mutex_unlock(&fabrics_mutex);
            pthread_cond_broadcast(&min_gemas_na_esteira);
            pthread_exit(0);
        }
        pthread_mutex_unlock(&gem_mutex);

        usleep(((rand() % 1000) + 500) * 1000);  // sleep for [0,5 - 1,5] seconds

        pthread_mutex_lock(&fabric_gem_mutex);
        while (gems_in_fabric < MIN_GEMAS_NA_ESTEIRA && gems) {  // Wait for minimum number of gems in fabric belt
            pthread_mutex_lock(&scr_mutex);
            mvprintw(map_size + 10, (id * 27), "[%d] Fab Status: %s", id, "waiting   ");
            pthread_mutex_unlock(&scr_mutex);

            pthread_cond_wait(&min_gemas_na_esteira, &fabric_gem_mutex);
        }
        pthread_mutex_lock(&scr_mutex);
        mvprintw(map_size + 10, (id * 27), "[%d] Fab Status: %s", id, "processing");
        pthread_mutex_unlock(&scr_mutex);

        pthread_mutex_unlock(&fabric_gem_mutex);

        while (1) {
            pthread_mutex_lock(&fabric_gem_mutex);
            if (!gems_in_fabric) {  // Leave if there are no more gems in fabric
                pthread_mutex_lock(&scr_mutex);
                mvprintw(map_size + 10, (id * 27), "[%d] Fab Status: %s", id, "stopping  ");
                pthread_mutex_unlock(&scr_mutex);
                pthread_mutex_unlock(&fabric_gem_mutex);
                break;
            }
            pthread_mutex_unlock(&fabric_gem_mutex);

            // Process one gem per turn until gems end
            pthread_mutex_lock(&fabric_gem_mutex);
            gems_in_fabric--;
            pthread_mutex_lock(&scr_mutex);
            mvprintw(map_size + 8, 0, "Gems waiting to be processed: %d ", gems_in_fabric);
            pthread_mutex_unlock(&scr_mutex);
            pthread_mutex_unlock(&fabric_gem_mutex);

            usleep(((rand() % 1000) + 500) * 1000);  // sleep for [0,5 - 1,5] seconds
        }
    }
}