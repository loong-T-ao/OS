#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 32
#define MAX_CONTENT_LEN 256
#define MAX_CHILDREN 32
#define MAX_BLOCKS 64
#define BLOCK_SIZE 8
#define MAX_LINE_LEN 512

typedef struct Node {
    char name[MAX_NAME_LEN];
    int is_directory;
    struct Node *parent;
    struct Node *children[MAX_CHILDREN];
    int child_count;
    char content[MAX_CONTENT_LEN];
    int size;
    int blocks[MAX_BLOCKS];
    int block_count;
} Node;

static int block_used[MAX_BLOCKS];

static Node *create_node(const char *name, int is_directory, Node *parent) {
    Node *node = (Node *)calloc(1U, sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "内存分配失败。\n");
        return NULL;
    }

    snprintf(node->name, sizeof(node->name), "%s", name);
    node->is_directory = is_directory;
    node->parent = parent;
    node->size = 0;
    node->block_count = 0;
    return node;
}

static Node *find_child(Node *directory, const char *name) {
    int i;
    for (i = 0; i < directory->child_count; ++i) {
        if (strcmp(directory->children[i]->name, name) == 0) {
            return directory->children[i];
        }
    }
    return NULL;
}

static int add_child(Node *parent, Node *child) {
    if (parent->child_count >= MAX_CHILDREN) {
        return 0;
    }
    parent->children[parent->child_count++] = child;
    return 1;
}

static void remove_child(Node *parent, int index) {
    int i;
    for (i = index; i < parent->child_count - 1; ++i) {
        parent->children[i] = parent->children[i + 1];
    }
    parent->child_count -= 1;
}

static void free_blocks(Node *file) {
    int i;
    for (i = 0; i < file->block_count; ++i) {
        int block_id = file->blocks[i];
        if (block_id >= 0 && block_id < MAX_BLOCKS) {
            block_used[block_id] = 0;
        }
        file->blocks[i] = -1;
    }
    file->block_count = 0;
}

static int allocate_blocks(Node *file, int required_blocks) {
    int i;
    int allocated = 0;

    free_blocks(file);

    for (i = 0; i < MAX_BLOCKS && allocated < required_blocks; ++i) {
        if (!block_used[i]) {
            block_used[i] = 1;
            file->blocks[allocated++] = i;
        }
    }

    if (allocated < required_blocks) {
        int j;
        for (j = 0; j < allocated; ++j) {
            block_used[file->blocks[j]] = 0;
            file->blocks[j] = -1;
        }
        file->block_count = 0;
        return 0;
    }

    file->block_count = allocated;
    return 1;
}

static void destroy_node(Node *node) {
    int i;
    if (node == NULL) {
        return;
    }

    for (i = 0; i < node->child_count; ++i) {
        destroy_node(node->children[i]);
    }

    if (!node->is_directory && node->block_count > 0) {
        free_blocks(node);
    }

    free(node);
}

static void list_directory(Node *current) {
    int i;
    printf("当前目录内容:\n");
    if (current->child_count == 0) {
        printf("  (empty)\n");
        return;
    }

    for (i = 0; i < current->child_count; ++i) {
        Node *child = current->children[i];
        printf(
            "  %s %s\n",
            child->is_directory ? "[DIR]" : "[FILE]",
            child->name
        );
    }
}

static void print_tree(Node *node, int depth) {
    int i;
    for (i = 0; i < depth; ++i) {
        printf("  ");
    }
    printf("%s%s\n", node->name, node->is_directory ? "/" : "");
    for (i = 0; i < node->child_count; ++i) {
        print_tree(node->children[i], depth + 1);
    }
}

static void print_status(Node *root) {
    int used_count = 0;
    int i;

    for (i = 0; i < MAX_BLOCKS; ++i) {
        if (block_used[i]) {
            used_count += 1;
        }
    }

    printf("文件系统状态:\n");
    printf("  总块数: %d\n", MAX_BLOCKS);
    printf("  已使用块数: %d\n", used_count);
    printf("  空闲块数: %d\n", MAX_BLOCKS - used_count);
    printf("  目录树:\n");
    print_tree(root, 1);
}

static Node *resolve_file(Node *root, Node *current, const char *path, Node **parent_out, int *child_index_out) {
    char buffer[MAX_LINE_LEN];
    char *token;
    Node *node = path[0] == '/' ? root : current;
    Node *parent = NULL;
    int child_index = -1;

    snprintf(buffer, sizeof(buffer), "%s", path);
    token = strtok(buffer, "/");

    while (token != NULL) {
        int i;
        if (strcmp(token, ".") == 0) {
            token = strtok(NULL, "/");
            continue;
        }
        if (strcmp(token, "..") == 0) {
            if (node->parent != NULL) {
                node = node->parent;
            }
            token = strtok(NULL, "/");
            continue;
        }

        parent = node;
        child_index = -1;
        node = NULL;
        for (i = 0; i < parent->child_count; ++i) {
            if (strcmp(parent->children[i]->name, token) == 0) {
                node = parent->children[i];
                child_index = i;
                break;
            }
        }
        if (node == NULL) {
            return NULL;
        }
        token = strtok(NULL, "/");
    }

    if (parent_out != NULL) {
        *parent_out = parent;
    }
    if (child_index_out != NULL) {
        *child_index_out = child_index;
    }
    return node;
}

static void command_mkdir(Node *current, const char *name) {
    Node *node;
    if (find_child(current, name) != NULL) {
        printf("mkdir 失败: %s 已存在。\n", name);
        return;
    }

    node = create_node(name, 1, current);
    if (node == NULL || !add_child(current, node)) {
        destroy_node(node);
        printf("mkdir 失败: 无法创建目录 %s。\n", name);
        return;
    }

    printf("mkdir 成功: 创建目录 %s。\n", name);
}

static void command_create(Node *current, const char *name) {
    Node *node;
    if (find_child(current, name) != NULL) {
        printf("create 失败: %s 已存在。\n", name);
        return;
    }

    node = create_node(name, 0, current);
    if (node == NULL || !add_child(current, node)) {
        destroy_node(node);
        printf("create 失败: 无法创建文件 %s。\n", name);
        return;
    }

    printf("create 成功: 创建文件 %s。\n", name);
}

static void command_write(Node *current, const char *name, const char *content) {
    Node *file = find_child(current, name);
    int content_len;
    int required_blocks;

    if (file == NULL || file->is_directory) {
        printf("write 失败: 文件 %s 不存在。\n", name);
        return;
    }

    content_len = (int)strlen(content);
    if (content_len >= MAX_CONTENT_LEN) {
        printf("write 失败: 内容过长。\n");
        return;
    }

    required_blocks = (content_len + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (required_blocks == 0) {
        required_blocks = 1;
    }

    if (!allocate_blocks(file, required_blocks)) {
        printf("write 失败: 空闲块不足。\n");
        return;
    }

    snprintf(file->content, sizeof(file->content), "%s", content);
    file->size = content_len;
    printf("write 成功: 文件 %s 写入 %d 字节，占用 %d 个块。\n", name, file->size, file->block_count);
}

static void command_read(Node *current, const char *name) {
    Node *file = find_child(current, name);
    int i;

    if (file == NULL || file->is_directory) {
        printf("read 失败: 文件 %s 不存在。\n", name);
        return;
    }

    printf("read 成功: %s -> %s\n", name, file->content);
    printf("  占用块: ");
    for (i = 0; i < file->block_count; ++i) {
        printf("%d", file->blocks[i]);
        if (i != file->block_count - 1) {
            printf(" ");
        }
    }
    printf("\n");
}

static void command_delete(Node *root, Node *current, const char *path) {
    Node *parent = NULL;
    Node *target;
    int index = -1;

    target = resolve_file(root, current, path, &parent, &index);
    if (target == NULL || target->is_directory) {
        printf("delete 失败: 文件 %s 不存在。\n", path);
        return;
    }

    if (parent != NULL && index >= 0) {
        remove_child(parent, index);
    }
    destroy_node(target);
    printf("delete 成功: 文件 %s 已删除并回收块。\n", path);
}

static Node *command_cd(Node *root, Node *current, const char *path) {
    Node *target;
    if (strcmp(path, "..") == 0) {
        if (current->parent != NULL) {
            printf("cd 成功: 返回上级目录 %s。\n", current->parent->name);
            return current->parent;
        }
        printf("cd 提示: 当前已在根目录。\n");
        return current;
    }

    target = resolve_file(root, current, path, NULL, NULL);
    if (target == NULL || !target->is_directory) {
        printf("cd 失败: 目录 %s 不存在。\n", path);
        return current;
    }

    printf("cd 成功: 进入目录 %s。\n", target->name);
    return target;
}

static void initialize_blocks(void) {
    int i;
    for (i = 0; i < MAX_BLOCKS; ++i) {
        block_used[i] = 0;
    }
}

static void execute_commands(const char *path) {
    FILE *file = fopen(path, "r");
    char line[MAX_LINE_LEN];
    Node *root;
    Node *current;

    if (file == NULL) {
        fprintf(stderr, "无法打开命令文件: %s\n", path);
        return;
    }

    initialize_blocks();
    root = create_node("root", 1, NULL);
    if (root == NULL) {
        fclose(file);
        return;
    }
    current = root;

    while (fgets(line, sizeof(line), file) != NULL) {
        char command[32];
        char arg1[MAX_LINE_LEN];
        char arg2[MAX_CONTENT_LEN];
        int parsed;

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (line[0] == '\0') {
            continue;
        }

        printf("命令: %s\n", line);
        parsed = sscanf(line, "%31s %511s %255[^\n]", command, arg1, arg2);

        if (parsed >= 2 && strcmp(command, "mkdir") == 0) {
            command_mkdir(current, arg1);
        } else if (parsed >= 2 && strcmp(command, "cd") == 0) {
            current = command_cd(root, current, arg1);
        } else if (parsed >= 2 && strcmp(command, "create") == 0) {
            command_create(current, arg1);
        } else if (parsed >= 3 && strcmp(command, "write") == 0) {
            command_write(current, arg1, arg2);
        } else if (parsed >= 2 && strcmp(command, "read") == 0) {
            command_read(current, arg1);
        } else if (parsed >= 2 && strcmp(command, "delete") == 0) {
            command_delete(root, current, arg1);
        } else if (parsed >= 1 && strcmp(command, "ls") == 0) {
            list_directory(current);
        } else if (parsed >= 1 && strcmp(command, "status") == 0) {
            print_status(root);
        } else {
            printf("非法命令或参数不完整。\n");
        }

        printf("\n");
    }

    destroy_node(root);
    fclose(file);
}

static void print_usage(const char *program) {
    printf("用法: %s -i <command_file>\n", program);
}

int main(int argc, char *argv[]) {
    const char *input_file = NULL;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            input_file = argv[++i];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (input_file == NULL) {
        print_usage(argv[0]);
        return 1;
    }

    execute_commands(input_file);
    return 0;
}
