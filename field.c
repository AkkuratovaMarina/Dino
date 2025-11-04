#include "field.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Функция wrap реализует тороидальную топологию:
 * если координата выходит за границы, она "оборачивается" на противоположную сторону
 */
int wrap(int coord, int size) {
    if (size <= 0) return 0; // Защита от деления на ноль
    coord %= size;           // Остаток от деления
    if (coord < 0) coord += size; // Если отрицательный — добавляем размер
    return coord;
}

/**
 * Создаёт новое поле размером w × h.
 * Инициализирует все клетки как пустые ('_') без цвета.
 */
Field* create_field(int w, int h) {
    if (w < MIN_WIDTH || w > MAX_WIDTH || h < MIN_HEIGHT || h > MAX_HEIGHT)
        return NULL;

    // Выделяем память под структуру поля
    Field* f = calloc(1, sizeof(Field));
    if (!f) return NULL;

    f->width = w;
    f->height = h;

    // Выделяем память под массив строк (указателей на Cell)
    f->grid = calloc(h, sizeof(Cell*));
    if (!f->grid) {
        free(f);
        return NULL;
    }

    // Для каждой строки выделяем память под клетки
    for (int i = 0; i < h; i++) {
        f->grid[i] = calloc(w, sizeof(Cell));
        if (!f->grid[i]) {
            // При ошибке — освобождаем уже выделенную память
            for (int j = 0; j < i; j++) free(f->grid[j]);
            free(f->grid);
            free(f);
            return NULL;
        }
        // Инициализация клеток: пусто и без цвета
        for (int j = 0; j < w; j++) {
            f->grid[i][j].symbol = '_';
            f->grid[i][j].color = 0;
        }
    }

    f->field_created = true;
    return f;
}

/**
 * Освобождает всю память, выделенную под поле
 */
void free_field(Field* f) {
    if (!f) return;
    if (f->grid) {
        for (int i = 0; i < f->height; i++) {
            free(f->grid[i]);
        }
        free(f->grid);
    }
    free(f);
}

/**
 * Создаёт полную копию поля 
 */
Field* copy_field(const Field* src) {
    Field* dst = create_field(src->width, src->height);
    if (!dst) return NULL;

    // Копируем содержимое каждой клетки
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            dst->grid[y][x] = src->grid[y][x];
        }
    }

    // Копируем позицию динозавра и флаги
    dst->dino_x = src->dino_x;
    dst->dino_y = src->dino_y;
    dst->dino_placed = src->dino_placed;

    return dst;
}

/**
 * Размещает динозавра в заданных координатах.
 * Сохраняет цвет клетки, если он был.
 * Обновляет глобальные координаты динозавра.
 */
void place_dinosaur(Field* f, int x, int y) {
    if (!f || !f->field_created) return;

    // Приводим координаты к корректному диапазону
    x = wrap(x, f->width);
    y = wrap(y, f->height);

    // Сохраняем цвет клетки
    char old_color = f->grid[y][x].color;

    // Ставим динозавра
    f->grid[y][x].symbol = '#';
    f->grid[y][x].color = old_color; 

    // Обновляем позицию
    f->dino_x = x;
    f->dino_y = y;
    f->dino_placed = true;
}

/**
 * Выводит поле в консоль.
 * Если клетка пустая ('_'), но имеет цвет — выводим цвет.
 * Иначе — выводим символ объекта.
 */
void print_field(Field* f) {
    if (!f || !f->field_created) return;

    for (int y = 0; y < f->height; y++) {
        for (int x = 0; x < f->width; x++) {
            char sym = f->grid[y][x].symbol;
            if (sym == '_' && f->grid[y][x].color != 0) {
                // Пустая клетка с цветом -> выводим цвет
                putchar(f->grid[y][x].color);
            } else {
                // Иначе — выводим символ объекта
                putchar(sym);
            }
        }
        putchar('\n');
    }
    fflush(stdout); // Гарантируем немедленный вывод
}

/**
 * Сохраняет поле в файл в формате:
 * width height
 * строка_поля_1
 * ...
 * строка_поля_height
 * DINO x y
 */
bool save_field_to_file(Field* f, const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return false;

    // Записываем размеры
    fprintf(fp, "%d %d\n", f->width, f->height);

    // Записываем каждую строку поля
    for (int y = 0; y < f->height; y++) {
        for (int x = 0; x < f->width; x++) {
            char sym = f->grid[y][x].symbol;
            if (sym == '_' && f->grid[y][x].color != 0) {
                fputc(f->grid[y][x].color, fp); // Цвет
            } else {
                fputc(sym, fp); // Символ объекта
            }
        }
        fputc('\n', fp);
    }

    // Записываем позицию динозавра
    fprintf(fp, "DINO %d %d\n", f->dino_x, f->dino_y);

    fclose(fp);
    return true;
}