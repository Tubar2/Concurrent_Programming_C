#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MA 10  // macacos que andam de A para B
#define MB 10  // macacos que andam de B para A

// int macaco_AB_count = 0;
// int macaco_BA_count = 0;
int countAB = 0;
int countBA = 0;

pthread_mutex_t lock_ponte = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_countAB = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_countBA = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gorilla_wants_to_pass = PTHREAD_MUTEX_INITIALIZER;

void *macacoAB(void *a) {
    int i = *((int *)a);
    while (1) {
        // pthread_mutex_lock(&gorilla_wants_to_pass);
        // pthread_mutex_unlock(&gorilla_wants_to_pass);

        pthread_mutex_lock(&lock_countBA);
        pthread_mutex_lock(&lock_countAB);

        // Procedimentos para acessar a corda
        countAB++;
        if (countAB == 1) {
            pthread_mutex_lock(&lock_ponte);
        }
        pthread_mutex_unlock(&lock_countAB);
        pthread_mutex_unlock(&lock_countBA);

        // Acessar corda
        printf("Macaco %d passando de A para B \n", i);
        sleep(1);

        // Procedimentos para quando sair da corda
        pthread_mutex_lock(&lock_countAB);
        countAB--;
        if (countAB == 0) {
            pthread_mutex_unlock(&lock_ponte);
        }
        pthread_mutex_unlock(&lock_countAB);
    }
    pthread_exit(0);
}

void *macacoBA(void *a) {
    int i = *((int *)a);
    while (1) {
        // pthread_mutex_lock(&gorilla_wants_to_pass);
        // pthread_mutex_unlock(&gorilla_wants_to_pass);

        pthread_mutex_lock(&lock_countBA);
        pthread_mutex_lock(&lock_countAB);

        // Procedimentos para acessar a corda
        countBA++;
        if (countBA == 1) {
            pthread_mutex_lock(&lock_ponte);
        }
        pthread_mutex_unlock(&lock_countBA);
        pthread_mutex_unlock(&lock_countAB);

        // Acessar corda
        printf("Macaco %d passando de B para A \n", i);
        sleep(1);

        // Procedimentos para quando sair da corda
        pthread_mutex_lock(&lock_countBA);
        countBA--;
        if (countBA == 0) {
            pthread_mutex_unlock(&lock_ponte);
        }
        pthread_mutex_unlock(&lock_countBA);
    }
    pthread_exit(0);
}

void *gorila(void *a) {
    while (1) {
        // Procedimentos para acessar a corda
        pthread_mutex_lock(&gorilla_wants_to_pass);

        pthread_mutex_lock(&lock_ponte);

        printf("Gorila passado de A para B \n");
        sleep(5);

        // Procedimentos para quando sair da corda
        pthread_mutex_unlock(&gorilla_wants_to_pass);
        pthread_mutex_unlock(&lock_ponte);
    }
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    pthread_t macacos[MA + MB];
    int *id;
    int i = 0;

    for (i = 0; i < MA + MB; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        if (i % 2 == 0) {
            if (pthread_create(&macacos[i], NULL, &macacoAB, (void *)id)) {
                printf("Não pode criar a thread %d\n", i);
                return -1;
            }
        } else {
            if (pthread_create(&macacos[i], NULL, &macacoBA, (void *)id)) {
                printf("Não pode criar a thread %d\n", i);
                return -1;
            }
        }
    }

    // pthread_t g;
    // pthread_create(&g, NULL, &gorila, NULL);

    pthread_join(macacos[0], NULL);
    return 0;
}
