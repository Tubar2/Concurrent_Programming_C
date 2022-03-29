#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define N 5
#define ESQ(id) (id + N - 1) % N
#define DIR(id) (id + 1) % N

#define COMENDO 0
#define COM_FOME 1
#define PENSANDO 2

sem_t *s[N];
int estado_filosofo[N];
pthread_mutex_t mesa = PTHREAD_MUTEX_INITIALIZER;

void *filosofos(void *arg);

void pega_talher(int n);
void devolve_talher(int n);

int main() {
    int i;
    int *id;
    // semaforo
    for (i = 0; i < N; i++) {
        char str[6];
        sprintf(str, "sem_%d", i);
        printf("%s\n", str);
        sem_unlink(str);
        s[i] = sem_open(str, O_CREAT, 0777, 0);
        if (s[i] == SEM_FAILED) {
            fprintf(stderr, "ERROR creating semaphore %s\n", str);
            exit(EXIT_FAILURE);
        }
        estado_filosofo[i] = PENSANDO;
    }
    pthread_t r[N];

    // criacao das threads de filosofos
    for (i = 0; i < N; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        pthread_create(&r[i], NULL, filosofos, (void *)(id));
    }

    pthread_join(r[0], NULL);
    return 0;
}

void *filosofos(void *arg) {
    int n = *((int *)arg);
    while (1) {
        // pensar
        printf("Filosofo %d pensando\n", n);
        sleep(3);
        // comer
        pega_talher(n);
        printf("\tFilosofo %d comendo\n", n);
        sleep(3);
        printf("Filosofo %d acabou de comer\n", n);
        devolve_talher(n);
    }
}

void pode_comer(int i) {
    if (
        estado_filosofo[i] == COM_FOME &&
        estado_filosofo[ESQ(i)] != COMENDO &&
        estado_filosofo[DIR(i)] != COMENDO) {
        estado_filosofo[i] = COMENDO;
        sem_post(s[i]);
    }
}

void pega_talher(int n) {
    pthread_mutex_lock(&mesa);
    estado_filosofo[n] = COM_FOME;
    // Ver se pode pegar talheres
    pode_comer(n);

    pthread_mutex_unlock(&mesa);
    sem_wait(s[n]);
}

void devolve_talher(int n) {
    pthread_mutex_lock(&mesa);
    estado_filosofo[n] = PENSANDO;
    pode_comer(ESQ(n));
    pode_comer(DIR(n));
    pthread_mutex_unlock(&mesa);
}
