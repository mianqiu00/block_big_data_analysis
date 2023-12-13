#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 区块数、交易数、用户数计数
int calc_block = 0;
int calc_transaction = 0;
int calc_user = 0;

// 时钟
clock_t start_time, end_time;

// 哈希表的长度
#define HashTableSize 100000

typedef struct Transaction
{
    int tx_id;
    int blockID;
    char* from;
    char* to;
    double amount;   
    struct Transaction* next;
    struct Transaction* prev;
} Transaction;

// 用于建立邻接图的
typedef struct user
{
    char* user_id;
    struct user* next_user;
    Transaction* out_list_head;
    Transaction* in_list_head;
    double path_length;  // 在最短路径算法中计算目标顶点到该顶点的最短路径
    int in_count;
    int out_count;
    int sign;  // 在最短路径算法中检验是否被使用过该顶点
    int ring_sign; // 用于检查在检查环时有没有包含该节点
} user;

typedef struct user_max
{
    char* user_id;
    double amount;
    struct user_max* next;
    struct user_max* prev;
} user_max;

// 用于建立hash表
typedef struct HashTable
{
    int size;
    user** table;
} HashTable;

typedef struct Block
{
    int blockID;
    char* hash;
    unsigned block_timestamp;
    int transaction_count;
    Transaction* transaction_head;
    struct Block* next;
    struct Block* prev;
} Block;

// 读取csv和建立区块链函数
Block* createLinkedList(HashTable* userTable);
void readBlock(Block* list);
void readTransaction(Block* list, HashTable* user_list);
void add_new_transaction(Block* list, HashTable* user_list, char* file_name);
void insertBlock(Block* list, int blockID, char* hash, unsigned time_stamp);
void insertTransaction(Block* list, int tx_id, int blockID, char* from, double amount, char* to);
Transaction* copy_transaction(Transaction* source);

// 处理用户名单和交易图（hash表、邻接图、逆邻接图）
HashTable* initHashTable(int size);
int hashFunction(char* key, int size);
void insert(HashTable* hashTable, char* key, int sign);
void insert_edge(HashTable* hashTable, int tx_id, int blockID, char* from, double amount, char* to);
void pathHashtable(HashTable* user_table);
void max_in_out(HashTable* user_table, int k);
void wealth_rank(HashTable* user_table, int k);
void free_hashTable(HashTable* HashTable);

// 最短路径相关算法
void check_ring(HashTable* user_table);
void shortest_path(HashTable* user_table, char* from, char* to);
user* find_user(HashTable* user_table, char* key);
void init_path(HashTable* user_table);
int check_ring_by_key(HashTable* user_table, char* from);
void init_ring(HashTable* user_table);

// 查询函数
void account_in_out(unsigned time_start, unsigned time_end, int k, char* account, Block* head);
void account_amount(unsigned time_end, char* account, Block* head);
HashTable* time_wealth_rank(Block* head, unsigned time_stamp, int k);
void data_lookup(Block* head, HashTable* user_table);
void data_analysis(Block* head, HashTable* user_table);
void add_file(Block* head, HashTable* user_table);
void operation(Block* head, HashTable* user_table);

int main()
{
    start_time = clock();
    HashTable* userTable = initHashTable(HashTableSize);
    Block* head = createLinkedList(userTable);

    operation(head, userTable);

    /* 下列注释用于各个函数的快速演示 */
    // add_new_transaction(head, userTable, "tx_data_part2.csv");
    // check_ring(userTable);
    // shortest_path(userTable, "1HNzxjHeAjDJ47KvQdYcsrsPUWhYWrAF4p", "1HxW4owufY8ujzwav7xbVW84FZabRV6rWt");
    // time_wealth_rank(head, 1354268787, 20);
    // wealth_rank(userTable, 50);
    // max_in_out(userTable, 3);
    // pathHashtable(userTable);
    // account_in_out(1284753029, 1358886914, 5, "1Mw6FCSvf81NxkC1B6u8djW1rXMQSv1VTv", head);
    // account_amount(1358886914, "1Mw6FCSvf81NxkC1B6u8djW1rXMQSv1VTv", head);

    return 0;
}

// 去除末尾\n
void removeNewline(char* str)
{
    int len = strlen(str);
    
    // 检查字符串是否为空
    if (len > 0)
    {
        // 检查最后一个字符是否为换行符
        if (str[len - 1] == '\n')
        {
            // 将换行符替换为字符串结束符'\0'
            str[len - 1] = '\0';
        }
    }
}

// 根据csv文件组构建链表
Block* createLinkedList(HashTable* userTable)
{
    Block* head = (Block*)malloc(sizeof(Block));
    head->blockID = 0;
    head->block_timestamp = 0;
    head->hash = 0;
    head->next = head;
    head->prev = head; 

    printf("初始化区块链和交易网络中，请稍等...\n");
    readBlock(head);
    readTransaction(head, userTable);
    end_time = clock();
    printf("区块链和交易网络初始化已完成!\n");
    printf("区块数: %d\n交易数: %d\n用户数: %d\n", calc_block, calc_transaction, calc_user);
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("运行时间: %.3f 秒\n", elapsed_time);

    return head;
}

// 读取区块信息
void readBlock(Block* list)
{
    int blockID;
    char hash_str[64] = {0};
    char* hash = hash_str;
    unsigned time_stamp;

    FILE* file = fopen("block_part1.csv", "r");

    // 逐行读取CSV文件
    char line[1024];
    int lineCount = 0;
    while (fgets(line, sizeof(line), file))
    {
        lineCount++;
        if (lineCount == 1)
        {
            continue;
        }
        // 使用strtok函数拆分CSV行
        char* token = strtok(line, ",");
        if (token == NULL) {
            continue; // 忽略空行
        }

        // 解析CSV列数据
        blockID = atoi(token);
        token = strtok(NULL, ",");
        hash = token;
        token = strtok(NULL, ",");
        time_stamp = (unsigned)atoi(token);
        
        // 调用insertBlock函数将块数据插入
        insertBlock(list, blockID, hash, time_stamp);
    }

    // 关闭文件
    fclose(file);  
}

// 读取交易信息
void readTransaction(Block* list, HashTable* user_list)
{
    int prev_blockID;
    int tx_id;
    int blockID;
    char from_str[34];
    char to_str[34];
    char* from = from_str;
    char* to = to_str;
    double amount;

    prev_blockID = blockID;

    FILE* file = fopen("tx_data_part1_v2.csv", "r");

    // 逐行读取CSV文件
    char line[512];
    int lineCount = 0;
    while (fgets(line, sizeof(line), file))
    {
        lineCount++;
        if (lineCount == 1)
        {
            continue;
        }
        // 使用strtok函数拆分CSV行
        char* token = strtok(line, ",");
        if (token == NULL) {
            continue; // 忽略空行
        }

        // 解析CSV列数据
        tx_id = atoi(token);
        token = strtok(NULL, ",");
        blockID = atoi(token);
        token = strtok(NULL, ",");
        from = token;
        token = strtok(NULL, ",");
        amount = strtod(token, NULL);
        token = strtok(NULL, ",");
        to = token;
        removeNewline(to);

        // 调用函数将块数据插入
        char* from_copy = strdup(from);
        char* to_copy = strdup(to);

        insertTransaction(list, tx_id, blockID, from_copy, amount, to_copy);


        // 插入user
        insert(user_list, from_copy, 1);
        insert(user_list, to_copy, 1);
        insert_edge(user_list, tx_id, blockID, from_copy, amount, to_copy);

        if (prev_blockID != blockID)
        {
            // 指针迁移，降低时间复杂度
            while (list->blockID != blockID)
            {
                list = list->next;
            }
        }
        prev_blockID = blockID;
    }

    // 关闭文件
    fclose(file);
}

// 插入单条区块信息
void insertBlock(Block* list, int blockID, char* hash, unsigned time_stamp)
{
    Block* newblock = (Block*)malloc(sizeof(Block));

    newblock->blockID = blockID;
    newblock->hash = strdup(hash);  //原来要这么复制指针指向的字符串
    newblock->block_timestamp = time_stamp;
    newblock->transaction_count = 0;

    // 初始化交易头结点
    newblock->transaction_head = (Transaction*)malloc(sizeof(Transaction));
    newblock->transaction_head->blockID = blockID;
    newblock->transaction_head->tx_id = 0;
    newblock->transaction_head->next = newblock->transaction_head;
    newblock->transaction_head->prev = newblock->transaction_head;

    // 尾插
    newblock->next = list;
    newblock->prev = list->prev;
    list->prev->next = newblock;
    list->prev = newblock;

    newblock->transaction_count = 0;
    
    calc_block++;
    if (calc_block % 1000 == 0)
    {
        printf("block: %d\n", calc_block);
    }
}

// 插入单条交易信息
void insertTransaction(Block* list, int tx_id, int blockID, char* from, double amount, char* to)
{
    Block* temp_list = list;
    while (temp_list->blockID != blockID)
    {
        temp_list = temp_list->next;
    }

    temp_list->transaction_count++;
    Transaction* newTransaction = (Transaction*)malloc(sizeof(Transaction));
    newTransaction->blockID = blockID;
    newTransaction->tx_id = tx_id;
    newTransaction->amount = amount;
    newTransaction->from = strdup(from);
    newTransaction->to = strdup(to);

    // 插入新的交易到区块上的交易链
    newTransaction->prev = temp_list->transaction_head->prev;
    newTransaction->next = temp_list->transaction_head;
    temp_list->transaction_head->prev->next = newTransaction;
    temp_list->transaction_head->prev = newTransaction;

    calc_transaction++;

    if (calc_transaction % 100000 == 0)
    {
        printf("transaction: %d\n", calc_transaction);
    }
}

// 计算一段时间内账户的交易出入
void account_in_out(unsigned time_start, unsigned time_end, int k, char* account, Block* head)
{
    if (time_start > time_end)
    {
        printf("起始时间必须小于终止时间\n");
        return;
    }

    Block* temp_block = head;
    while (temp_block->block_timestamp < time_start)
    {
        temp_block = temp_block->next;
    }
    // 已找到时间起始的交易区块
    Transaction* transaction_list = (Transaction*)malloc(sizeof(Transaction));
    transaction_list->next = transaction_list;
    transaction_list->prev = transaction_list;
    int transaction_count = 0;
    double transaction_in = 0;
    double transaction_out = 0;
    while (temp_block->block_timestamp <= time_end && temp_block->next != head)
    {
        Transaction* temp_transaction = temp_block->transaction_head->next;

        while (temp_transaction != temp_block->transaction_head)
        {
            if (strcmp(temp_transaction->from, account) == 0 || strcmp(temp_transaction->to, account) == 0)
            {

                if (transaction_list->next == transaction_list)
                {
                    Transaction* transaction_copy = copy_transaction(temp_transaction);

                    transaction_copy->next = transaction_list;
                    transaction_copy->prev = transaction_list;
                    transaction_list->next = transaction_copy;
                    transaction_list->prev = transaction_copy;
                }
                else
                {
                    Transaction* temp_transaction_list = transaction_list;
                    while (temp_transaction_list->next->amount > temp_transaction->amount && temp_transaction_list->next != transaction_list)
                    {
                        temp_transaction_list = temp_transaction_list->next;
                    }

                    Transaction* transaction_copy = copy_transaction(temp_transaction);

                    temp_transaction_list->next->prev = transaction_copy;
                    transaction_copy->next = temp_transaction_list->next;
                    transaction_copy->prev = temp_transaction_list;
                    temp_transaction_list->next = transaction_copy;
                }
                if (strcmp(temp_transaction->from, account) == 0)
                {
                    transaction_out += temp_transaction->amount;
                }
                else
                {
                    transaction_in += temp_transaction->amount;
                }
                transaction_count++;
            }
            temp_transaction = temp_transaction->next;
        }
        // 更新查找区块
        temp_block = temp_block->next;
    }
    printf("总交易数: %d\n", transaction_count);
    printf("总支出: %.2lf\n", transaction_out);
    printf("总收入: %.2lf\n", transaction_in);
    printf("交易金额最大的%d笔交易:\n", k);
    Transaction* temp_list = transaction_list->next;
    for (int i = 0; i < k && temp_list != transaction_list; i++)
    {
        printf("txid: %d\nblockID: %d\nadd_in: %s\nadd_out: %s\namount: %.2lf\n\n", 
        temp_list->tx_id, temp_list->blockID, temp_list->from, temp_list->to, temp_list->amount);
        temp_list = temp_list->next;
        if (temp_list == transaction_list) break;
    }
    // printf("\n");
}

// 复制交易节点
Transaction* copy_transaction(Transaction* source)
{  
    Transaction* transaction_copy = (Transaction*)malloc(sizeof(Transaction));
    transaction_copy->amount = source->amount;
    transaction_copy->blockID = source->blockID;
    transaction_copy->tx_id = source->tx_id;
    transaction_copy->from = strdup(source->from);
    transaction_copy->to = strdup(source->to);
    return transaction_copy;
}

// 统计结余
void account_amount(unsigned time_end, char* account, Block* head)
{
    Block* temp_block = head->next;

    // 已找到时间起始的交易区块
    int transaction_count = 0;
    double transaction_in = 0;
    double transaction_out = 0;
    while (temp_block->block_timestamp <= time_end && temp_block->next != head)
    {
        Transaction* temp_transaction = temp_block->transaction_head->next;
        while (temp_transaction != temp_block->transaction_head)
        {
            if (strcmp(temp_transaction->from, account) == 0)
            {
                transaction_out += temp_transaction->amount;
                transaction_count++;      
            }
            else if (strcmp(temp_transaction->to, account) == 0)
            {
                transaction_in += temp_transaction->amount;
                transaction_count++;
            }
            temp_transaction = temp_transaction->next;
        }
        // 更新查找区块
        temp_block = temp_block->next;
    }
    printf("总交易数: %d\n", transaction_count);
    printf("总支出: %.2lf\n", transaction_out);
    printf("总收入: %.2lf\n", transaction_in);
    printf("结余: %.2lf\n", transaction_in - transaction_out);
}

// 初始化哈希表
HashTable* initHashTable(int size)
{
    HashTable* hashTable = (HashTable*)malloc(sizeof(HashTable));
    hashTable->size = size;
    hashTable->table = (user**)malloc(sizeof(user*) * size);

    for (int i = 0; i < size; i++)
    {
        hashTable->table[i] = (user*)malloc(sizeof(user));
        hashTable->table[i]->next_user = 0;
    }

    return hashTable;
}

// 哈希函数
int hashFunction(char* key, int size)
{
    unsigned int hash = 5381; // 一个常用的初始哈希值
    int c;

    while ((c = *key++))
    {
        hash = ((hash << 5) + hash) + c; // 乘以33并加上字符的ASCII值
    }

    return hash % size;
}

// 插入操作
void insert(HashTable* hashTable, char* key, int sign)
{
    int index = hashFunction(key, hashTable->size);
    user* new_user = (user*)malloc(sizeof(user));
    new_user->user_id = key;
    new_user->in_count = 0;
    new_user->out_count = 0;
    new_user->in_list_head = (Transaction*)malloc(sizeof(Transaction));
    new_user->out_list_head = (Transaction*)malloc(sizeof(Transaction));
    new_user->in_list_head->next = 0;
    new_user->out_list_head->next = 0;
    new_user->in_list_head->amount = 0;
    new_user->out_list_head->amount = 0;

    user* temp_user = hashTable->table[index]->next_user;
    while (temp_user != 0)
    {
        if (strcmp(temp_user->user_id, new_user->user_id) == 0)
        {
            return;
        }
        temp_user = temp_user->next_user;
    }
    new_user->next_user = hashTable->table[index]->next_user;
    hashTable->table[index]->next_user = new_user;

    if (sign == 1)
    {
        calc_user++;
    }

    if (calc_user % 100000 == 0)
    {
        printf("insert user: %d\n", calc_user);
    }
}

// 将交易插入邻接图和逆邻接图
void insert_edge(HashTable* hashTable, int tx_id, int blockID, char* from, double amount, char* to)
{
    Transaction* new_transaction_in = (Transaction*)malloc(sizeof(Transaction));
    new_transaction_in->amount = amount;
    new_transaction_in->blockID = blockID;
    new_transaction_in->tx_id = tx_id;
    new_transaction_in->from = strdup(from);
    new_transaction_in->to = strdup(to);
    Transaction* new_transaction_out = copy_transaction(new_transaction_in);
    
    int index_from = hashFunction(from, hashTable->size);
    int index_to = hashFunction(to, hashTable->size);
    user* temp_user = hashTable->table[index_from]->next_user;
    while (temp_user != 0)
    {
        if (strcmp(temp_user->user_id, from) == 0)
        {
            new_transaction_in->next = temp_user->out_list_head->next;
            temp_user->out_list_head->next = new_transaction_in;
            temp_user->out_count++;
            temp_user->out_list_head->amount += new_transaction_in->amount;
            break;
        }
        temp_user = temp_user->next_user;
    }
    temp_user = hashTable->table[index_to]->next_user;
    while (temp_user != 0)
    {
        if (strcmp(temp_user->user_id, to) == 0)
        {
            new_transaction_out->next = temp_user->in_list_head->next;
            temp_user->in_list_head->next = new_transaction_out;
            temp_user->in_count++;
            temp_user->in_list_head->amount += new_transaction_out->amount;
            break;
        }
        temp_user = temp_user->next_user;
    }
}

// 遍历哈希表并统计平均入度出度
void pathHashtable(HashTable* user_table)
{
    int index;
    int total_in = 0;
    int total_out = 0;
    double total_in_amount = 0;
    double total_out_amount = 0;

    for (index = 0; index < user_table->size; index++)
    {
        user* temp_user = user_table->table[index];
        if (temp_user->next_user == 0)
        {
            continue;
        }
        temp_user = temp_user->next_user;
        while (temp_user != 0)
        {
            total_in += temp_user->in_count;
            total_out += temp_user->in_count;
            total_in_amount += temp_user->in_list_head->amount;
            total_out_amount += temp_user->out_list_head->amount;
            temp_user = temp_user->next_user;
        }
    }
    double average_in, average_out, average_in_amount, average_out_amount;
    average_in = ((double)total_in) / ((double)calc_user);
    average_out = ((double)total_out) / ((double)calc_user);
    average_out_amount = total_out_amount / ((double)calc_user);
    average_in_amount = total_in_amount / ((double)calc_user);

    printf("平均入度为: %.2lf\n平均出度为: %.2lf\n加权平均入度为: %.2lf\n加权平均出度为: %.2lf\n", 
    average_in, average_out, average_in_amount, average_out_amount);
}

// 遍历哈希表并统计最大出度和最大入度
void max_in_out(HashTable* user_table, int k)
{
    int index;
    user_max* max_in = (user_max*)malloc(sizeof(user));
    max_in->amount = 0;
    max_in->next = 0;
    max_in->user_id = "";
    user_max* max_out = (user_max*)malloc(sizeof(user));
    max_out->amount = 0;
    max_out->next = 0;
    max_out->user_id = "";
    user_max* max_in_amount = (user_max*)malloc(sizeof(user));
    max_in_amount->amount = 0;
    max_in_amount->next = 0;
    max_in_amount->user_id = "";
    user_max* max_out_amount = (user_max*)malloc(sizeof(user));
    max_out_amount->amount = 0;
    max_out_amount->next = 0;
    max_out_amount->user_id = "";

    for (index = 0; index < user_table->size; index++)
    {
        user* temp_user = user_table->table[index];
        if (temp_user->next_user == 0)
        {
            continue;
        }
        temp_user = temp_user->next_user;
        while (temp_user != 0)
        {
            int i = 0;
            user_max* temp;

            user_max* user_in = (user_max*)malloc(sizeof(user_max));
            user_in->user_id = strdup(temp_user->user_id);
            user_in->amount = (double)temp_user->in_count;
            temp = max_in;
            for (i = 0; i < k; i++)
            {
                if (temp->next == 0)
                {
                    user_in->next = temp->next;
                    temp->next = user_in;
                    break;
                }
                if (temp->next->amount < temp_user->in_count)
                {
                    user_in->next = temp->next;
                    temp->next = user_in;
                    break;
                }
                temp = temp->next;
            }

            user_max* user_out = (user_max*)malloc(sizeof(user_max));
            user_out->user_id = strdup(temp_user->user_id);
            user_out->amount = (double)temp_user->out_count;
            temp = max_out;
            for (i = 0; i < k; i++)
            {
                if (temp->next == 0)
                {
                    user_out->next = temp->next;
                    temp->next = user_out;
                    break;
                }
                if (temp->next->amount < temp_user->out_count)
                {
                    user_out->next = temp->next;
                    temp->next = user_out;
                    break;
                }
                temp = temp->next;
            }

            user_max* user_in_amount = (user_max*)malloc(sizeof(user_max));
            user_in_amount->user_id = strdup(temp_user->user_id);
            user_in_amount->amount = temp_user->in_list_head->amount;
            temp = max_in_amount;
            for (i = 0; i < k; i++)
            {
                if (temp->next == 0)
                {
                    user_in_amount->next = temp->next;
                    temp->next = user_in_amount;
                    break;
                }
                if (temp->next->amount < temp_user->in_list_head->amount)
                {
                    user_in_amount->next = temp->next;
                    temp->next = user_in_amount;
                    break;
                }
                temp = temp->next;
            }

            user_max* user_out_amount = (user_max*)malloc(sizeof(user_max));
            user_out_amount->user_id = strdup(temp_user->user_id);
            user_out_amount->amount = temp_user->out_list_head->amount;
            temp = max_out_amount;
            for (i = 0; i < k; i++)
            {
                if (temp->next == 0)
                {
                    user_out_amount->next = temp->next;
                    temp->next = user_out_amount;
                    break;
                }
                if (temp->next->amount < temp_user->out_list_head->amount)
                {
                    user_out_amount->next = temp->next;
                    temp->next = user_out_amount;
                    break;
                }
                temp = temp->next;
            }

            temp_user = temp_user->next_user;
        }
    }

    user_max* temp;

    temp = max_in;
    printf("入度排行前%d名\n", k);
    for (int i = 0; i < k; i++)
    {
        temp = temp->next;
        printf("入度 NO.%d: %s, %d\n", i + 1, temp->user_id, (int)temp->amount);
        if (temp == 0)
        {
            break;
        }
    }

    temp = max_out;
    printf("出度排行前%d名\n", k);
    for (int i = 0; i < k; i++)
    {
        temp = temp->next;
        printf("出度 NO.%d: %s, %d\n", i + 1, temp->user_id, (int)temp->amount);
        if (temp == 0)
        {
            break;
        }
    }

    temp = max_in_amount;
    printf("加权入度排行前%d名\n", k);
    for (int i = 0; i < k; i++)
    {
        temp = temp->next;
        printf("加权入度 NO.%d: %s, %.2lf\n", i + 1, temp->user_id, temp->amount);
        if (temp == 0)
        {
            break;
        }
    }

    temp = max_out_amount;
    printf("加权出度排行前%d名\n", k);
    for (int i = 0; i < k; i++)
    {
        temp = temp->next;
        printf("加权出度 NO.%d: %s, %.2lf\n", i + 1, temp->user_id, temp->amount);
        if (temp == 0)
        {
            break;
        }
    }
}

// 计算财富排行
void wealth_rank(HashTable* user_table, int k)
{
    int index;
    user_max* max_wealth = (user_max*)malloc(sizeof(user));
    max_wealth->amount = 0;
    max_wealth->next = 0;
    max_wealth->user_id = "";

    for (index = 0; index < user_table->size; index++)
    {
        user* temp_user = user_table->table[index];
        if (temp_user->next_user == 0)
        {
            continue;
        }
        temp_user = temp_user->next_user;
        while (temp_user != 0)
        {
            int i = 0;
            user_max* temp;

            user_max* user_wealth = (user_max*)malloc(sizeof(user_max));
            user_wealth->user_id = strdup(temp_user->user_id);
            double temp_user_wealth = temp_user->in_list_head->amount - temp_user->out_list_head->amount;
            user_wealth->amount = temp_user_wealth;
            temp = max_wealth;
            for (i = 0; i < k; i++)
            {
                if (temp->next == 0)
                {
                    user_wealth->next = temp->next;
                    temp->next = user_wealth;
                    break;
                }
                if (temp->next->amount < temp_user_wealth)
                {
                    user_wealth->next = temp->next;
                    temp->next = user_wealth;
                    break;
                }
                temp = temp->next;
            }

            temp_user = temp_user->next_user;
        }
    }

    user_max* temp;

    temp = max_wealth;
    printf("财富排行前%d名\n", k);
    for (int i = 0; i < k; i++)
    {
        temp = temp->next;
        printf("财富 NO.%d: %s, %.2lf\n", i + 1, temp->user_id, temp->amount);
        if (temp == 0)
        {
            break;
        }
    }
}

// 在某时间上的交易网络
HashTable* time_wealth_rank(Block* head, unsigned time_stamp, int k)
{
    HashTable* user_table = initHashTable(10000);
    
    Block* temp_block = head->next;
    while (temp_block->block_timestamp <= time_stamp)
    {
        Transaction* temp_transaction = temp_block->transaction_head->next;
        while (temp_transaction->next != temp_block->transaction_head)
        {
            insert(user_table, temp_transaction->from, 0);
            insert(user_table, temp_transaction->to, 0);
            insert_edge(user_table, temp_transaction->tx_id, temp_transaction->blockID, 
            temp_transaction->from, temp_transaction->amount, temp_transaction->to);
            
            temp_transaction = temp_transaction->next;
        }
        
        temp_block = temp_block->next;
    }

    wealth_rank(user_table, k);
    free_hashTable(user_table);
}

// 释放哈希表内存
void free_hashTable(HashTable* HashTable)
{
    for (int i = 0; i < HashTable->size; i++)
    {
        user* temp_user = HashTable->table[i]->next_user;
        user* free_user = HashTable->table[i];
        free(free_user);

        do
        {
            Transaction* temp_transaction = temp_user->in_list_head;
            Transaction* free_transaction;
            do
            {
                free_transaction = temp_transaction;
                temp_transaction = temp_transaction->next;
                free(free_transaction);
            } while (temp_transaction != 0);

            temp_transaction = temp_user->out_list_head;
            do
            {
                free_transaction = temp_transaction;
                temp_transaction = temp_transaction->next;
                free(free_transaction);
            } while (temp_transaction != 0);
            

            free_user = temp_user;
            temp_user = temp_user->next_user;
            free(free_user);
        } while (temp_user->next_user != 0);
    }
    
    free(HashTable);
}

// 检查交易网络是否有环
void check_ring(HashTable* user_table)
{
    init_ring(user_table);
    int calc = 0;

    for (int i = 0; i < user_table->size; i++)
    {
        if (user_table->table[i]->next_user == 0)
        {
            continue;
        }
        user* temp_user = user_table->table[i]->next_user;
        while (temp_user != 0)
        {
            // 进入user的遍历层
            if (temp_user->in_count != 0 && temp_user->out_count != 0)
            {
                int status = check_ring_by_key(user_table, temp_user->user_id);
                if (status == 1)
                {
                    printf("YES\n（交易网络中存在环）\n");
                    return;
                }
            }
            
            calc++;
            if (calc % 10000 == 0)
            {
                printf("unring-user: %d\n", calc);
            }

            temp_user = temp_user->next_user;
        }
    }
    printf("NO\n（交易网络中不存在环）\n");
}

// 对于单个user检查是否成环
int check_ring_by_key(HashTable* user_table, char* from)
{
    init_path(user_table);

    user* head_user = find_user(user_table, from);
    head_user->sign = -1;  // 首节点的sign设置-1，已参加计算的节点是1，未参加是0
    
    user_max* contained_user = (user_max*)malloc(sizeof(user_max));
    contained_user->user_id = strdup(from);
    contained_user->next = contained_user;
    contained_user->prev = contained_user;


    // 寻径函数
    user_max* temp_user_brief = contained_user;
    user* temp_user = find_user(user_table, temp_user_brief->user_id);

    do
    {
        Transaction* temp_transaction = temp_user->out_list_head->next;
        while (temp_transaction != 0)
        {
            user* next_user = find_user(user_table, temp_transaction->to);
            
            if (next_user->sign == 0 && next_user->out_count != 0)
            {
                next_user->sign = 1;
                next_user->ring_sign = 1;
                
                user_max* new_user = (user_max*)malloc(sizeof(user_max));
                new_user->user_id = strdup(next_user->user_id);
                temp_user_brief->prev->next = new_user;
                new_user->prev = temp_user_brief->prev;
                new_user->next = temp_user_brief;
                temp_user_brief->prev = new_user;

            }
            else if (next_user->sign == -1)
            {
                user_max* current_p = contained_user;
                if (current_p->next = current_p)
                {
                    free(current_p);
                }
                else
                {
                    current_p = current_p->next;
                    user_max* prev_p = current_p;
                    while(current_p != contained_user)
                    {
                        current_p = current_p->next;
                        free(prev_p);
                        prev_p = current_p;
                    }
                    free(prev_p);
                }
                return 1;
            }

            temp_transaction = temp_transaction->next;
        }

        temp_user_brief = temp_user_brief->next;
        temp_user = find_user(user_table, temp_user_brief->user_id);
    } while (temp_user_brief != contained_user);


    user_max* current_p = contained_user;
    if (current_p->next = current_p)
    {
        free(current_p);
    }
    else
    {
        current_p = current_p->next;
        user_max* prev_p = current_p;
        while(current_p != contained_user)
        {
            current_p = current_p->next;
            free(prev_p);
            prev_p = current_p;
        }
        free(prev_p);
    }

    return 0;
}

// 计算两个节点之间的最短路径
void shortest_path(HashTable* user_table, char* from, char* to)
{
    init_path(user_table);

    user* head_user = find_user(user_table, from);
    head_user->sign = -1;  // 首节点的sign设置-1，已参加计算的节点是1，未参加是0
    
    // 寻径函数
    int update_calc = 1;
    
    while(update_calc)
    {
        update_calc = 0;

        for (int i = 0; i < user_table->size; i++)
        {
            if (user_table->table[i]->next_user == 0)
            {
                continue;
            }
            user* temp_user = user_table->table[i]->next_user;
            while (temp_user != 0)
            {
                // 进入user的遍历层
                if (temp_user->sign == 0)
                {
                    temp_user = temp_user->next_user;
                    continue;
                }

                Transaction* temp_transaction = temp_user->out_list_head->next;
                while (temp_transaction != 0)
                {
                    user* next_user = find_user(user_table, temp_transaction->to);
                    if (next_user->sign == 0)
                    {
                        next_user->path_length = temp_transaction->amount + temp_user->path_length;
                        next_user->sign = 1;
                        update_calc++;
                    }
                    else if (next_user->sign != -1)
                    {
                        double new_path_length = temp_transaction->amount + temp_user->path_length;
                        if (next_user->path_length > new_path_length)
                        {
                            next_user->path_length = new_path_length;
                            update_calc++;
                        }
                    }

                    temp_transaction = temp_transaction->next;
                }

                temp_user = temp_user->next_user;
            }
        }

        // printf("update: %d\n", update_calc);
    }

    user* target_user = find_user(user_table, to);
    if (target_user->path_length != 0)
    {
        printf("用户: %s\n到\n用户: %s\n最短路径为: %.2lf\n", from, to, target_user->path_length);
    }
    else
    {
        printf("用户: %s\n到\n用户: %s\n不存在路径\n", from, to);
    }
}

// 在哈希表中找到user
user* find_user(HashTable* user_table, char* key)
{
    int index = hashFunction(key, user_table->size);
    user* temp_user = user_table->table[index]->next_user;
    while (strcmp(temp_user->user_id, key) != 0)
    {
        temp_user = temp_user->next_user;
    }
    return temp_user;
}

// 将哈希表中user的sign和path_length初始化为0
void init_path(HashTable* user_table)
{
    for (int i = 0; i < user_table->size; i++)
    {
        if (user_table->table[i]->next_user == 0)
        {
            continue;
        }
        user* temp_user = user_table->table[i]->next_user;
        while (temp_user != 0)
        {
            temp_user->sign = 0;
            temp_user->path_length = 0;
            temp_user = temp_user->next_user;
        }
    }
}

// 将哈希表中user的ring_sing初始化为0
void init_ring(HashTable* user_table)
{
    for (int i = 0; i < user_table->size; i++)
    {
        if (user_table->table[i]->next_user == 0)
        {
            continue;
        }
        user* temp_user = user_table->table[i]->next_user;
        while (temp_user != 0)
        {
            temp_user->ring_sign = 0;
            temp_user = temp_user->next_user;
        }
    }
}

// 增加新的交易
void add_new_transaction(Block* list, HashTable* user_list, char* file_name)
{
    printf("更新区块链和交易网络中，请稍等...\n");

    int prev_blockID;
    int tx_id;
    int blockID;
    char from_str[34];
    char to_str[34];
    char* from = from_str;
    char* to = to_str;
    double amount;

    prev_blockID = blockID;

    FILE* file = fopen(file_name, "r");

    // 逐行读取CSV文件
    char line[512];
    int lineCount = 0;
    while (fgets(line, sizeof(line), file))
    {
        lineCount++;
        if (lineCount == 1)
        {
            continue;
        }
        // 使用strtok函数拆分CSV行
        char* token = strtok(line, ",");
        if (token == NULL) {
            continue; // 忽略空行
        }

        // 解析CSV列数据
        tx_id = atoi(token);
        token = strtok(NULL, ",");
        blockID = atoi(token);
        token = strtok(NULL, ",");
        from = token;
        token = strtok(NULL, ",");
        amount = strtod(token, NULL);
        token = strtok(NULL, ",");
        to = token;
        removeNewline(to);

        // 调用函数将块数据插入
        char* from_copy = strdup(from);
        char* to_copy = strdup(to);

        insertTransaction(list, tx_id, blockID, from_copy, amount, to_copy);


        // 插入user
        insert(user_list, from_copy, 1);
        insert(user_list, to_copy, 1);
        insert_edge(user_list, tx_id, blockID, from_copy, amount, to_copy);

        if (prev_blockID != blockID)
        {
            // 指针迁移，降低时间复杂度
            while (list->blockID != blockID)
            {
                list = list->next;
            }
        }
        prev_blockID = blockID;
    }

    // 关闭文件
    fclose(file);

    printf("区块链和交易网络更新已完成!\n");
    printf("区块数: %d\n交易数: %d\n用户数: %d\n", calc_block, calc_transaction, calc_user);
    
    end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("运行时间: %.3f 秒\n", elapsed_time);
}

// 数据查询界面操作
void data_lookup(Block* head, HashTable* user_table)
{
    int operator;
    while (1)
    {
        operator = 0;
        printf("请输入需要进行的查询操作: \n  0: 返回上一级操作\n");
        printf("  1: 查找指定账号在一个时间段内的所有转入或转出记录，返回总记录数，交易金额最大的前k条记录\n");
        printf("  2: 查询某个账号在某个时刻的金额\n");
        printf("  3: 在某个时刻的福布斯富豪榜!\n     输出在该时刻最有钱的前k个用户\n\n");
        scanf("%d", &operator);
        if (operator == 0)
        {
            break;
        }
        else if (operator == 1)
        {
            char user_id[50];
            int k;
            unsigned start, end;
            printf("请输入账号: \n");
            scanf("%s", user_id);
            printf("请输入k: \n");
            scanf("%d", &k);
            printf("请输入开始时间: \n");
            scanf("%u", &start);
            printf("请输入结束时间: \n");
            scanf("%u", &end);
            start_time = clock();
            account_in_out(start, end, k, user_id, head);
            end_time = clock();
            double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            printf("运行时间: %.3f 秒\n\n", elapsed_time);
        }
        else if (operator == 2)
        {
            char user_id[50];
            unsigned end;
            printf("请输入账号: \n");
            scanf("%s", user_id);
            printf("请输入时间: \n");
            scanf("%u", &end);
            start_time = clock();
            account_amount(end, user_id, head);
            end_time = clock();
            double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            printf("运行时间: %.3f 秒\n\n", elapsed_time);
        }
        else if (operator == 3)
        {
            unsigned end;
            int k;
            printf("请输入k: \n");
            scanf("%d", &k);
            printf("请输入时间: \n");
            scanf("%u", &end);
            start_time = clock();
            time_wealth_rank(head, end, k);
            end_time = clock();
            double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            printf("运行时间: %.3f 秒\n\n", elapsed_time);
        }
        else
        {
            printf("请输入正确的操作指令...\n");
        }
    }
}

// 数据分析界面操作
void data_analysis(Block* head, HashTable* user_table)
{
    int operator;
    while (1)
    {
        operator = 0;
        printf("请输入需要进行的分析操作: \n  0: 返回上一级操作\n");
        printf("  1: 构建交易关系图\n");
        printf("  2: 统计交易关系图的平均出度、入度，显示出度 / 入度最高的前k个帐号\n");
        printf("  3: 检查交易关系图中是否存在环（首次计算时间复杂度高）\n");
        printf("  4: 给定一个账号A，求A到账号B的最短路径\n\n");
        scanf("%d", &operator);
        if (operator == 0)
        {
            break;
        }
        else if (operator == 1)
        {
            printf("交易关系图构建已完成\n");
        }
        else if (operator == 2)
        {
            int k;
            pathHashtable(user_table);
            printf("输入k值: \n");
            scanf("%d", &k);
            start_time = clock();
            max_in_out(user_table, k);
            end_time = clock();
            double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            printf("运行时间: %.3f 秒\n\n", elapsed_time);
        }
        else if (operator == 3)
        {
            start_time = clock();
            check_ring(user_table);
            end_time = clock();
            double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            printf("运行时间: %.3f 秒\n\n", elapsed_time);
        }
        else if (operator == 4)
        {
            char user_from[50];
            char user_to[50];
            printf("输入账号A: \n");
            scanf("%s", user_from);
            printf("输入账号B: \n");
            scanf("%s", user_to);
            start_time = clock();
            shortest_path(user_table, user_from, user_to);
            end_time = clock();
            double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            printf("运行时间: %.3f 秒\n\n", elapsed_time);
        }
        else
        {
            printf("请输入正确的操作指令...\n");
        }
    }
}

// 根据文件添加交易
void add_file(Block* head, HashTable* user_table)
{
    printf("输入用于添加交易的文件: \n");
    char file_direction[50]; // = "tx_data_part2.csv"; // 默认tx_data_part2.csv
    scanf("%s", file_direction);
    start_time = clock();

    FILE* file = fopen(file_direction, "r");
    // 检查文件是否成功打开
    if (file != NULL)
    {
        fclose(file);
        add_new_transaction(head, user_table, file_direction);
    }
    else
    {
        printf("文件不存在\n");
    }
}

// 用户操作主界面
void operation(Block* head, HashTable* user_table)
{
int operator;
    while (1)
    {
        operator = 0;
        printf("请输入你要进行的操作: \n  0: 退出系统\n  1: 数据初始化\n  2: 数据查询\n  3: 数据分析\n  4. 数据插入\n\n");
        scanf("%d", &operator);
        if (operator == 0)
        {
            break;
        }
        else if (operator == 1)
        {
            printf("数据初始化已完成!\n");
        }
        else if (operator == 2)
        {
            // 数据查询
            data_lookup(head, user_table);
        }
        else if (operator == 3)
        {
            // 数据分析
            data_analysis(head, user_table);
        }
        else if (operator == 4)
        {
            // 数据插入
            add_file(head, user_table);
        }
        else
        {
            printf("请输入正确的操作指令...\n");
        }
    }
}

