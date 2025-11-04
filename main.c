#include "field.h"
#include "history.h"
#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Разбирает аргументы командной строки после имён файлов.
 * Поддерживаемые опции:
 * --interval N   : задержка между кадрами (по умолчанию 1)
 * --no-display   : отключить визуализацию
 * --no-save      : не сохранять результат в файл
 */
void parse_options(int argc, char* argv[], Options* opts) {
    // Устанавливаем значения по умолчанию
    opts->interval = 1;
    opts->display = true;
    opts->save = true;

    // Проходим по аргументам
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            opts->interval = atoi(argv[++i]); // Берём следующий аргумент как число
            if (opts->interval < 0) opts->interval = 0;
        } else if (strcmp(argv[i], "--no-display") == 0) {
            opts->display = false;
        } else if (strcmp(argv[i], "--no-save") == 0) {
            opts->save = false;
        }
    }
}

/**
 * Точка входа в программу.
 * Формат запуска: ./movdino input.txt output.txt [опции]
 */
int main(int argc, char* argv[]) {
    // Проверка минимального количества аргументов
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input.txt output.txt [--interval N] [--no-display] [--no-save]\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    // Разбор опций
    Options opts;
    parse_options(argc - 3, argv + 3, &opts);

    // Создаём базовое поле (изначально не инициализировано)
    Field base_field = {0}; // Все поля = 0 / false

    // Создаём стек истории для UNDO
    History* history = create_history();

    // Запускаем выполнение программы из файла
    bool ok = parse_and_execute_file(input_file, &base_field, history, &opts);

    // Сохраняем результат, если не запрещено
    if (ok && opts.save) {
        save_field_to_file(&base_field, output_file);
    }

    // Освобождаем память, выделенную под поле
    if (base_field.grid) {
        for (int i = 0; i < base_field.height; i++) {
            free(base_field.grid[i]);
        }
        free(base_field.grid);
    }

    // Освобождаем историю
    free_history(history);

    // Возвращаем код завершения: 0 — успех, 1 — ошибка
    return ok ? 0 : 1;
}