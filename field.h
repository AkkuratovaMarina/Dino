#ifndef FIELD_H
#define FIELD_H

#include <stdbool.h>

#define MAX_WIDTH 100
#define MAX_HEIGHT 100
#define MIN_WIDTH 10
#define MIN_HEIGHT 10

/**
 * Структура Cell описывает одну клетку игрового поля.
 * - symbol: текущий символ в клетке ('_', '#', '%', '^', '&', '@' или буква цвета)
 * - color: если клетка была окрашена командой PAINT, здесь хранится буква 'a'-'z';
 *          иначе — 0 (нулевой символ).
 */
typedef struct {
    char symbol; 
    char color;   
} Cell;

/**
 * Структура Field описывает всё игровое поле.
 * - width, height: размеры поля
 * - grid: двумерный массив клеток (Указатель на массив указателей)
 * - dino_x, dino_y: текущие координаты динозавра
 * - field_created: флаг, создано ли поле командой SIZE
 * - dino_placed: флаг, поставлен ли динозавр командой START
 */
typedef struct {
    int width;
    int height;
    Cell** grid;           
    int dino_x, dino_y;    
    bool field_created;    
    bool dino_placed;      
} Field;

// Создаёт новое поле заданного размера. Возвращает NULL при ошибке
Field* create_field(int w, int h);

// Освобождает всю память, выделенную под поле
void free_field(Field* f);

// Создаёт полную копию поля
Field* copy_field(const Field* src);

// Размещает динозавра в заданных координатах
void place_dinosaur(Field* f, int x, int y);

// Выводит текущее состояние поля в консоль
void print_field(Field* f);

// Сохраняет поле в файл
bool save_field_to_file(Field* f, const char* filename);

// Приводит координату к корректному диапазону [0, size) с учётом тора
int wrap(int coord, int size);

#endif