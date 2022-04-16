#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/////
#ifndef PTHREAD_BARRIER_H
#define PTHREAD_BARRIER_H

#ifdef __APPLE__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(PTHREAD_BARRIER_SERIAL_THREAD)
#define PTHREAD_BARRIER_SERIAL_THREAD (1)
#endif

#if !defined(PTHREAD_PROCESS_PRIVATE)
#define PTHREAD_PROCESS_PRIVATE (42)
#endif
#if !defined(PTHREAD_PROCESS_SHARED)
#define PTHREAD_PROCESS_SHARED (43)
#endif

typedef struct {
} pthread_barrierattr_t;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned int limit;
    unsigned int count;
    unsigned int phase;
} pthread_barrier_t;

int pthread_barrierattr_init(pthread_barrierattr_t *attr);
int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);

int pthread_barrierattr_getpshared(const pthread_barrierattr_t *restrict attr,
                                   int *restrict pshared);
int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr,
                                   int pshared);

int pthread_barrier_init(pthread_barrier_t *restrict barrier,
                         const pthread_barrierattr_t *restrict attr,
                         unsigned int count);
int pthread_barrier_destroy(pthread_barrier_t *barrier);

int pthread_barrier_wait(pthread_barrier_t *barrier);

#ifdef __cplusplus
}
#endif

#endif /* __APPLE__ */

#endif /* PTHREAD_BARRIER_H */
/////
#ifdef __APPLE__

// #define __unused __attribute__((unused))

int pthread_barrierattr_init(pthread_barrierattr_t *attr __unused) {
    return 0;
}

int pthread_barrierattr_destroy(pthread_barrierattr_t *attr __unused) {
    return 0;
}

int pthread_barrierattr_getpshared(const pthread_barrierattr_t *restrict attr __unused,
                                   int *restrict pshared) {
    *pshared = PTHREAD_PROCESS_PRIVATE;
    return 0;
}

int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr __unused,
                                   int pshared) {
    if (pshared != PTHREAD_PROCESS_PRIVATE) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int pthread_barrier_init(pthread_barrier_t *restrict barrier,
                         const pthread_barrierattr_t *restrict attr __unused,
                         unsigned count) {
    if (count == 0) {
        errno = EINVAL;
        return -1;
    }

    if (pthread_mutex_init(&barrier->mutex, 0) < 0) {
        return -1;
    }
    if (pthread_cond_init(&barrier->cond, 0) < 0) {
        int errno_save = errno;
        pthread_mutex_destroy(&barrier->mutex);
        errno = errno_save;
        return -1;
    }

    barrier->limit = count;
    barrier->count = 0;
    barrier->phase = 0;

    return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier) {
    pthread_mutex_destroy(&barrier->mutex);
    pthread_cond_destroy(&barrier->cond);
    return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier) {
    pthread_mutex_lock(&barrier->mutex);
    barrier->count++;
    if (barrier->count >= barrier->limit) {
        barrier->phase++;
        barrier->count = 0;
        pthread_cond_broadcast(&barrier->cond);
        pthread_mutex_unlock(&barrier->mutex);
        return PTHREAD_BARRIER_SERIAL_THREAD;
    } else {
        unsigned phase = barrier->phase;
        do
            pthread_cond_wait(&barrier->cond, &barrier->mutex);
        while (phase == barrier->phase);
        pthread_mutex_unlock(&barrier->mutex);
        return 0;
    }
}

#endif /* __APPLE__ */
/////

#define MAXSIZE 10000 /* maximum matrix size */

pthread_barrier_t barrier;
int size;

int matrix[MAXSIZE][MAXSIZE];
int Res[MAXSIZE];

void *Worker(void *);

int main(int argc, char *argv[]) {
    int i, j;

    size = atoi(argv[1]);

    if (size > MAXSIZE) {
        printf("Tamanho muito grande!\n");
        return 0;
    }

    pthread_t workerid[size];

    pthread_barrier_init(&barrier, NULL, size);

    for (i = 0; i < size; i++)
        for (j = 0; j < size; j++)
            matrix[i][j] = (i + 1) * (j + 1);
    //    1 2 3 4 5
    // 1  1 2 3 4 5     -> 16
    // 2  2 4 6 8 10    -> 30
    // 3  3 6 9 12 15   -> 45
    // 4  4 8 13 16 20  -> 61
    // 5  5 10 13 20 25 -> 73
    //                     225

    for (j = 0; j < size; j++)
        Res[j] = 0;

    int *id;
    for (i = 0; i < size; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        pthread_create(&workerid[i], NULL, Worker, (void *)(id));
    }

    for (i = 0; i < size; i++) {
        if (pthread_join(workerid[i], NULL)) {
            printf("\n ERROR joining thread");
            exit(1);
        }
    }
    printf("Bye!\n");
}

void *Worker(void *arg) {
    int myid = *(int *)(arg);
    int j, k;
    int add = 0;

    int self = pthread_self();

    printf("worker %d (pthread id %d) has started\n", myid, self);

    // int lineRes = 0;
    for (k = 0; k < size; k++) {
        Res[myid] += matrix[myid][k];
    }

    pthread_barrier_wait(&barrier);

    if (myid == 0) {
        printf("\n");
        for (j = 0; j < size; j++) {
            add += Res[j];
        }
        printf("res: %d", add);
        printf("\n ");
    }

    pthread_exit(0);
}
