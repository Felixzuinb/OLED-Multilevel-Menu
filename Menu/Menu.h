#ifndef __MENU_H
#define __MENU_H

#include "stm32f1xx_hal.h"

typedef enum{
    MODE_VERTICAL = 0,
    MODE_HORIZONTAL
} MenuMode;

// 菜单项结构体
typedef struct MenuItem {
    char text[20];              // 菜单显示文本
    void (*action)(void);       // 菜单项执行函数
    struct MenuItem *parent;    // 父菜单指针
    struct MenuItem *child;     // 子菜单指针
    struct MenuItem *next;      // 下一个兄弟菜单指针
    struct MenuItem *prev;      // 上一个兄弟菜单指针
} MenuItem;

// 菜单管理器
typedef struct {
    MenuItem *current_menu;     // 当前显示的菜单
    MenuItem *current_item;     // 当前选中的菜单项
    uint8_t display_start;      // 显示起始行
    uint8_t selected_index;     // 当前选中项的索引
    MenuMode mode;              // 菜单显示模式
} MenuManager;


extern MenuManager menu_manager;

#endif
