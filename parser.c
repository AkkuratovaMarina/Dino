#include "parser.h"
#include "utils.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Вспомогательная функция для обработки условной команды IF.
 * Формат: IF CELL x y IS символ THEN команда
 *
 * Проверяет клетку (x, y):
 * - Если там объект или цвет совпадает с указанным — выполняет команду.
 * - Иначе — игнорирует.
 */
static bool execute_if_command(Field* f, History* hist, const char* rest, int line_num, const Options* opts) {
    int x, y;
    char sym[8];          // Символ для проверки (макс. 7 символов + '\0')
    char then_cmd[256];   // Команда после THEN (макс. 255 символов)

    // Разбираем строку: "CELL x y IS sym THEN команда"
    if (sscanf(rest, "CELL %d %d IS %7s THEN %[^\n]", &x, &y, sym, then_cmd) != 4) {
        fprintf(stderr, "ОШИБКА (строка %d): Неверный формат IF\n", line_num);
        return false;
    }

    // Нормализуем координаты (тор)
    x = wrap(x, f->width);
    y = wrap(y, f->height);

    // Определяем, что реально находится в клетке:
    // Если символ — '_', но есть цвет → используем цвет
    char cell_sym = f->grid[y][x].symbol;
    if (cell_sym == '_') {
        cell_sym = f->grid[y][x].color ? f->grid[y][x].color : '_';
    }

    // Ожидаемый символ — первый символ из строки sym
    char expected = sym[0];

    // Проверяем совпадение:
    // - либо точное совпадение символов,
    // - либо оба — строчные буквы (цвета)
    if (strlen(sym) == 1 && (
        expected == cell_sym ||
        (expected >= 'a' && expected <= 'z' && cell_sym == expected)
    )) {
        // Условие выполнено — выполняем команду рекурсивно
        return execute_command(f, hist, then_cmd, line_num, opts);
    }

    // Условие не выполнено — ничего не делаем
    return true;
}

/**
 * Основная функция выполнения одной команды.
 * Здесь реализована логика порядка команд, проверки ошибок и вызова действий.
 */
bool execute_command(Field* f, History* hist, char* line, int line_num, const Options* opts) {
    char cmd[32]; // Буфер для названия команды

    // Извлекаем первое "слово" — название команды
    if (sscanf(line, "%31s", cmd) != 1) {
        fprintf(stderr, "ОШИБКА (строка %d): Неверный формат команды\n", line_num);
        return false;
    }

    // =============== Команды, которые могут быть ПЕРВЫМИ ===============

    // Команда SIZE: задаёт размер поля
    if (strcmp(cmd, "SIZE") == 0) {
        // Нельзя вызывать SIZE дважды
        if (f->field_created) {
            fprintf(stderr, "ОШИБКА (строка %d): SIZE уже вызван\n", line_num);
            return false;
        }

        int w, h;
        if (sscanf(line, "SIZE %d %d", &w, &h) != 2) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверный формат SIZE\n", line_num);
            return false;
        }

        // Создаём новое поле
        Field* newf = create_field(w, h);
        if (!newf) {
            fprintf(stderr, "ОШИБКА (строка %d): Неправильный размер поля (должен быть от %dx%d до %dx%d)\n",
                    line_num, MIN_WIDTH, MIN_HEIGHT, MAX_WIDTH, MAX_HEIGHT);
            return false;
        }

        // Копируем данные нового поля в существующую структуру
        *f = *newf;
        free(newf); // Указатель скопирован, саму структуру можно удалить
        return true;
    }

    // Команда LOAD: загружает готовое поле из файла
    if (strcmp(cmd, "LOAD") == 0) {
        // LOAD должна быть первой командой
        if (f->field_created || f->dino_placed) {
            fprintf(stderr, "ОШИБКА (строка %d): LOAD должны быть первой командой\n", line_num);
            return false;
        }

        char fname[256];
        if (sscanf(line, "LOAD %255s", fname) != 1) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверный формат LOAD\n", line_num);
            return false;
        }

        // Открываем файл для загрузки
        FILE* fp = fopen(fname, "r");
        if (!fp) {
            fprintf(stderr, "ОШИБКА (строка %d): Невозможно открыть LOAD файл '%s'\n", line_num, fname);
            return false;
        }

        // Читаем размеры
        int w, h;
        if (fscanf(fp, "%d %d\n", &w, &h) != 2) {
            fclose(fp);
            return false;
        }

        // Создаём поле нужного размера
        Field* loaded = create_field(w, h);
        if (!loaded) {
            fclose(fp);
            return false;
        }

        // Читаем само поле посимвольно
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int c = fgetc(fp);
                if (c == EOF || c == '\n') {
                    free_field(loaded);
                    fclose(fp);
                    return false;
                }

                // Если символ — строчная буква, это цвет
                if (c >= 'a' && c <= 'z') {
                    loaded->grid[y][x].color = c;
                    loaded->grid[y][x].symbol = '_';
                } else {
                    // Иначе — это объект или пустота
                    loaded->grid[y][x].symbol = c;
                    loaded->grid[y][x].color = 0;
                }
            }
            // Проверяем, что строка завершается \n
            if (fgetc(fp) != '\n') {
                free_field(loaded);
                fclose(fp);
                return false;
            }
        }

        // Читаем позицию динозавра
        char dino_cmd[16];
        int dx, dy;
        if (fscanf(fp, "%15s %d %d", dino_cmd, &dx, &dy) != 3 || strcmp(dino_cmd, "DINO") != 0) {
            free_field(loaded);
            fclose(fp);
            return false;
        }
        fclose(fp);

        // Размещаем динозавра
        place_dinosaur(loaded, dx, dy);

        // Копируем загруженное поле в основное
        *f = *loaded;
        free(loaded);
        return true;
    }

    // =============== Проверки: поле и динозавр должны быть созданы ===============

    // Все остальные команды требуют, чтобы поле было создано
    if (!f->field_created) {
        fprintf(stderr, "ОШИБКА (строка %d): Поле не создано (не хватает SIZE или LOAD)\n", line_num);
        return false;
    }

    // Все команды, кроме START, требуют, чтобы динозавр был поставлен
    if (!f->dino_placed && strcmp(cmd, "START") != 0) {
        fprintf(stderr, "ОШИБКА (строка %d): Динозавр не размещён (не хватает START)\n", line_num);
        return false;
    }

    // =============== Сохранение состояния для UNDO ===============
    // Не сохраняем для UNDO, EXEC и IF (чтобы не засорять стек)
    if (strcmp(cmd, "UNDO") != 0 && strcmp(cmd, "EXEC") != 0 && strncmp(cmd, "IF", 2) != 0) {
        push_state(hist, f);
    }

    // =============== Выполнение конкретных команд ===============
    bool success = true;

    if (strcmp(cmd, "START") == 0) {
        // Нельзя вызывать START дважды
        if (f->dino_placed) {
            fprintf(stderr, "ОШИБКА (строка %d): START уже вызван\n", line_num);
            return false;
        }

        int x, y;
        if (sscanf(line, "START %d %d", &x, &y) != 2) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверный формат START\n", line_num);
            return false;
        }

        // Размещаем динозавра (координаты нормализуются внутри)
        place_dinosaur(f, x, y);
    }
    else if (strcmp(cmd, "MOVE") == 0) {
        char dir[16];
        if (sscanf(line, "MOVE %15s", dir) != 1 || !is_direction(dir)) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверное направление MOVE\n", line_num);
            return false;
        }
        success = move_dino(f, dir);
    }
    else if (strcmp(cmd, "JUMP") == 0) {
        char dir[16];
        int n;
        if (sscanf(line, "JUMP %15s %d", dir, &n) != 2 || !is_direction(dir) || n <= 0) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверный формат JUMP\n", line_num);
            return false;
        }
        success = jump_dino(f, dir, n);
    }
    else if (strcmp(cmd, "PAINT") == 0) {
        char c;
        if (sscanf(line, "PAINT %c", &c) != 1 || c < 'a' || c > 'z') {
            fprintf(stderr, "ОШИБКА (строка %d): PAINT требует строчную букву\n", line_num);
            return false;
        }
        paint_cell(f, c);
    }
    else if (strcmp(cmd, "DIG") == 0) {
        char dir[16];
        if (sscanf(line, "DIG %15s", dir) != 1 || !is_direction(dir)) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверное направление DIG\n", line_num);
            return false;
        }
        modify_adjacent(f, dir, '%', true); // Яма только на пустой клетке
    }
    else if (strcmp(cmd, "MOUND") == 0) {
        char dir[16];
        if (sscanf(line, "MOUND %15s", dir) != 1 || !is_direction(dir)) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверное направление MOUND\n", line_num);
            return false;
        }
        modify_adjacent(f, dir, '^', true); // Гора только на пустой клетке
    }
    else if (strcmp(cmd, "GROW") == 0) {
        char dir[16];
        if (sscanf(line, "GROW %15s", dir) != 1 || !is_direction(dir)) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверное направление GROW\n", line_num);
            return false;
        }
        modify_adjacent(f, dir, '&', true); // Дерево только на пустой клетке
    }
    else if (strcmp(cmd, "CUT") == 0) {
        char dir[16];
        if (sscanf(line, "CUT %15s", dir) != 1 || !is_direction(dir)) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверное направление CUT\n", line_num);
            return false;
        }
        cut_tree(f, dir);
    }
    else if (strcmp(cmd, "MAKE") == 0) {
        char dir[16];
        if (sscanf(line, "MAKE %15s", dir) != 1 || !is_direction(dir)) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверное направление MAKE\n", line_num);
            return false;
        }
        modify_adjacent(f, dir, '@', true); // Камень только на пустой клетке
    }
    else if (strcmp(cmd, "PUSH") == 0) {
        char dir[16];
        if (sscanf(line, "PUSH %15s", dir) != 1 || !is_direction(dir)) {
            fprintf(stderr, "ОШИБКА (строка %d): Неверное направление PUSH\n", line_num);
            return false;
        }
        push_stone(f, dir);
    }
    else if (strcmp(cmd, "EXEC") == 0) {
        char fname[256];
        if (sscanf(line, "EXEC %255s", fname) != 1) {
            fprintf(stderr, "ОШИБКА (строка %d): Неправильный формат EXEC\n", line_num);
            return false;
        }
        // Рекурсивно выполняем другой файл
        if (!parse_and_execute_file(fname, f, hist, opts)) {
            return false;
        }
    }
    else if (strcmp(cmd, "UNDO") == 0) {
        // Восстанавливаем предыдущее состояние
        if (!pop_state(hist, f)) {
            fprintf(stderr, "ВНИМАНИЕ (строка %d): Нечего отменять\n", line_num);
        }
        // Пропускаем визуализацию после UNDO (goto ниже)
        goto skip_display;
    }
    else if (strncmp(cmd, "IF", 2) == 0) {
        // Находим остаток строки после "IF "
        char* rest = strchr(line, ' ');
        if (!rest) {
            fprintf(stderr, "ОШИБКА (строка %d): Неправильный синтаксис IF\n", line_num);
            return false;
        }
        rest++; // Пропускаем пробел
        if (!execute_if_command(f, hist, rest, line_num, opts)) {
            return false;
        }
        goto skip_display; // Не сохраняем и не визуализируем дважды
    }
    else {
        // Неизвестная команда
        fprintf(stderr, "ОШИБКА (строка %d): Незнакомая команда '%s'\n", line_num, cmd);
        return false;
    }

    // Если команда завершилась критической ошибкой (например, падение в яму)
    if (!success) return false;

    // =============== Визуализация ===============
skip_display:
    if (opts->display) {
        clear_screen();      // Очищаем консоль
        print_field(f);      // Выводим поле
        delay_seconds(opts->interval); // Ждём заданное время
    }

    return true;
}

/**
 * Основная функция чтения файла.
 * Читает построчно, пропускает комментарии и пустые строки,
 * проверяет формат и вызывает execute_command.
 */
bool parse_and_execute_file(const char* filename, Field* f, History* hist, const Options* opts) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "ОШИБКА: Невозможно открыть файл '%s'\n", filename);
        return false;
    }

    char buffer[4096]; // Буфер фиксированного размера
    int line_num = 0;

    // Читаем файл построчно
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        line_num++;

        // Удаляем символ новой строки
        char* nl = strchr(buffer, '\n');
        if (nl) *nl = '\0';

        // Удаляем пробелы в конце строки
        char* end = buffer + strlen(buffer) - 1;
        while (end >= buffer && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }

        // Пропускаем пустые строки и комментарии
        if (strlen(buffer) == 0 || strncmp(buffer, "//", 2) == 0) {
            continue;
        }

        // Проверяем пробелы в начале строки
        if (isspace((unsigned char)buffer[0])) {
            fprintf(stderr, "ОШИБКА (строка %d): Пробелы в начале строки запрещены\n", line_num);
            fclose(fp);
            return false;
        }

        // Выполняем команду
        if (!execute_command(f, hist, buffer, line_num, opts)) {
            fclose(fp);
            return false;
        }
    }

    // Освобождаем буфер и закрываем файл
    fclose(fp);
    return true;
}