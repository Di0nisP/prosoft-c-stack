#ifndef CSTACK_H
#define CSTACK_H

/**
 * @typedef hstack_t
 * @brief Тип хэндлера стека.
 */
typedef int hstack_t;

/**
 * @brief Создать новый стек.
 *
 * @return Хэндлер нового стека (>=0) в случае успеха.
 * @return @c -1 в случае ошибки выполнения.
 */
hstack_t stack_new(void);

/**
 * @brief Удалить стек, если соответствующий хэндлеру стек существует.
 *
 * @param[in] stack     Хэндлер стека.
 */
void stack_free(const hstack_t stack);

/**
 * @brief Проверить хэндлер.
 *
 * @param[in] stack     Хэндлер стека.
 * 
 * @return @c 0 - если стек существует.
 * @return @c 1 - если стек не существует.
 */
int stack_valid_handler(const hstack_t stack);

/**
 * @brief Получить количество элементов в стеке.
 *
 * @param[in] stack     Хэндлер стека.
 * 
 * @return Количество элементов в стеке.
 * @return @c 0 - если стек не существует.
 */
unsigned int stack_size(const hstack_t stack);

/**
 * @brief Добавить элемент данных из буфера в стек.
 *
 * @param[in] stack     Хэндлер стека.
 * @param[in] data_in   Указатель на буфер с данными.
 * @param[in] size      Размер буфера с данными (в байтах).
 */
void stack_push(const hstack_t stack, const void* data_in, const unsigned int size);

/**
 * @brief Извлечь элемент из стека и записать его в буфер.
 *
 * @param[in] stack     Хэндлер стека.
 * @param[out] data_out Указатель на буфер для записи данных.
 * @param[in] size      Размер буфера (в байтах).
 * 
 * @return Размер записанных данных в байтах.
 * @return @c 0 - если стек не существует.
 */
unsigned int stack_pop(const hstack_t stack, void* data_out, const unsigned int size);

#endif // CSTACK_H
