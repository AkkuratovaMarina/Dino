#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h> // Для sleep()

// Определение команды очистки в зависимости от ОС
#ifdef _WIN32
    #include <windows.h> // Для Sleep()
    #define CLEAR_CMD "cls"
#else
    #define CLEAR_CMD "clear"
#endif

/**
 * Вызывает системную команду очистки терминала.
 * Работает на Windows (cls) и Unix-системах (clear).
 */
void clear_screen(void) {
    system(CLEAR_CMD);
}

/**
 * Делает паузу на sec секунд.
 * На Windows использует Sleep() (в миллисекундах),
 * на Unix — sleep() (в секундах).
 */
void delay_seconds(int sec) {
    if (sec <= 0) return; // Нет задержки — выходим
#ifdef _WIN32
    Sleep(sec * 1000); // Windows: Sleep в миллисекундах
#else
    sleep(sec);        // Unix: sleep в секундах
#endif
}

/**
 * Проверяет, совпадает ли строка s с одним из четырёх направлений.
 */
bool is_direction(const char* s) {
    return strcmp(s, "UP") == 0 || strcmp(s, "DOWN") == 0 ||
           strcmp(s, "LEFT") == 0 || strcmp(s, "RIGHT") == 0;
}

/**
 * Заполняет *dx и *dy в зависимости от направления.
 */
void get_delta(const char* dir, int* dx, int* dy) {
    *dx = 0;
    *dy = 0;
    if (strcmp(dir, "UP") == 0) {
        *dy = -1; // Вверх — уменьшение y
    } else if (strcmp(dir, "DOWN") == 0) {
        *dy = 1;  // Вниз — увеличение y
    } else if (strcmp(dir, "LEFT") == 0) {
        *dx = -1; // Влево — уменьшение x
    } else if (strcmp(dir, "RIGHT") == 0) {
        *dx = 1;  // Вправо — увеличение x
    }
}