#ifndef DIALOG_H
#define DIALOG_H

#include <iostream>

#include <unistd.h>
#include <syscall.h>

/*
     \033[30m   设置前景色:黑色
     \033[40m   设置背景色:黑色

     \033[31m   设置前景色:深红
     \033[41m   设置背景色:深红

     \033[32m   设置前景色:绿色
     \033[42m   设置背景色:绿色

     \033[33m   设置前景色:黄色
     \033[43m   设置背景色:黄色

     \033[34m   设置前景色:蓝色
     \033[44m   设置背景色:蓝色

     \033[35m   设置前景色:紫色
     \033[45m   设置背景色:紫色

     \033[36m   设置前景色:深绿色
     \033[46m   设置背景色:深绿色

     \033[37m   设置前景色:白色
     \033[47m   设置背景色:白色d
*/
enum COLOR {
    COLOR_ORIGIN_FRONT = 1, //origin
    COLOR_INFO_FRONT = 30, //black
    COLOR_DEBUG_FRONT = 34, //blue
    COLOR_WARN_FRONT = 33, //yellow
    COLOR_ERROR_FRONT = 31, //red
    COLOR_FATAL_FRONT = 35, //purple

    COLOR_ORIGIN_BACK = 11, //origin
    COLOR_INFO_BACK = 40, //black
    COLOR_DEBUG_BACK = 44, //blue
    COLOR_WARN_BACK = 43, //yellow
    COLOR_ERROR_BACK = 41, //red
    COLOR_FATAL_BACK = 45 //purple
};


static int s_color[] = {
        COLOR_ORIGIN_FRONT,
        COLOR_INFO_FRONT,
        COLOR_DEBUG_FRONT,
        COLOR_WARN_FRONT,
        COLOR_ERROR_FRONT,
        COLOR_FATAL_FRONT,
        COLOR_ORIGIN_BACK,
        COLOR_INFO_BACK,
        COLOR_DEBUG_BACK,
        COLOR_WARN_BACK,
        COLOR_ERROR_BACK,
        COLOR_FATAL_BACK,

};

//#define PRINT(title, level, files, func, line, fmt, ...) \
//    printf(""); \
//    printf("\033[%d;40m", s_color[level]); \
//    printf("[%s]tid:%d, files: %s, func: %s, line: %d, " fmt, title, syscall(SYS_gettid), files, func, line, ##__VA_ARGS__); \
//    printf("\033[0m")
#define PRINT(title, level_f, level_b, file, func, line, fmt, ...) \
    printf(""); \
    printf("\033[%d;%dm", level_f, level_b); \
    printf("[%s] tid:%d, %s:<%s>, line: %d, " fmt, title, syscall(SYS_gettid), file, func, line, ##__VA_ARGS__); \
    printf("\033[0m\n"); \

#define PRINT_LOG(title, level_f, level_b, fmt, ...) \
    PRINT(title, s_color[level_f], s_color[level_b], __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)



#define LOGI(fmt, ...) PRINT_LOG("info", 0, 6, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) PRINT_LOG("debug", 2, 6, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) PRINT_LOG("warn🥵", 3, 6, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) PRINT_LOG("error🤐", 4, 6, fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) PRINT_LOG("fatal", 5, 6, fmt, ##__VA_ARGS__)

//#define SHOWI(fmt, ...) PRINT_BACK("info", 5, fmt, ##__VA_ARGS__)
//#define SHOWD(fmt, ...) PRINT_BACK("debug", 6, fmt, ##__VA_ARGS__)
//#define SHOWW(fmt, ...) PRINT_BACK("warn", 7, fmt, ##__VA_ARGS__)
//#define SHOWE(fmt, ...) PRINT_BACK("error", 8, fmt, ##__VA_ARGS__)
//#define SHOWF(fmt, ...) PRINT_BACK("fatal", 9, fmt, ##__VA_ARGS__)

#endif