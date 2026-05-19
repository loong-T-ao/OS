#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 128
#define NAME_LEN 32
#define LOG_LEN 128
#define MAX_LOGS 1024

typedef struct {
    char name[NAME_LEN];
    int arrival_time;
    int burst_time;
    int priority;
    int original_index;
} Process;

typedef struct {
    char name[NAME_LEN];
    int arrival_time;
    int burst_time;
    int priority;
    int start_time;
    int finish_time;
    int turnaround_time;
    double weighted_turnaround_time;
} ProcessResult;

typedef struct {
    char algorithm[32];
    char logs[MAX_LOGS][LOG_LEN];
    int log_count;
    ProcessResult results[MAX_PROCESSES];
    int result_count;
    double average_turnaround_time;
    double average_weighted_turnaround_time;
} ScheduleResult;

static void copy_processes(const Process *src, Process *dst, int count) {
    memcpy(dst, src, sizeof(Process) * (size_t)count);
}

static int compare_arrival_then_index(const void *a, const void *b) {
    const Process *left = (const Process *)a;
    const Process *right = (const Process *)b;
    if (left->arrival_time != right->arrival_time) {
        return left->arrival_time - right->arrival_time;
    }
    return left->original_index - right->original_index;
}

static int compare_name_result(const void *a, const void *b) {
    const ProcessResult *left = (const ProcessResult *)a;
    const ProcessResult *right = (const ProcessResult *)b;
    return strcmp(left->name, right->name);
}

static void append_log(ScheduleResult *result, int start, int end, const char *name) {
    if (result->log_count >= MAX_LOGS) {
        return;
    }
    snprintf(
        result->logs[result->log_count],
        LOG_LEN,
        "time %d -> %d: %.31s",
        start,
        end,
        name
    );
    result->log_count += 1;
}

static void build_result(
    ScheduleResult *result,
    const char *algorithm,
    const Process *processes,
    int count,
    const int *start_times,
    const int *finish_times
) {
    int i;
    double total_turnaround = 0.0;
    double total_weighted = 0.0;

    memset(result->algorithm, 0, sizeof(result->algorithm));
    snprintf(result->algorithm, sizeof(result->algorithm), "%s", algorithm);
    result->result_count = count;

    for (i = 0; i < count; ++i) {
        ProcessResult *item = &result->results[i];
        int turnaround = finish_times[i] - processes[i].arrival_time;
        double weighted = (double)turnaround / (double)processes[i].burst_time;

        snprintf(item->name, NAME_LEN, "%s", processes[i].name);
        item->arrival_time = processes[i].arrival_time;
        item->burst_time = processes[i].burst_time;
        item->priority = processes[i].priority;
        item->start_time = start_times[i];
        item->finish_time = finish_times[i];
        item->turnaround_time = turnaround;
        item->weighted_turnaround_time = weighted;

        total_turnaround += (double)turnaround;
        total_weighted += weighted;
    }

    qsort(result->results, (size_t)count, sizeof(ProcessResult), compare_name_result);

    result->average_turnaround_time = count > 0 ? total_turnaround / (double)count : 0.0;
    result->average_weighted_turnaround_time = count > 0 ? total_weighted / (double)count : 0.0;
}

static void schedule_fcfs(const Process *processes, int count, ScheduleResult *result) {
    Process ordered[MAX_PROCESSES];
    int start_times[MAX_PROCESSES] = {0};
    int finish_times[MAX_PROCESSES] = {0};
    int current_time = 0;
    int i;

    memset(result, 0, sizeof(ScheduleResult));
    copy_processes(processes, ordered, count);
    qsort(ordered, (size_t)count, sizeof(Process), compare_arrival_then_index);

    for (i = 0; i < count; ++i) {
        int index = ordered[i].original_index;
        if (current_time < ordered[i].arrival_time) {
            current_time = ordered[i].arrival_time;
        }
        start_times[index] = current_time;
        append_log(result, current_time, current_time + ordered[i].burst_time, ordered[i].name);
        current_time += ordered[i].burst_time;
        finish_times[index] = current_time;
    }

    build_result(result, "FCFS", processes, count, start_times, finish_times);
}

static void schedule_sjf(const Process *processes, int count, ScheduleResult *result) {
    int done[MAX_PROCESSES] = {0};
    int start_times[MAX_PROCESSES] = {0};
    int finish_times[MAX_PROCESSES] = {0};
    int completed = 0;
    int current_time = 0;

    memset(result, 0, sizeof(ScheduleResult));

    while (completed < count) {
        int selected = -1;
        int i;

        for (i = 0; i < count; ++i) {
            if (done[i] || processes[i].arrival_time > current_time) {
                continue;
            }
            if (selected == -1 ||
                processes[i].burst_time < processes[selected].burst_time ||
                (processes[i].burst_time == processes[selected].burst_time &&
                 processes[i].arrival_time < processes[selected].arrival_time) ||
                (processes[i].burst_time == processes[selected].burst_time &&
                 processes[i].arrival_time == processes[selected].arrival_time &&
                 processes[i].original_index < processes[selected].original_index)) {
                selected = i;
            }
        }

        if (selected == -1) {
            int next_arrival = -1;
            for (i = 0; i < count; ++i) {
                if (done[i]) {
                    continue;
                }
                if (next_arrival == -1 || processes[i].arrival_time < next_arrival) {
                    next_arrival = processes[i].arrival_time;
                }
            }
            current_time = next_arrival;
            continue;
        }

        start_times[selected] = current_time;
        append_log(result, current_time, current_time + processes[selected].burst_time, processes[selected].name);
        current_time += processes[selected].burst_time;
        finish_times[selected] = current_time;
        done[selected] = 1;
        completed += 1;
    }

    build_result(result, "SJF", processes, count, start_times, finish_times);
}

static void schedule_priority(const Process *processes, int count, ScheduleResult *result) {
    int done[MAX_PROCESSES] = {0};
    int start_times[MAX_PROCESSES] = {0};
    int finish_times[MAX_PROCESSES] = {0};
    int completed = 0;
    int current_time = 0;

    memset(result, 0, sizeof(ScheduleResult));

    while (completed < count) {
        int selected = -1;
        int i;

        for (i = 0; i < count; ++i) {
            if (done[i] || processes[i].arrival_time > current_time) {
                continue;
            }
            if (selected == -1 ||
                processes[i].priority < processes[selected].priority ||
                (processes[i].priority == processes[selected].priority &&
                 processes[i].arrival_time < processes[selected].arrival_time) ||
                (processes[i].priority == processes[selected].priority &&
                 processes[i].arrival_time == processes[selected].arrival_time &&
                 processes[i].original_index < processes[selected].original_index)) {
                selected = i;
            }
        }

        if (selected == -1) {
            int next_arrival = -1;
            for (i = 0; i < count; ++i) {
                if (done[i]) {
                    continue;
                }
                if (next_arrival == -1 || processes[i].arrival_time < next_arrival) {
                    next_arrival = processes[i].arrival_time;
                }
            }
            current_time = next_arrival;
            continue;
        }

        start_times[selected] = current_time;
        append_log(result, current_time, current_time + processes[selected].burst_time, processes[selected].name);
        current_time += processes[selected].burst_time;
        finish_times[selected] = current_time;
        done[selected] = 1;
        completed += 1;
    }

    build_result(result, "Priority", processes, count, start_times, finish_times);
}

static void schedule_rr(const Process *processes, int count, int time_slice, ScheduleResult *result) {
    Process ordered[MAX_PROCESSES];
    int remaining[MAX_PROCESSES] = {0};
    int start_times[MAX_PROCESSES];
    int finish_times[MAX_PROCESSES] = {0};
    int queue[MAX_LOGS];
    int head = 0;
    int tail = 0;
    int index = 0;
    int current_time = 0;
    int completed = 0;
    int i;

    memset(result, 0, sizeof(ScheduleResult));
    copy_processes(processes, ordered, count);
    qsort(ordered, (size_t)count, sizeof(Process), compare_arrival_then_index);

    for (i = 0; i < count; ++i) {
        remaining[i] = processes[i].burst_time;
        start_times[i] = -1;
    }

    while (completed < count) {
        while (index < count && ordered[index].arrival_time <= current_time) {
            queue[tail++] = ordered[index].original_index;
            index += 1;
        }

        if (head == tail) {
            if (index < count) {
                current_time = ordered[index].arrival_time;
                continue;
            }
            break;
        }

        {
            int selected = queue[head++];
            int run_time;
            int start = current_time;

            if (start_times[selected] == -1) {
                start_times[selected] = current_time;
            }

            run_time = remaining[selected] < time_slice ? remaining[selected] : time_slice;
            current_time += run_time;
            remaining[selected] -= run_time;
            append_log(result, start, current_time, processes[selected].name);

            while (index < count && ordered[index].arrival_time <= current_time) {
                queue[tail++] = ordered[index].original_index;
                index += 1;
            }

            if (remaining[selected] > 0) {
                queue[tail++] = selected;
            } else {
                finish_times[selected] = current_time;
                completed += 1;
            }
        }
    }

    build_result(result, "RR", processes, count, start_times, finish_times);
}

static int load_processes(const char *path, Process *processes) {
    FILE *file = fopen(path, "r");
    int count = 0;

    if (file == NULL) {
        fprintf(stderr, "无法打开输入文件: %s\n", path);
        return -1;
    }

    while (count < MAX_PROCESSES) {
        Process *item = &processes[count];
        int fields = fscanf(
            file,
            "%31s %d %d %d",
            item->name,
            &item->arrival_time,
            &item->burst_time,
            &item->priority
        );

        if (fields == EOF) {
            break;
        }

        if (fields != 4) {
            fprintf(stderr, "输入文件格式错误，第 %d 行解析失败。\n", count + 1);
            fclose(file);
            return -1;
        }

        item->original_index = count;
        count += 1;
    }

    fclose(file);
    return count;
}

static void print_result(const ScheduleResult *result) {
    int i;

    printf("算法: %s\n", result->algorithm);
    printf("执行顺序:\n");
    for (i = 0; i < result->log_count; ++i) {
        printf("  %s\n", result->logs[i]);
    }

    printf("\n");
    printf("进程结果:\n");
    printf("  name  arrival  burst  priority  start  finish  turnaround  weighted_turnaround\n");
    for (i = 0; i < result->result_count; ++i) {
        const ProcessResult *item = &result->results[i];
        printf(
            "  %-5s %-7d %-5d %-8d %-5d %-6d %-10d %.2f\n",
            item->name,
            item->arrival_time,
            item->burst_time,
            item->priority,
            item->start_time,
            item->finish_time,
            item->turnaround_time,
            item->weighted_turnaround_time
        );
    }

    printf("\n");
    printf("平均周转时间: %.2f\n", result->average_turnaround_time);
    printf("平均带权周转时间: %.2f\n", result->average_weighted_turnaround_time);
}

static void print_usage(const char *program) {
    printf("用法: %s -i <input_file> -a <algorithm> [-q time_slice]\n", program);
    printf("算法可选: all | fcfs | sjf | rr | priority\n");
}

int main(int argc, char *argv[]) {
    Process processes[MAX_PROCESSES];
    ScheduleResult result;
    const char *input_file = NULL;
    const char *algorithm = "all";
    int time_slice = 2;
    int count;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            input_file = argv[++i];
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            algorithm = argv[++i];
        } else if (strcmp(argv[i], "-q") == 0 && i + 1 < argc) {
            time_slice = atoi(argv[++i]);
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (input_file == NULL) {
        print_usage(argv[0]);
        return 1;
    }

    if (time_slice <= 0) {
        fprintf(stderr, "时间片必须大于 0。\n");
        return 1;
    }

    count = load_processes(input_file, processes);
    if (count <= 0) {
        return 1;
    }

    if (strcmp(algorithm, "all") == 0 || strcmp(algorithm, "fcfs") == 0) {
        schedule_fcfs(processes, count, &result);
        print_result(&result);
        if (strcmp(algorithm, "all") == 0) {
            printf("\n========================================================================\n\n");
        }
    }

    if (strcmp(algorithm, "all") == 0 || strcmp(algorithm, "sjf") == 0) {
        schedule_sjf(processes, count, &result);
        print_result(&result);
        if (strcmp(algorithm, "all") == 0) {
            printf("\n========================================================================\n\n");
        }
    }

    if (strcmp(algorithm, "all") == 0 || strcmp(algorithm, "rr") == 0) {
        schedule_rr(processes, count, time_slice, &result);
        print_result(&result);
        if (strcmp(algorithm, "all") == 0) {
            printf("\n========================================================================\n\n");
        }
    }

    if (strcmp(algorithm, "all") == 0 || strcmp(algorithm, "priority") == 0) {
        schedule_priority(processes, count, &result);
        print_result(&result);
    }

    if (strcmp(algorithm, "all") != 0 &&
        strcmp(algorithm, "fcfs") != 0 &&
        strcmp(algorithm, "sjf") != 0 &&
        strcmp(algorithm, "rr") != 0 &&
        strcmp(algorithm, "priority") != 0) {
        fprintf(stderr, "不支持的算法: %s\n", algorithm);
        return 1;
    }

    return 0;
}
