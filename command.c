#include "command.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

/**
 * Вспомогательная функция: проверяет, можно ли зайти в клетку (x, y).
 * Препятствия: яма (%), гора (^), дерево (&), камень (@).
 * Пустые клетки (_) и цветные — проходимы.
 */
bool can_move_to(Field* f, int x, int y) {
    // Нормализуем координаты (тор)
    x = wrap(x, f->width);
    y = wrap(y, f->height);

    char sym = f->grid[y][x].symbol;
    return sym != '%' && sym != '^' && sym != '&' && sym != '@';
}

/**
 * Перемещает динозавра на одну клетку в направлении dir.
 * Логика:
 * 1. Вычисляем новые координаты.
 * 2. Проверяем целевую клетку:
 *    - Если яма (%) → ошибка, завершение.
 *    - Если препятствие (^, &, @) → предупреждение, команда игнорируется.
 * 3. Если всё ок — перемещаем динозавра.
 */
bool move_dino(Field* f, const char* dir) {
    int dx = 0, dy = 0;
    get_delta(dir, &dx, &dy); // Получаем вектор смещения

    // Новые координаты с учётом тора
    int nx = wrap(f->dino_x + dx, f->width);
    int ny = wrap(f->dino_y + dy, f->height);

    // Проверяем, что находится в целевой клетке
    char target = f->grid[ny][nx].symbol;

    // Случай 1: яма — критическая ошибка
    if (target == '%') {
        fprintf(stderr, "ОШИБКА: Динозавр свалился в яму!\n");
        return false; // Программа завершится
    }

    // Случай 2: препятствие — просто игнорируем команду
    if (target == '^' || target == '&' || target == '@') {
        fprintf(stderr, "ВНИМАНИЕ: Движение блокируется препятствием.\n");
        return true; // Не ошибка, просто ничего не делаем
    }

    // Случай 3: можно идти
    // Сначала очищаем старую позицию:
    // Если там был цвет — оставляем его, иначе ставим '_'
    f->grid[f->dino_y][f->dino_x].symbol =
        f->grid[f->dino_y][f->dino_x].color ?
        f->grid[f->dino_y][f->dino_x].color : '_';

    // Ставим динозавра на новое место
    place_dinosaur(f, nx, ny);
    return true;
}

/**
 * Выполняет прыжок на n клеток в направлении dir.
 * Логика:
 * 1. Пошагово проверяем каждую клетку на пути.
 * 2. Если встречаем гору, дерево или камень — останавливаемся ПЕРЕД препятствием.
 * 3. Если приземляемся в яму — ошибка.
 * 4. Иначе — перемещаемся в конечную точку.
 */
bool jump_dino(Field* f, const char* dir, int n) {
    if (n <= 0) return true; // Некорректный прыжок — игнорируем

    int dx = 0, dy = 0;
    get_delta(dir, &dx, &dy);

    int cx = f->dino_x; // Текущая позиция
    int cy = f->dino_y;

    // Предварительно вычисляем конечную точку (с тором)
    int final_x = wrap(cx + dx * n, f->width);
    int final_y = wrap(cy + dy * n, f->height);

    // Проверяем путь по каждой промежуточной клетке
    for (int step = 1; step <= n; step++) {
        int px = wrap(cx + dx * step, f->width);
        int py = wrap(cy + dy * step, f->height);
        char sym = f->grid[py][px].symbol;

        // Если встречаем непроходимый объект — останавливаемся перед ним
        if (sym == '&' || sym == '@' || sym == '^') {
            if (step == 1) {
                // Препятствие сразу рядом — прыжок невозможен
                fprintf(stderr, "ВНИМАНИЕ: Прыжок блокируется немедленно.\n");
                return true;
            }
            // Останавливаемся на предыдущей клетке
            final_x = wrap(cx + dx * (step - 1), f->width);
            final_y = wrap(cy + dy * (step - 1), f->height);
            fprintf(stderr, "ВНИМАНИЕ: Прыжок остановлен перед препятствием.\n");
            break; // Дальше не летим
        }
        //Ямы (%) не блокируют полёт, только приземление
    }

    // Проверяем клетку приземления
    if (f->grid[final_y][final_x].symbol == '%') {
        fprintf(stderr, "ОШИБКА: Динозавр приземлился в яму во время прыжка!\n");
        return false;
    }

    // Перемещаем динозавра
    f->grid[f->dino_y][f->dino_x].symbol =
        f->grid[f->dino_y][f->dino_x].color ?
        f->grid[f->dino_y][f->dino_x].color : '_';

    place_dinosaur(f, final_x, final_y);
    return true;
}

/**
 * Окрашивает текущую клетку динозавра в букву c.
 * Цвет сохраняется даже после ухода динозавра.
 */
bool paint_cell(Field* f, char c) {
    if (c < 'a' || c > 'z') return false; // Только строчные латинские буквы
    f->grid[f->dino_y][f->dino_x].color = c;
    return true;
}

/**
 * Создаёт или изменяет объект в соседней клетке по направлению.
 * Параметры:
 * - new_symbol: символ создаваемого объекта
 * - require_empty: если true, объект можно ставить только на '_'
 * Особенности:
 * - MOUND (^) в яму (%) → яма засыпается (становится пустой или цветной)
 * - Дерево (&) и камень (@) можно ставить ТОЛЬКО на пустую клетку '_'
 */
bool modify_adjacent(Field* f, const char* dir, char new_symbol, bool require_empty) {
    int dx = 0, dy = 0;
    get_delta(dir, &dx, &dy);

    // Координаты соседней клетки
    int nx = wrap(f->dino_x + dx, f->width);
    int ny = wrap(f->dino_y + dy, f->height);

    char current = f->grid[ny][nx].symbol;
    char current_color = f->grid[ny][nx].color;

    // Общий запрет: нельзя создавать на непустой клетке (если require_empty=true),
    // за исключением специального случая: MOUND в яму
    if (require_empty && current != '_' && !(current == '%' && new_symbol == '^')) {
        return false; // Нельзя создать
    }

    // Специальный случай: засыпание ямы горой
    if (current == '%' && new_symbol == '^') {
        // Яма исчезает, остаётся цвет (если был)
        f->grid[ny][nx].symbol = current_color ? current_color : '_';
        f->grid[ny][nx].color = current_color;
        return true;
    }

    // Для дерева и камня — дополнительная проверка: только на '_'
    if ((new_symbol == '&' || new_symbol == '@') && current != '_') {
        return false;
    }

    // Обычное создание объекта
    f->grid[ny][nx].symbol = new_symbol;
    // Цвет НЕ перезаписываем — он сохраняется!
    return true;
}

/**
 * Удаляет дерево в соседней клетке по направлению.
 * Если клетка была окрашена — цвет сохраняется.
 */
bool cut_tree(Field* f, const char* dir) {
    int dx = 0, dy = 0;
    get_delta(dir, &dx, &dy);

    int nx = wrap(f->dino_x + dx, f->width);
    int ny = wrap(f->dino_y + dy, f->height);

    // Проверяем, есть ли там дерево
    if (f->grid[ny][nx].symbol != '&') {
        return false; // Нечего рубить
    }

    // Сохраняем цвет клетки
    char col = f->grid[ny][nx].color;

    // Делаем клетку пустой, но с цветом (если был)
    f->grid[ny][nx].symbol = col ? col : '_';
    return true;
}

/**
 * Толкает камень в соседней клетке по направлению.
 * Логика:
 * 1. Проверяем, есть ли камень рядом.
 * 2. Определяем клетку, куда он должен полететь (на шаг дальше).
 * 3. Если там яма (%) → камень засыпает яму.
 * 4. Если там пусто (_) → камень перемещается.
 * 5. Если там препятствие (^, &, @) → ничего не происходит.
 */
bool push_stone(Field* f, const char* dir) {
    int dx = 0, dy = 0;
    get_delta(dir, &dx, &dy);

    // Позиция камня (соседняя клетка)
    int sx = wrap(f->dino_x + dx, f->width);
    int sy = wrap(f->dino_y + dy, f->height);

    // Проверяем, есть ли там камень
    if (f->grid[sy][sx].symbol != '@') {
        return false; // Нечего толкать
    }

    // Позиция, куда летит камень (на шаг дальше)
    int tx = wrap(sx + dx, f->width);
    int ty = wrap(sy + dy, f->height);

    char target = f->grid[ty][tx].symbol;
    char target_color = f->grid[ty][tx].color;

    // Если там неподвижное препятствие — камень не двигается
    if (target == '^' || target == '&' || target == '@') {
        return true; // Просто ничего не делаем
    }

    // Если камень попадает в яму — яма засыпается
    if (target == '%') {
        // Яма исчезает, цвет сохраняется
        f->grid[ty][tx].symbol = target_color ? target_color : '_';
        f->grid[ty][tx].color = target_color;
    }
    // Если клетка пустая — просто ставим туда камень
    else if (target == '_') {
        f->grid[ty][tx].symbol = '@';
        // Цвет остаётся как есть (обычно 0)
    } else {
        // Неожиданный символ — ошибка (не должно происходить)
        return false;
    }

    // Убираем камень со старого места
    char old_col = f->grid[sy][sx].color;
    f->grid[sy][sx].symbol = old_col ? old_col : '_';
    f->grid[sy][sx].color = old_col;

    return true;
}