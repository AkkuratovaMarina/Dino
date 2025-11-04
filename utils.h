#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

/**
 * Очищает консоль.
 * Использует system("clear") на Linux/macOS и system("cls") на Windows.
 */
void clear_screen(void);

/**
 * Делает паузу на указанное количество секунд.
 */
void delay_seconds(int sec);

/**
 * Проверяет, является ли строка допустимым направлением движения.
 */
bool is_direction(const char* s);

/**
 * Преобразует строку направления в вектор смещения (dx, dy)
 */
void get_delta(const char* dir, int* dx, int* dy);

#endif