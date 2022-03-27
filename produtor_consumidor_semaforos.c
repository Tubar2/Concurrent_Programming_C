#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PR 2  // número de produtores
#define CN 3  // número de consumidores
#define N 5   // tamanho do buffer

void read_data_base();
void use_data_read();
void think_up_data();
void write_data_base();
void *produtor(void *meuid);
void *consumidor(void *meuid);

sem_t *empty;
sem_t *full;

int main(int argc, char *argv[]) {
    int erro;
    int i, n, m;
    int *id;

    sem_unlink("empty");
    sem_unlink("full");
    empty = sem_open("empty", O_CREAT, 0777, N);
    if (empty == SEM_FAILED) {
        fprintf(stderr, "%s\n", "ERROR creating semaphore empty");
        exit(EXIT_FAILURE);
    }

    full = sem_open("full", O_CREAT, 0777, 0);
    if (full == SEM_FAILED) {
        fprintf(stderr, "%s\n", "ERROR creating semaphore full");
        exit(EXIT_FAILURE);
    }

    pthread_t tid[PR];

    for (i = 0; i < PR; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        erro = pthread_create(&tid[i], NULL, produtor, (void *)(id));

        if (erro) {
            printf("erro na criacao do thread %d\n", i);
            exit(1);
        }
    }

    pthread_t tCid[CN];

    for (i = 0; i < CN; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        erro = pthread_create(&tCid[i], NULL, consumidor, (void *)(id));

        if (erro) {
            printf("erro na criacao do thread %d\n", i);
            exit(1);
        }
    }

    pthread_join(tid[0], NULL);
}

void *produtor(void *arg) {
    int i = *((int *)arg);
    while (1) {
        think_up_data(i); /* região não crítica */
        sem_wait(empty);
        write_data_base(i); /* atualiza os dados */
        sem_post(full);
    }
    pthread_exit(0);
}

void *consumidor(void *arg) {
    int i = *((int *)arg);
    while (1) {
        printf("Leitor %d está esperando!\n", i);
        sem_wait(full);
        read_data_base(i); /* acesso aos dados */
        sem_post(empty);
    }
    use_data_read(i); /* região não crítica */
    pthread_exit(0);
}

void read_data_base(int i) {
    printf("Leitor %d está lendo os dados!\n", i);
    // printf("Leitor %d está lendo os dados! Número de leitores: %d\n", i, num_leitores);
    sleep(rand() % 5);
}

void use_data_read(int i) {
    printf("Leitor %d está usando os dados lidos!\n", i);
    // printf("Leitor %d está usando os dados lidos! Número de leitores: %d\n", i, num_leitores);
    sleep(rand() % 5);
}

void think_up_data(int i) {
    printf("Escritor %d está pensando no que escrever!\n", i);
    sleep(rand() % 5);
}

void write_data_base(int i) {
    printf("Escritor %d está escrevendo os dados!\n", i);
    // printf("Escritor %d está escrevendo os dados! Número de leitores: %d\n", i, num_leitores);
    sleep(rand() % 15);
}