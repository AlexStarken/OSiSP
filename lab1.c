#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include <locale.h>

typedef enum types {
    file,      ///< Обычный файл
    symlink,   ///< Символическая ссылка
    dir        ///< Директория
} types;



typedef struct item_s {
    char* path;   ///< Путь к файлу или директории.
    types type;   ///< Тип элемента (файл, символическая ссылка, директория).
} item_t;


item_t create_item_t(char* path, struct stat fileStat);
void print_items(item_t* items, int count);
void free_items(item_t* items, int count);




typedef struct parameters_s {
    char* path;          ///< Путь к директории, которую нужно сканировать.
    bool symlinks;       ///< Флаг, указывающий, нужно ли включать символические ссылки в результат.
    bool catalogs;       ///< Флаг, указывающий, нужно ли включать каталоги в результат.
    bool files;          ///< Флаг, указывающий, нужно ли включать обычные файлы в результат.
    bool sort;           ///< Флаг, указывающий, нужно ли сортировать результат по алфавиту.
} parameters_t;


parameters_t get_options(int argc, char* argv[]);
//item_t* scan_file(parameters_t params, item_t** items, int* count);
int compare_strings(const void *a, const void *b);


item_t create_item_t(char* path, struct stat fileStat){
    item_t ret;

    // Выделение памяти для хранения пути и копирование пути в структуру.
    ret.path = (char*)malloc(strlen(path) + 1);
    strcpy(ret.path, path);

    // Определение типа элемента в зависимости от режима файла.
    if(S_ISREG(fileStat.st_mode)) ret.type = file;
    if(S_ISLNK(fileStat.st_mode)) ret.type = symlink;
    if(S_ISDIR(fileStat.st_mode)) ret.type = dir;

    return ret;
}

/**
 * @brief Выводит элементы массива item_t на экран.
 *
 * @param items Массив элементов item_t.
 * @param count Количество элементов в массиве.
 */
void print_items(item_t* items, int count){
    for(int i = 0; i < count; i++)
        printf("%s\n", items[i].path);
}

/**
 * @brief Освобождает память, выделенную под элементы и их пути.
 *
 * @param items Массив элементов item_t.
 * @param count Количество элементов в массиве.
 */
void free_items(item_t* items, int count){
    for(int i = 0; i < count; i++)
        free(items[i].path);
    free(items);
}


parameters_t get_options(int argc, char* argv[]){
    parameters_t params;
    params.path = (char*)malloc(255);  // Выделение памяти для пути по умолчанию
    params.symlinks = false;
    params.catalogs = false;
    params.files = false;
    params.sort = false;

    char ch;
    while((ch = getopt(argc, argv, "ldfs")) != EOF){
        switch (ch){
        case 'l':
            params.symlinks = true;
            break;
        case 'd':
            params.catalogs = true;
            break;
        case 'f':
            params.files = true;
            break;
        case 's':
            params.sort = true;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-d] [-f] [-s]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Если не указаны категории (каталоги, файлы, символические ссылки), включаем все по умолчанию.
    if(!(params.symlinks || params.catalogs || params.files)){
        params.symlinks = true;
        params.catalogs = true;
        params.files = true;
    }

    // Обновляем argc и argv, чтобы игнорировать аргументы, обработанные getopt.
    argc -= optind;
    argv += optind;

    // Если путь не указан, используем текущую директорию.
    if(argc == 0)
        strcpy(params.path, ".");
    else
        params.path = argv[0];

    return params;
}
int compare_strings(const void *a, const void *b) {
    const item_t *item_l = (const item_t *)a;
    const item_t *item_r = (const item_t *)b;
    return strcoll((const char *)item_l->path, (const char *)item_r->path);
}

/**
 * @brief Сканирует файлы и каталоги в указанной директории и ее поддиректориях.
 *
 * @param params Параметры сканирования (фильтры и директория).
 * @param items Массив элементов item_t, который будет заполнен результатами сканирования.
 * @param count Количество элементов в массиве items.
 */

void scan_file( parameters_t params, item_t** items, int* count){
    DIR *directory;
    struct dirent *entry;
    struct stat fileStat;

    // Открываем папку.
    directory = opendir(params.path);

    // Проверка на успешное открытие папки.
    if (directory == NULL) {
        perror("Ошибка при открытии папки");
        exit(EXIT_FAILURE);
    }

    // Читаем содержимое папки.
    while ((entry = readdir(directory)) != NULL) {
        // Игнорируем ".." и "."
        if(strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0)
            continue;

        // Составляем полный путь к файлу.
        char* full_path = (char*)malloc(strlen(params.path) + strlen(entry->d_name) + 2);
        snprintf(full_path, strlen(params.path) + strlen(entry->d_name) + 2, "%s/%s", params.path, entry->d_name);

        // Получаем информацию о файле.
        if (stat(full_path, &fileStat) == -1) {
            perror("Ошибка при получении информации о файле");
            exit(EXIT_FAILURE);
        }

        // Если текущий элемент - директория, рекурсивно вызываем scan_file для нее.
        if(S_ISDIR(fileStat.st_mode)){
            parameters_t new_params;
            new_params.path = (char*)malloc(strlen(full_path) + 1);
            new_params.catalogs = params.catalogs;
            new_params.symlinks = params.symlinks;
            new_params.files = params.files;
            new_params.sort = params.sort;

            strcpy(new_params.path, full_path);
            scan_file(new_params, items, count);
        }
        
        // Выводим имена файлов в консоль, если соответствует условиям.
        if( (params.catalogs && S_ISDIR(fileStat.st_mode)) ||
            (params.files && S_ISREG(fileStat.st_mode)) ||
            (params.symlinks && S_ISLNK(fileStat.st_mode))){
            *items = (item_t*)realloc(*items, sizeof(item_t) * (++(*count)));
            (*items)[*count - 1] = create_item_t(full_path, fileStat);
        }
    }
    
    // Закрываем папку.
    closedir(directory);
}

int main(int argc, char *argv[]){
    // Выделение памяти для массива структур item_t.
    // Инициализация с нулевым размером и указанием наличия элементов.
    item_t* items = (item_t*)malloc(0);

    // Инициализация счетчика элементов.
    int count = 0;

    // Получение параметров командной строки.
    parameters_t params = get_options(argc, argv);

    // Сканирование файлов в соответствии с переданными параметрами.
    scan_file(params, &items, &count);

    // Если включена опция сортировки, применение qsort для сортировки элементов.
    if (params.sort)
        qsort(items, count, sizeof(item_t), compare_strings);

    // Вывод элементов на экран.
    print_items(items, count);

    // Освобождение выделенной памяти.
    free_items(items, count);
    
    // Завершение программы с кодом успешного завершения.
    exit(EXIT_SUCCESS);
}
