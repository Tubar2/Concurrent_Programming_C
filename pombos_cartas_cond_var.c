#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "unistd.h"

#define N 10  // número de usuários

#define CARTAS 20  // quantidade de cartas na mochila

pthread_cond_t pombo_em_A = PTHREAD_COND_INITIALIZER;
pthread_cond_t mochila_cheia = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mochila_pombo = PTHREAD_MUTEX_INITIALIZER;

void *f_usuario(void *arg);
void *f_pombo(void *arg);

int main(int argc, char **argv) {
    int i;

    pthread_t usuario[N];
    int *id;
    for (i = 0; i < N; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        pthread_create(&(usuario[i]), NULL, f_usuario, (void *)(id));
    }
    pthread_t pombo;
    id = (int *)malloc(sizeof(int));
    *id = 0;
    pthread_create(&(pombo), NULL, f_pombo, (void *)(id));

    pthread_join(pombo, NULL);
}

int n_cartas = 0;

void *f_pombo(void *arg) {
    time_t rawtime;
    struct tm *timeinfo;
    struct tm buf;
    char str[30];

    while (1) {
        // Inicialmente está em A, aguardar/dorme a mochila ficar cheia (20 cartas)
        pthread_mutex_lock(&mochila_pombo);
        time(&rawtime);
        timeinfo = localtime_r(&rawtime, &buf);
        asctime_r(timeinfo, str);
        str[24] = '\0';
        if (n_cartas != 20) {
            printf("[%s] Pombo esperando em A\n", str);
            pthread_cond_wait(&mochila_cheia, &mochila_pombo);
        }
        // Leva as cartas para B e volta para A
        time(&rawtime);
        timeinfo = localtime_r(&rawtime, &buf);
        asctime_r(timeinfo, str);
        str[24] = '\0';
        printf("[%s] Pombo em B\n", str);

        sleep(3);
        n_cartas = 0;

        // Acordar os usuários
        pthread_mutex_unlock(&mochila_pombo);
    }
}

void *f_usuario(void *arg) {
    int id = *(int *)arg;
    time_t rawtime;
    struct tm *timeinfo;
    struct tm buf;
    char str[30];
    while (1) {
        // Escreve uma carta
        time(&rawtime);
        timeinfo = localtime_r(&rawtime, &buf);
        asctime_r(timeinfo, str);
        str[24] = '\0';
        printf("[%s] Usuário %d escrevendo carta\n", str, id);
        sleep(rand() % 7);

        // Caso o pombo não esteja em A ou a mochila estiver cheia, então dorme
        pthread_mutex_lock(&mochila_pombo);

        // Posta sua carta na mochila do pombo
        n_cartas++;
        time(&rawtime);
        timeinfo = localtime_r(&rawtime, &buf);
        asctime_r(timeinfo, str);
        str[24] = '\0';
        printf("[%s] Usuário %d escreveu carta: %d\n", str, id, n_cartas);
        if (n_cartas == 20) {
            // Caso a mochila fique cheia, acorda o ṕombo
            printf("Usuário %d encheu mochila\n", id);
            pthread_cond_signal(&mochila_cheia);
        }
        pthread_mutex_unlock(&mochila_pombo);
    }
}
