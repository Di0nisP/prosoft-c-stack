#include "cstack.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

//#define UNUSED(VAR) (void)(VAR)

/**
 * @struct node
 * @brief Узел стека, 
 * реализованного как односвязный список с гибким массивом данных.
 *
 * Каждый элемент стека содержит: @n 
 * @arg указатель на предыдущий элемент,
 * @arg размер хранимых данных,
 * @arg сам блок данных произвольного размера, 
 * размещённый сразу после структуры в памяти.
 * 
 * @see stack_push
 * @see stack_pop
 */
struct node
{
    /**
     * @brief Указатель на предыдущий элемент стека.
     *
     * Используется для реализации связности между элементами.
     * Является константным, чтобы предотвратить изменение связей после создания узла.
     *
     * @ref stack_t
     */
    const struct node* prev;

    /**
     * @brief Размер хранимых данных в байтах.
     *
     * Используется для корректной обработки содержимого @ref node::data .
     */
    unsigned int size;

    /**
     * @brief Гибкий массив данных.
     *
     * Позволяет хранить данные произвольного размера сразу после структуры в памяти.
     * При выделении памяти необходимо учитывать выражение:
     * @code
     * sizeof(struct node) + size
     * @endcode
     */
    char data[0];
};

typedef struct node* stack_t;

/**
 * @struct stack_entry
 * @brief Элемент (слот) таблицы стеков.
 */
struct stack_entry
{
    /**
     * @brief Флаг занятости слота таблицы стеков.
     *
     * Указывает, используется ли данный элемент таблицы для активного стека.
     * Значение @c 1 означает, что слот занят и связан с валидным хэндлером.
     * Значение @c 0 означает, что слот свободен и может быть повторно выделен.
     *
     * Это поле не влияет на содержимое самого стека, но определяет,
     * можно ли обращаться к нему через хэндлер.
     */
    int reserved;

    /**
     * @brief Хэндлер стека.
     *
     * Указывает на вершину стека, связанного с данным элементом таблицы.
     * 
     * @ref stack_t
     */
    stack_t stack;
};

typedef struct stack_entry stack_entry_t;

/**
 * @struct stack_entries_table
 * @brief Таблица стеков.
 *
 * Хранит массив элементов @ref stack_entry_t и его текущий размер.
 */
struct stack_entries_table
{
    /**
     * @brief Количество элементов в таблице.
     *
     * Определяет размер массива @ref stack_entries_table::entries .
     */
    unsigned int size;

    /**
     * @brief Указатель на массив элементов таблицы.
     *
     * Каждый элемент представляет отдельный стек.
     * 
     * @ref stack_entry_t
     */
    stack_entry_t* entries;
};

struct stack_entries_table g_table = {0u, NULL};

// Константы для начального размера таблицы и роста
enum { TABLE_INITIAL_SIZE = 4 }; // compile-time int, без объекта

/**
 * @brief Расширить таблицу стеков до заданного минимального размера.
 * 
 * Гарантирует, что глобальная таблица @c g_table содержит по крайней мере
 * @p min_size элементов. При необходимости увеличивает ёмкость таблицы
 * методом роста в 2 раза, выделяя новый блок через @c realloc .
 *
 * Поведение: @n
 * @arg Если текущая ёмкость @c g_table.size >= @p min_size , 
 * функция немедленно возвращает 0 и не изменяет таблицу.
 * @arg Иначе вычисляет новое значение ёмкости как минимум @p min_size ,
 * увеличивая текущее значение в степени двух 
 * (начиная с @c TABLE_INITIAL_SIZE когда таблица ещё пуста).
 * @arg Пытается выполнить @c realloc буфера @c g_table.entries до нового размера.
 * @arg При успешном @c realloc инициализирует только вновь добавленные слоты:
 * устанавливает @c reserved @c = @c 0 и @c stack @c = @c NULL 
 * для индексов @c [old_size, @c new_size) .
 * @arg Обновляет поля @c g_table.entries и @c g_table.size только после
 * успешного выделения памяти.
 *
 * @param[in] min_size Минимально требуемое число слотов в таблице.
 *
 * @return @c  0 при успешном обеспечении требуемой ёмкости.
 * @return @c -1 при ошибке выделения памяти; в этом случае @c g_table
 * остаётся в прежнем состоянии и требует обработку ошибки вызывающим кодом.
 * 
 * Инварианты: @n
 * @arg После успешного возврата гарантируется, что
 * @c (g_table.size @c >= @c min_size) и 
 * @c (g_table.entries @c != @c NULL @c && @c g_table.size @c > @c 0).
 * @arg Вне зависимости от результата старый буфер остаётся валидным до тех пор,
 * пока не выполнится присваивание @c g_table.entries @c = @c new_entries
 * (т.е. при неуспехе @c realloc исходная память не теряется).
 *
 * Побочные эффекты: @n
 * @arg Может изменить значение @c g_table.entries и @c g_table.size (только при успехе).
 * @arg Инициализирует новые элементы таблицы как свободные.
 */
static int table_ensure_capacity(const unsigned int min_size)
{
    if (g_table.size >= min_size)
        return 0;

    unsigned int new_size = (g_table.size == 0) ? TABLE_INITIAL_SIZE : g_table.size;
    while (new_size < min_size)
        new_size *= 2;

    stack_entry_t* const new_entries = (stack_entry_t*)realloc(g_table.entries, new_size * sizeof(stack_entry_t));
    if (!new_entries)
        return -1;

    // Инициализировать новые слоты как свободные
    for (unsigned int i = g_table.size; i < new_size; ++i) {
        new_entries[i].reserved = 0;
        new_entries[i].stack = NULL;
    }

    g_table.entries = new_entries;
    g_table.size = new_size;
    return 0;
}

/**
 * @brief Выделить свободный слот в таблице стеков.
 *
 * Функция ищет первый свободный слот в глобальной таблице @c g_table.entries ,
 * где поле @c (reserved @c == @c 0). 
 * Если такой слот найден - возвращает его индекс.
 * Если свободных слотов нет, функция пытается расширить таблицу до размера
 * @c (g_table.size @c + @c 1) с помощью @ref table_ensure_capacity .
 *
 * После успешного расширения таблицы, функция повторно ищет свободный слот
 * среди новых элементов. Возвращает индекс первого найденного свободного слота.
 *
 * Поведение: @n
 * @arg При наличии свободного слота — возвращает его индекс.
 * @arg При отсутствии — расширяет таблицу и ищет среди новых слотов.
 * @arg При ошибке выделения памяти — возвращает @c -1.
 *
 * Безопасность: @n
 * @arg Проверяет, что @c g_table.entries @c != @c NULL перед доступом.
 * @arg Не теряет старый указатель при ошибке realloc.
 * @arg Гарантирует, что возвращаемый индекс всегда валиден при успехе.
 *
 * @return Индекс свободного слота (>= 0) при успехе.
 * @return @c -1 при ошибке.
 * 
 * @see table_ensure_capacity
 */
static int table_alloc_slot(void)
{
    // Поиск существующего свободного слота
    if (g_table.entries != NULL) {
        for (unsigned int i = 0; i < g_table.size; ++i) {
            if (g_table.entries[i].reserved == 0)
                return (int)i;
        }
    }

    // Cвободного слота нет -> расширить таблицу минимум до (g_table.size + 1)
    const unsigned int old_size = g_table.size;
    if (table_ensure_capacity(old_size + 1) != 0)
        return -1;

    // g_table.entries != NULL

    // Поиск существующего свободного слота
    for (unsigned int i = old_size; i < g_table.size; ++i) {
        if (g_table.entries[i].reserved == 0)
            return (int)i;
    }

    return -1;
}

hstack_t stack_new(void)
{
    // Найти слот
    const int slot = table_alloc_slot();
    if (slot < 0)
        return -1; // Ошибка памяти (Out Of Memory) или другая ошибка

    // Инициализировать слот как занятый
    g_table.entries[slot].reserved = 1; // Слот занят
    g_table.entries[slot].stack = NULL; // Стек пуст

    // Вернуть индекс слота как hstack_t
    return (hstack_t)slot;
}

void stack_free(const hstack_t hstack)
{
    if (stack_valid_handler(hstack)) return;

    stack_entry_t* const entry = &g_table.entries[hstack];

    // Освободить узлы стека
    {
        stack_t current = entry->stack;
        while (current) {
            stack_t prev = (stack_t)current->prev;
            free(current);
            current = prev;
        }
    }
    
    // Обнулить слот
    entry->stack = NULL;
    entry->reserved = 0;
}

int stack_valid_handler(const hstack_t hstack)
{
    if (hstack < 0 || (unsigned int)hstack >= g_table.size)
        return 1;

    if (g_table.entries == NULL)
        return 1;

    if (g_table.entries[hstack].reserved == 0)
        return 1;

    return 0;
}

unsigned int stack_size(const hstack_t hstack)
{
    if (stack_valid_handler(hstack)) 
        return 0u;

    stack_entry_t* const entry = &g_table.entries[hstack];
    if (entry == NULL || entry->stack == NULL)
        return 0u;

    unsigned int count = 0u;
    stack_t current = entry->stack;
    while (current) {
        count++;
        current = (stack_t)current->prev;
    }

    return count;
}

void stack_push(const hstack_t hstack, const void* data_in, const unsigned int size)
{
    /// @todo Добавить предупреждение о причине выхода из функции @ref stack_push
    if (stack_valid_handler(hstack) || data_in == NULL || size == 0)
        return;

    stack_entry_t* const entry = &g_table.entries[hstack];

    // Выделить память под узел
    stack_t node = (stack_t)malloc(sizeof(struct node) + size);
    if (!node)
        return; // Ошибка памяти (Out Of Memory)

    // Копировать данные в область сразу после структуры
    void* payload = (void*)(node + 1);  // Указатель на "обезличенные" данные
    memcpy(payload, data_in, size); 

    // Инизиализировать узел стека
    node->size = size;
    node->prev = entry->stack;

    // Обновить вершину
    entry->stack = node;
}

unsigned int stack_pop(const hstack_t hstack, void* data_out, const unsigned int size)
{
    /// @todo Добавить предупреждение о причине выхода из функции @ref stack_pop
    if (stack_valid_handler(hstack) || data_out == NULL || size == 0)
        return 0u;

    stack_entry_t* const entry = &g_table.entries[hstack];

    stack_t node = entry->stack;
    if (!node)
        return 0u; // Пустой стек

    // Копировать
    if (node->size != size)
        return 0u;
    // Если копировать доступное валидное:
    //const unsigned int copy_size = (node->size < size) ? node->size : size;
    void* payload = (void*)(node + 1);
    memcpy(data_out, payload, size);

    // Обновить вершину и освобождить узел
    entry->stack = (stack_t)node->prev;
    free(node);

    return size;
}


