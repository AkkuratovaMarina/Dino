#include "history.h"
#include <stdlib.h>

/**
 * Создаёт новую структуру History и инициализирует её как пустой стек.
 */
History* create_history(void) {
    History* h = malloc(sizeof(History));
    if (!h) return NULL;
    h->top = NULL;   // Стек пуст
    h->count = 0;    // Нет сохранённых состояний
    return h;
}

/**
 * Добавляет копию текущего поля в стек истории.
 */
void push_state(History* h, Field* f) {
    // Защита от некорректных указателей и переполнения
    if (!h || !f || h->count >= MAX_UNDO_DEPTH) return;

    // Создаём новый узел стека
    State* s = malloc(sizeof(State));
    if (!s) return;

    // Делаем копию поля (все клетки, позиция динозавра)
    s->field = copy_field(f);
    if (!s->field) {
        free(s);
        return;
    }

    // Добавляем новый узел на вершину стека
    s->next = h->top;
    h->top = s;
    h->count++;
}

/**
 * Восстанавливает предыдущее состояние поля
 * Удаляет верхний элемент стека и копирует его данные в current.
 */
bool pop_state(History* h, Field* current) {
    // Проверка: стек не пуст и аргументы корректны
    if (!h || !h->top || !current) return false;

    // Запоминаем верхний элемент
    State* old = h->top;
    h->top = old->next; // Сдвигаем вершину вниз

    // Получаем сохранённое поле
    Field* saved = old->field;

    // Освобождаем память текущего поля (кроме самой структуры current)
    for (int i = 0; i < current->height; i++) {
        free(current->grid[i]);
    }
    free(current->grid);

    // Копируем все данные из сохранённого поля в current
    *current = *saved; // Копирование структуры (включая width, height, флаги)

    // Освобождаем саму структуру saved
    free(saved);

    // Уменьшаем счётчик и освобождаем узел стека
    h->count--;
    free(old);
    return true;
}

/**
 * Освобождает всю память, выделенную под стек истории.
 * Проходит по всем узлам и удаляет их вместе с полями.
 */
void free_history(History* h) {
    if (!h) return;

    // Удаляем все узлы стека по одному
    while (h->top) {
        State* tmp = h->top;
        h->top = tmp->next;
        free_field(tmp->field); 
        free(tmp);              
    }

    free(h);
}