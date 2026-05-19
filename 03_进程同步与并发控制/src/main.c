#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_BUFFER 64
#define MAX_THREADS 32
#define MAX_LINE 128

typedef struct {
    int buffer_size;
    int producer_count;
    int consumer_count;
    int items_per_producer;
    int reader_count;
    int writer_count;
    int reads_per_reader;
    int writes_per_writer;
    int philosopher_count;
    int meals_per_philosopher;
} Config;

typedef struct {
    int buffer[MAX_BUFFER];
    int buffer_size;
    int in_index;
    int out_index;
    int count;
    int next_item;
    int total_items;
    int consumed_items;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} ProducerConsumerState;

typedef struct {
    ProducerConsumerState *state;
    int thread_id;
} ProducerConsumerArgs;

typedef struct {
    int shared_value;
    int read_count;
    pthread_mutex_t read_mutex;
    pthread_mutex_t resource_mutex;
} ReaderWriterState;

typedef struct {
    ReaderWriterState *state;
    int thread_id;
    int loops;
} ReaderWriterArgs;

typedef struct {
    int philosopher_count;
    int meals_per_philosopher;
    sem_t forks[MAX_THREADS];
} DiningState;

typedef struct {
    DiningState *state;
    int philosopher_id;
} DiningArgs;

static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

static void sleep_milliseconds(long milliseconds) {
    struct timespec req;
    req.tv_sec = milliseconds / 1000L;
    req.tv_nsec = (milliseconds % 1000L) * 1000000L;
    nanosleep(&req, NULL);
}

static void safe_print(const char *message) {
    pthread_mutex_lock(&print_mutex);
    printf("%s", message);
    fflush(stdout);
    pthread_mutex_unlock(&print_mutex);
}

static void print_separator(void) {
    safe_print("\n========================================================================\n\n");
}

static int load_config(const char *path, Config *config) {
    FILE *file = fopen(path, "r");
    char line[MAX_LINE];

    if (file == NULL) {
        fprintf(stderr, "无法打开配置文件: %s\n", path);
        return 0;
    }

    config->buffer_size = 5;
    config->producer_count = 2;
    config->consumer_count = 2;
    config->items_per_producer = 4;
    config->reader_count = 3;
    config->writer_count = 2;
    config->reads_per_reader = 3;
    config->writes_per_writer = 2;
    config->philosopher_count = 5;
    config->meals_per_philosopher = 2;

    while (fgets(line, sizeof(line), file) != NULL) {
        char key[MAX_LINE];
        int value = 0;

        if (sscanf(line, "%127[^=]=%d", key, &value) != 2) {
            continue;
        }

        if (strcmp(key, "buffer_size") == 0) {
            config->buffer_size = value;
        } else if (strcmp(key, "producer_count") == 0) {
            config->producer_count = value;
        } else if (strcmp(key, "consumer_count") == 0) {
            config->consumer_count = value;
        } else if (strcmp(key, "items_per_producer") == 0) {
            config->items_per_producer = value;
        } else if (strcmp(key, "reader_count") == 0) {
            config->reader_count = value;
        } else if (strcmp(key, "writer_count") == 0) {
            config->writer_count = value;
        } else if (strcmp(key, "reads_per_reader") == 0) {
            config->reads_per_reader = value;
        } else if (strcmp(key, "writes_per_writer") == 0) {
            config->writes_per_writer = value;
        } else if (strcmp(key, "philosopher_count") == 0) {
            config->philosopher_count = value;
        } else if (strcmp(key, "meals_per_philosopher") == 0) {
            config->meals_per_philosopher = value;
        }
    }

    fclose(file);

    if (config->buffer_size <= 0 || config->buffer_size > MAX_BUFFER) {
        fprintf(stderr, "buffer_size 必须在 1 到 %d 之间。\n", MAX_BUFFER);
        return 0;
    }
    if (config->producer_count <= 0 || config->producer_count > MAX_THREADS ||
        config->consumer_count <= 0 || config->consumer_count > MAX_THREADS ||
        config->reader_count <= 0 || config->reader_count > MAX_THREADS ||
        config->writer_count <= 0 || config->writer_count > MAX_THREADS ||
        config->philosopher_count <= 1 || config->philosopher_count > MAX_THREADS) {
        fprintf(stderr, "线程数量配置超出允许范围。\n");
        return 0;
    }
    if (config->items_per_producer <= 0 || config->reads_per_reader <= 0 ||
        config->writes_per_writer <= 0 || config->meals_per_philosopher <= 0) {
        fprintf(stderr, "循环次数配置必须大于 0。\n");
        return 0;
    }

    return 1;
}

static void *producer_thread(void *arg) {
    ProducerConsumerArgs *args = (ProducerConsumerArgs *)arg;
    ProducerConsumerState *state = args->state;

    while (1) {
        int item;
        char message[128];

        pthread_mutex_lock(&state->mutex);
        if (state->next_item >= state->total_items) {
            pthread_mutex_unlock(&state->mutex);
            break;
        }
        item = state->next_item++;
        while (state->count == state->buffer_size) {
            pthread_cond_wait(&state->not_full, &state->mutex);
        }
        state->buffer[state->in_index] = item;
        state->in_index = (state->in_index + 1) % state->buffer_size;
        state->count += 1;
        snprintf(
            message,
            sizeof(message),
            "生产者 %d 生产产品 %d，缓冲区当前数量: %d\n",
            args->thread_id,
            item,
            state->count
        );
        pthread_cond_signal(&state->not_empty);
        pthread_mutex_unlock(&state->mutex);

        safe_print(message);
        sleep_milliseconds(30L);
    }

    return NULL;
}

static void *consumer_thread(void *arg) {
    ProducerConsumerArgs *args = (ProducerConsumerArgs *)arg;
    ProducerConsumerState *state = args->state;

    while (1) {
        int item;
        char message[128];

        pthread_mutex_lock(&state->mutex);
        while (state->count == 0 && state->consumed_items < state->total_items) {
            pthread_cond_wait(&state->not_empty, &state->mutex);
        }

        if (state->consumed_items >= state->total_items && state->count == 0) {
            pthread_mutex_unlock(&state->mutex);
            break;
        }

        item = state->buffer[state->out_index];
        state->out_index = (state->out_index + 1) % state->buffer_size;
        state->count -= 1;
        state->consumed_items += 1;
        snprintf(
            message,
            sizeof(message),
            "消费者 %d 消费产品 %d，缓冲区当前数量: %d\n",
            args->thread_id,
            item,
            state->count
        );
        pthread_cond_signal(&state->not_full);
        if (state->consumed_items >= state->total_items) {
            pthread_cond_broadcast(&state->not_empty);
        }
        pthread_mutex_unlock(&state->mutex);

        safe_print(message);
        sleep_milliseconds(40L);
    }

    return NULL;
}

static void run_producer_consumer(const Config *config) {
    ProducerConsumerState state;
    pthread_t producers[MAX_THREADS];
    pthread_t consumers[MAX_THREADS];
    ProducerConsumerArgs producer_args[MAX_THREADS];
    ProducerConsumerArgs consumer_args[MAX_THREADS];
    int total_items = config->producer_count * config->items_per_producer;
    int i;

    memset(&state, 0, sizeof(state));
    state.buffer_size = config->buffer_size;
    state.total_items = total_items;
    pthread_mutex_init(&state.mutex, NULL);
    pthread_cond_init(&state.not_full, NULL);
    pthread_cond_init(&state.not_empty, NULL);

    safe_print("问题: 生产者-消费者\n");

    for (i = 0; i < config->producer_count; ++i) {
        producer_args[i].state = &state;
        producer_args[i].thread_id = i + 1;
        pthread_create(&producers[i], NULL, producer_thread, &producer_args[i]);
    }

    for (i = 0; i < config->consumer_count; ++i) {
        consumer_args[i].state = &state;
        consumer_args[i].thread_id = i + 1;
        pthread_create(&consumers[i], NULL, consumer_thread, &consumer_args[i]);
    }

    for (i = 0; i < config->producer_count; ++i) {
        pthread_join(producers[i], NULL);
    }
    pthread_mutex_lock(&state.mutex);
    pthread_cond_broadcast(&state.not_empty);
    pthread_mutex_unlock(&state.mutex);
    for (i = 0; i < config->consumer_count; ++i) {
        pthread_join(consumers[i], NULL);
    }

    safe_print("生产者-消费者测试结束。\n");
    pthread_mutex_destroy(&state.mutex);
    pthread_cond_destroy(&state.not_full);
    pthread_cond_destroy(&state.not_empty);
}

static void *reader_thread(void *arg) {
    ReaderWriterArgs *args = (ReaderWriterArgs *)arg;
    ReaderWriterState *state = args->state;
    int i;

    for (i = 0; i < args->loops; ++i) {
        char message[128];

        pthread_mutex_lock(&state->read_mutex);
        state->read_count += 1;
        if (state->read_count == 1) {
            pthread_mutex_lock(&state->resource_mutex);
        }
        pthread_mutex_unlock(&state->read_mutex);

        snprintf(
            message,
            sizeof(message),
            "读者 %d 正在读取，共享数据 = %d\n",
            args->thread_id,
            state->shared_value
        );
        safe_print(message);
        sleep_milliseconds(25L);

        pthread_mutex_lock(&state->read_mutex);
        state->read_count -= 1;
        if (state->read_count == 0) {
            pthread_mutex_unlock(&state->resource_mutex);
        }
        pthread_mutex_unlock(&state->read_mutex);
        sleep_milliseconds(20L);
    }

    return NULL;
}

static void *writer_thread(void *arg) {
    ReaderWriterArgs *args = (ReaderWriterArgs *)arg;
    ReaderWriterState *state = args->state;
    int i;

    for (i = 0; i < args->loops; ++i) {
        char message[128];

        pthread_mutex_lock(&state->resource_mutex);
        state->shared_value += 1;
        snprintf(
            message,
            sizeof(message),
            "写者 %d 正在写入，共享数据更新为 %d\n",
            args->thread_id,
            state->shared_value
        );
        safe_print(message);
        sleep_milliseconds(35L);
        pthread_mutex_unlock(&state->resource_mutex);

        sleep_milliseconds(30L);
    }

    return NULL;
}

static void run_reader_writer(const Config *config) {
    ReaderWriterState state;
    pthread_t readers[MAX_THREADS];
    pthread_t writers[MAX_THREADS];
    ReaderWriterArgs reader_args[MAX_THREADS];
    ReaderWriterArgs writer_args[MAX_THREADS];
    int i;

    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.read_mutex, NULL);
    pthread_mutex_init(&state.resource_mutex, NULL);

    safe_print("问题: 读者-写者\n");

    for (i = 0; i < config->reader_count; ++i) {
        reader_args[i].state = &state;
        reader_args[i].thread_id = i + 1;
        reader_args[i].loops = config->reads_per_reader;
        pthread_create(&readers[i], NULL, reader_thread, &reader_args[i]);
    }

    for (i = 0; i < config->writer_count; ++i) {
        writer_args[i].state = &state;
        writer_args[i].thread_id = i + 1;
        writer_args[i].loops = config->writes_per_writer;
        pthread_create(&writers[i], NULL, writer_thread, &writer_args[i]);
    }

    for (i = 0; i < config->reader_count; ++i) {
        pthread_join(readers[i], NULL);
    }
    for (i = 0; i < config->writer_count; ++i) {
        pthread_join(writers[i], NULL);
    }

    safe_print("读者-写者测试结束。\n");
    pthread_mutex_destroy(&state.read_mutex);
    pthread_mutex_destroy(&state.resource_mutex);
}

static void *philosopher_thread(void *arg) {
    DiningArgs *args = (DiningArgs *)arg;
    DiningState *state = args->state;
    int left = args->philosopher_id;
    int right = (args->philosopher_id + 1) % state->philosopher_count;
    int first = left < right ? left : right;
    int second = left < right ? right : left;
    int meal;

    for (meal = 0; meal < state->meals_per_philosopher; ++meal) {
        char message[128];

        snprintf(message, sizeof(message), "哲学家 %d 正在思考。\n", args->philosopher_id);
        safe_print(message);
        sleep_milliseconds(20L);

        if (sem_wait(&state->forks[first]) != 0) {
            return NULL;
        }
        if (sem_wait(&state->forks[second]) != 0) {
            sem_post(&state->forks[first]);
            return NULL;
        }

        snprintf(message, sizeof(message), "哲学家 %d 正在进餐，第 %d 次。\n", args->philosopher_id, meal + 1);
        safe_print(message);
        sleep_milliseconds(30L);

        sem_post(&state->forks[second]);
        sem_post(&state->forks[first]);
    }

    return NULL;
}

static void run_dining_philosophers(const Config *config) {
    DiningState state;
    pthread_t philosophers[MAX_THREADS];
    DiningArgs args[MAX_THREADS];
    int i;

    state.philosopher_count = config->philosopher_count;
    state.meals_per_philosopher = config->meals_per_philosopher;

    safe_print("问题: 哲学家进餐\n");

    for (i = 0; i < state.philosopher_count; ++i) {
        if (sem_init(&state.forks[i], 0, 1) != 0) {
            fprintf(stderr, "初始化叉子信号量失败。\n");
            for (; i > 0; --i) {
                sem_destroy(&state.forks[i - 1]);
            }
            return;
        }
    }

    for (i = 0; i < state.philosopher_count; ++i) {
        args[i].state = &state;
        args[i].philosopher_id = i;
        pthread_create(&philosophers[i], NULL, philosopher_thread, &args[i]);
    }

    for (i = 0; i < state.philosopher_count; ++i) {
        pthread_join(philosophers[i], NULL);
        sem_destroy(&state.forks[i]);
    }

    safe_print("哲学家进餐测试结束。\n");
}

static void print_usage(const char *program) {
    printf("用法: %s -m <mode> -c <config_file>\n", program);
    printf("mode 可选: all | pc | rw | dp\n");
}

int main(int argc, char *argv[]) {
    const char *mode = "all";
    const char *config_file = NULL;
    Config config;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            mode = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            config_file = argv[++i];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (config_file == NULL || !load_config(config_file, &config)) {
        fprintf(stderr, "需要提供有效的配置文件。\n");
        return 1;
    }

    if (strcmp(mode, "all") == 0 || strcmp(mode, "pc") == 0) {
        run_producer_consumer(&config);
        if (strcmp(mode, "all") == 0) {
            print_separator();
        }
    }

    if (strcmp(mode, "all") == 0 || strcmp(mode, "rw") == 0) {
        run_reader_writer(&config);
        if (strcmp(mode, "all") == 0) {
            print_separator();
        }
    }

    if (strcmp(mode, "all") == 0 || strcmp(mode, "dp") == 0) {
        run_dining_philosophers(&config);
    }

    if (strcmp(mode, "all") != 0 &&
        strcmp(mode, "pc") != 0 &&
        strcmp(mode, "rw") != 0 &&
        strcmp(mode, "dp") != 0) {
        fprintf(stderr, "不支持的模式: %s\n", mode);
        return 1;
    }

    return 0;
}
