#ifndef PARSER_H
#define PARSER_H

#include "field.h"    
#include "history.h"  
#include "command.h"  
#include <stdbool.h>

/**
 * Основная функция: читает файл с командами и выполняет их.
 * - filename: путь к входному файлу
 * - f: указатель на текущее поле (изменяется в процессе)
 * - hist: стек истории для UNDO
 * - opts: настройки визуализации
 * Возвращает true при успешном завершении, false — при ошибке.
 */
bool parse_and_execute_file(const char* filename, Field* f, History* hist, const Options* opts);

/**
 * Выполняет одну строку команды.
 * - line: строка из файла (без перевода строки)
 * - line_num: номер строки (для сообщений об ошибках)
 * Остальные параметры — как выше.
 */
bool execute_command(Field* f, History* hist, char* line, int line_num, const Options* opts);

#endif