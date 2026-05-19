#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PAGES 256
#define MAX_BLOCKS 256
#define MAX_NAME_LEN 32
#define MAX_OPS 512

typedef struct {
    int frame_count;
    int pages[MAX_PAGES];
    int page_count;
} PageInput;

typedef struct {
    int start;
    int size;
    int is_free;
    char owner[MAX_NAME_LEN];
} MemoryBlock;

typedef struct {
    int total_size;
    char lines[MAX_OPS][64];
    int line_count;
} PartitionInput;

static void print_separator(void) {
    printf("\n========================================================================\n\n");
}

static int load_page_input(const char *path, PageInput *input) {
    FILE *file = fopen(path, "r");
    int value;

    if (file == NULL) {
        fprintf(stderr, "无法打开页面置换输入文件: %s\n", path);
        return 0;
    }

    if (fscanf(file, "%d", &input->frame_count) != 1 || input->frame_count <= 0) {
        fprintf(stderr, "页面置换输入文件格式错误：页框数量无效。\n");
        fclose(file);
        return 0;
    }

    input->page_count = 0;
    while (input->page_count < MAX_PAGES && fscanf(file, "%d", &value) == 1) {
        input->pages[input->page_count++] = value;
    }

    fclose(file);

    if (input->page_count == 0) {
        fprintf(stderr, "页面置换输入文件中没有访问序列。\n");
        return 0;
    }

    return 1;
}

static int load_partition_input(const char *path, PartitionInput *input) {
    FILE *file = fopen(path, "r");
    char buffer[64];

    if (file == NULL) {
        fprintf(stderr, "无法打开动态分区输入文件: %s\n", path);
        return 0;
    }

    if (fscanf(file, "%d", &input->total_size) != 1 || input->total_size <= 0) {
        fprintf(stderr, "动态分区输入文件格式错误：总内存大小无效。\n");
        fclose(file);
        return 0;
    }

    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fclose(file);
        fprintf(stderr, "动态分区输入文件缺少操作内容。\n");
        return 0;
    }
    input->line_count = 0;

    while (input->line_count < MAX_OPS && fgets(buffer, sizeof(buffer), file) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        if (buffer[0] == '\0') {
            continue;
        }
        snprintf(input->lines[input->line_count], sizeof(input->lines[input->line_count]), "%s", buffer);
        input->line_count += 1;
    }

    fclose(file);
    return 1;
}

static void print_frames(const int *frames, int frame_count) {
    int i;
    printf("  页框: [");
    for (i = 0; i < frame_count; ++i) {
        if (frames[i] == -1) {
            printf("-");
        } else {
            printf("%d", frames[i]);
        }
        if (i != frame_count - 1) {
            printf(" ");
        }
    }
    printf("]\n");
}

static void simulate_fifo(const PageInput *input) {
    int frames[MAX_PAGES];
    int next_replace = 0;
    int faults = 0;
    int i;

    for (i = 0; i < input->frame_count; ++i) {
        frames[i] = -1;
    }

    printf("算法: FIFO 页面置换\n");
    printf("页框数: %d\n", input->frame_count);

    for (i = 0; i < input->page_count; ++i) {
        int page = input->pages[i];
        int hit = 0;
        int j;

        printf("访问页面 %d:\n", page);
        for (j = 0; j < input->frame_count; ++j) {
            if (frames[j] == page) {
                hit = 1;
                break;
            }
        }

        if (hit) {
            printf("  命中，无需置换。\n");
        } else {
            int empty_index = -1;
            faults += 1;
            for (j = 0; j < input->frame_count; ++j) {
                if (frames[j] == -1) {
                    empty_index = j;
                    break;
                }
            }

            if (empty_index != -1) {
                frames[empty_index] = page;
                printf("  缺页，装入空闲页框 %d。\n", empty_index);
            } else {
                printf("  缺页，淘汰页框 %d 中的页面 %d。\n", next_replace, frames[next_replace]);
                frames[next_replace] = page;
                next_replace = (next_replace + 1) % input->frame_count;
            }
        }

        print_frames(frames, input->frame_count);
    }

    printf("\n缺页次数: %d\n", faults);
    printf("缺页率: %.2f%%\n", (double)faults * 100.0 / (double)input->page_count);
}

static int find_lru_victim(const int *last_used, int frame_count) {
    int victim = 0;
    int i;
    for (i = 1; i < frame_count; ++i) {
        if (last_used[i] < last_used[victim]) {
            victim = i;
        }
    }
    return victim;
}

static void simulate_lru(const PageInput *input) {
    int frames[MAX_PAGES];
    int last_used[MAX_PAGES];
    int faults = 0;
    int i;

    for (i = 0; i < input->frame_count; ++i) {
        frames[i] = -1;
        last_used[i] = -1;
    }

    printf("算法: LRU 页面置换\n");
    printf("页框数: %d\n", input->frame_count);

    for (i = 0; i < input->page_count; ++i) {
        int page = input->pages[i];
        int hit = 0;
        int j;

        printf("访问页面 %d:\n", page);

        for (j = 0; j < input->frame_count; ++j) {
            if (frames[j] == page) {
                hit = 1;
                last_used[j] = i;
                break;
            }
        }

        if (hit) {
            printf("  命中，更新最近使用时间。\n");
        } else {
            int empty_index = -1;
            int victim;
            faults += 1;

            for (j = 0; j < input->frame_count; ++j) {
                if (frames[j] == -1) {
                    empty_index = j;
                    break;
                }
            }

            if (empty_index != -1) {
                frames[empty_index] = page;
                last_used[empty_index] = i;
                printf("  缺页，装入空闲页框 %d。\n", empty_index);
            } else {
                victim = find_lru_victim(last_used, input->frame_count);
                printf("  缺页，淘汰最近最久未使用页面 %d。\n", frames[victim]);
                frames[victim] = page;
                last_used[victim] = i;
            }
        }

        print_frames(frames, input->frame_count);
    }

    printf("\n缺页次数: %d\n", faults);
    printf("缺页率: %.2f%%\n", (double)faults * 100.0 / (double)input->page_count);
}

static void print_blocks(const MemoryBlock *blocks, int block_count) {
    int i;
    printf("当前分区状态:\n");
    for (i = 0; i < block_count; ++i) {
        printf(
            "  [%d] start=%d size=%d status=%s owner=%s\n",
            i,
            blocks[i].start,
            blocks[i].size,
            blocks[i].is_free ? "free" : "used",
            blocks[i].is_free ? "-" : blocks[i].owner
        );
    }
}

static void merge_blocks(MemoryBlock *blocks, int *block_count) {
    int i = 0;
    while (i < *block_count - 1) {
        if (blocks[i].is_free && blocks[i + 1].is_free) {
            blocks[i].size += blocks[i + 1].size;
            memmove(&blocks[i + 1], &blocks[i + 2], sizeof(MemoryBlock) * (size_t)(*block_count - i - 2));
            *block_count -= 1;
        } else {
            i += 1;
        }
    }
}

static int allocate_block(
    MemoryBlock *blocks,
    int *block_count,
    const char *owner,
    int size,
    int use_best_fit
) {
    int selected = -1;
    int i;

    for (i = 0; i < *block_count; ++i) {
        if (!blocks[i].is_free || blocks[i].size < size) {
            continue;
        }

        if (!use_best_fit) {
            selected = i;
            break;
        }

        if (selected == -1 || blocks[i].size < blocks[selected].size) {
            selected = i;
        }
    }

    if (selected == -1) {
        return 0;
    }

    if (blocks[selected].size == size) {
        blocks[selected].is_free = 0;
        snprintf(blocks[selected].owner, MAX_NAME_LEN, "%s", owner);
        return 1;
    }

    if (*block_count >= MAX_BLOCKS) {
        return 0;
    }

    memmove(&blocks[selected + 1], &blocks[selected], sizeof(MemoryBlock) * (size_t)(*block_count - selected));
    blocks[selected + 1].start = blocks[selected].start + size;
    blocks[selected + 1].size = blocks[selected].size - size;
    blocks[selected + 1].is_free = 1;
    snprintf(blocks[selected + 1].owner, MAX_NAME_LEN, "-");

    blocks[selected].size = size;
    blocks[selected].is_free = 0;
    snprintf(blocks[selected].owner, MAX_NAME_LEN, "%s", owner);
    *block_count += 1;
    return 1;
}

static int free_block(MemoryBlock *blocks, int *block_count, const char *owner) {
    int i;
    for (i = 0; i < *block_count; ++i) {
        if (!blocks[i].is_free && strcmp(blocks[i].owner, owner) == 0) {
            blocks[i].is_free = 1;
            snprintf(blocks[i].owner, MAX_NAME_LEN, "-");
            merge_blocks(blocks, block_count);
            return 1;
        }
    }
    return 0;
}

static void simulate_partition(const PartitionInput *input, int use_best_fit) {
    MemoryBlock blocks[MAX_BLOCKS];
    int block_count = 1;
    int i;

    blocks[0].start = 0;
    blocks[0].size = input->total_size;
    blocks[0].is_free = 1;
    snprintf(blocks[0].owner, MAX_NAME_LEN, "-");

    printf("算法: %s 动态分区分配\n", use_best_fit ? "BF" : "FF");
    printf("总内存大小: %d\n", input->total_size);

    for (i = 0; i < input->line_count; ++i) {
        char action[16];
        char owner[MAX_NAME_LEN];
        int size = 0;
        int matched = sscanf(input->lines[i], "%15s %31s %d", action, owner, &size);

        printf("操作: %s\n", input->lines[i]);

        if (matched == 3 && strcmp(action, "alloc") == 0) {
            if (allocate_block(blocks, &block_count, owner, size, use_best_fit)) {
                printf("  分配成功: %s 占用 %d 单位内存。\n", owner, size);
            } else {
                printf("  分配失败: 没有足够的连续空闲分区。\n");
            }
        } else if (matched >= 2 && strcmp(action, "free") == 0) {
            if (free_block(blocks, &block_count, owner)) {
                printf("  释放成功: %s 的分区已回收并尝试合并。\n", owner);
            } else {
                printf("  释放失败: 未找到进程 %s 的分区。\n", owner);
            }
        } else {
            printf("  非法操作，已跳过。\n");
        }

        print_blocks(blocks, block_count);
    }
}

static void print_usage(const char *program) {
    printf("用法: %s -m <mode> [-p page_input] [-d partition_input]\n", program);
    printf("mode 可选: all | paging | partition\n");
}

int main(int argc, char *argv[]) {
    const char *mode = "all";
    const char *page_file = NULL;
    const char *partition_file = NULL;
    PageInput page_input;
    PartitionInput partition_input;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            mode = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            page_file = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            partition_file = argv[++i];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (strcmp(mode, "all") == 0 || strcmp(mode, "paging") == 0) {
        if (page_file == NULL || !load_page_input(page_file, &page_input)) {
            fprintf(stderr, "页面置换模式需要有效的 -p 输入文件。\n");
            return 1;
        }

        simulate_fifo(&page_input);
        print_separator();
        simulate_lru(&page_input);
    }

    if (strcmp(mode, "all") == 0 || strcmp(mode, "partition") == 0) {
        if (strcmp(mode, "all") == 0) {
            print_separator();
        }

        if (partition_file == NULL || !load_partition_input(partition_file, &partition_input)) {
            fprintf(stderr, "动态分区模式需要有效的 -d 输入文件。\n");
            return 1;
        }

        simulate_partition(&partition_input, 0);
        print_separator();
        simulate_partition(&partition_input, 1);
    }

    if (strcmp(mode, "all") != 0 &&
        strcmp(mode, "paging") != 0 &&
        strcmp(mode, "partition") != 0) {
        fprintf(stderr, "不支持的模式: %s\n", mode);
        return 1;
    }

    return 0;
}
