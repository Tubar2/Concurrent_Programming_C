#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PR 4  // número de produtores
#define CN 7  // número de consumidores
#define N 5   // tamanho do buffer

void *produtor(void *meuid);
void *consumidor(void *meuid);

int items[N];
int itemId = 0;
int count = 0;

typedef struct Item {
    int pos, val;
} Item;

pthread_mutex_t itemID_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t itemArr_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t p_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_cond = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]) {
    int erro;
    int i, n, m;
    int *id;

    for (int i = 0; i < N; i++) {
        items[i] = -1;
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

int produce_item() {
    sleep(1);

    pthread_mutex_lock(&itemID_mutex);
    int item_id = itemId++;
    pthread_mutex_unlock(&itemID_mutex);

    return item_id;
}

int insert_item(int item_id) {
    int insert_at = -1;
    for (int i = rand() % N;; i = ((i + 1) % N)) {  // Cycle through arr
        if (items[i] == -1) {
            items[i] = item_id;
            insert_at = i;
            break;
        }
    }
    return insert_at;
}

// Consome um item aleatório do array
Item consume_item() {
    sleep(1);
    int consumed_at = -1;
    int val = -1;
    for (int i = rand() % N;; i = ((i + 1) % N)) {
        if (items[i] != -1) {
            val = items[i];
            consumed_at = i;
            items[i] = -1;
            break;
        }
    }
    Item item = {.pos = consumed_at, .val = val};
    return item;
}

void *produtor(void *pi) {
    while (1) {
        int item_id = produce_item();

        pthread_mutex_lock(&itemArr_mutex);
        while (count == N) pthread_cond_wait(&p_cond, &itemArr_mutex);  // If buffer is full, wait on cv

        int pos = insert_item(item_id);

        count++;

        pthread_cond_signal(&c_cond);  // If buffer has any element on it signal any consumer that might be waiting
        pthread_mutex_unlock(&itemArr_mutex);

        printf("inserted item %d at pos %d\n", item_id, pos);
    }
    pthread_exit(0);
}

void *consumidor(void *pi) {
    while (1) {
        pthread_mutex_lock(&itemArr_mutex);
        while (count == 0) pthread_cond_wait(&c_cond, &itemArr_mutex);  // If buffer is empty, wait on cv

        Item item = consume_item();

        count--;

        pthread_cond_signal(&p_cond);  // If buffer has minus one element, signal any producer that might be waiting
        pthread_mutex_unlock(&itemArr_mutex);

        printf("consumed item %d at pos %d\n", item.val, item.pos);
    }
    pthread_exit(0);
}
