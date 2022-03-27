#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "unistd.h"

#define N 10
#define K 10000

int count = 0;

void *pthread_func(void *arg) {
    int id = *((int *)arg);

    for (int j = 0; j < K; j++)
        count++;

    pthread_exit(0);
}

int main() {
    pthread_t a[N];

    int i;
    int *id;
    for (i = 0; i < N; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        pthread_create(&a[i], NULL, pthread_func, (void *)(id));
    }

    for (i = 0; i < N; i++) {
        pthread_join(a[i], NULL);
    }

    printf("Count = %d\n", count);
    return 0;
}
