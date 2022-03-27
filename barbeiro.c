/*
 * Problema do barbeiro dorminhoco.
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define N_CLIENTES 50
#define N_CADEIRAS 5

sem_t* barber_waiting_room;
sem_t* client_in_barber_chair;
pthread_mutex_t barber_chair = PTHREAD_MUTEX_INITIALIZER;

void* f_barbeiro(void* v) {
    time_t rawtime;
    struct tm* timeinfo;
    struct tm buf;
    char str[30];

    while (1) {
        //...Esperar/dormindo algum cliente sentar na cadeira do barbeiro (e acordar o barbeiro)
        sem_wait(client_in_barber_chair);

        sleep(2);  // Cortar o cabelo do cliente
        time(&rawtime);
        timeinfo = localtime_r(&rawtime, &buf);
        asctime_r(timeinfo, str);
        str[24] = '\0';
        printf("[%s] Barbeiro cortou o cabelo de um cliente\n", str);

        //...Liberar/desbloquear o cliente
        sem_post(client_in_barber_chair);
    }
    pthread_exit(0);
}

void* f_cliente(void* v) {
    int id = *(int*)v;

    time_t rawtime;
    struct tm* timeinfo;
    struct tm buf;
    char str[30];

    sleep((rand() % 20) + 5);  // Aguardar de 5 - 25 sec

    if (sem_trywait(barber_waiting_room) == 0) {  // conseguiu pegar uma cadeira de espera
        time(&rawtime);
        timeinfo = localtime_r(&rawtime, &buf);
        asctime_r(timeinfo, str);
        str[24] = '\0';
        printf("[%s] Cliente %d entrou na barbearia\n", str, id);

        //... pegar/sentar a cadeira do barbeiro
        pthread_mutex_lock(&barber_chair);

        //... acordar o barbeiro para cortar seu cabelo
        sem_post(client_in_barber_chair);

        //... aguardar o corte do seu cabelo
        sem_wait(client_in_barber_chair);

        //... liberar a cadeira do barbeiro
        pthread_mutex_unlock(&barber_chair);

        //... liberar a sua cadeira de espera
        sem_post(barber_waiting_room);
        time(&rawtime);
        timeinfo = localtime_r(&rawtime, &buf);
        asctime_r(timeinfo, str);
        str[24] = '\0';
        printf("[%s] Cliente %d cortou o cabelo e foi embora \n", str, id);

    } else {  // barbearia cheia
        time(&rawtime);
        timeinfo = localtime_r(&rawtime, &buf);
        asctime_r(timeinfo, str);
        str[24] = '\0';
        printf("[%s] Barbearia cheia, cliente %d indo embora\n", str, id);
    }

    pthread_exit(0);
}

int main() {
    pthread_t thr_clientes[N_CLIENTES], thr_barbeiro;
    int i, id[N_CLIENTES];

    sem_unlink("sem_cadeiras");
    sem_unlink("barber_waiting_room");
    barber_waiting_room = sem_open("barber_waiting_room", O_CREAT, 0777, N_CADEIRAS);
    if (barber_waiting_room == SEM_FAILED) {
        fprintf(stderr, "%s\n", "ERROR creating semaphore barber_waiting_room");
        exit(EXIT_FAILURE);
    }

    sem_unlink("client_in_barber_chair");
    client_in_barber_chair = sem_open("client_in_barber_chair", O_CREAT, 0777, 0);
    if (client_in_barber_chair == SEM_FAILED) {
        fprintf(stderr, "%s\n", "ERROR creating semaphore client_in_barber_chair");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < N_CLIENTES; i++) {
        id[i] = i;
        pthread_create(&thr_clientes[i], NULL, f_cliente, (void*)&id[i]);
    }

    pthread_create(&thr_barbeiro, NULL, f_barbeiro, NULL);

    for (i = 0; i < N_CLIENTES; i++)
        pthread_join(thr_clientes[i], NULL);

    /* Barbeiro assassinado */
    sem_unlink("barber_waiting_room");
    sem_unlink("client_in_barber_chair");

    return 0;
}
