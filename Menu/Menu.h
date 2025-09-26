#ifndef __MENU_H
#define __MENU_H

#include "stm32f1xx_hal.h"

typedef enum{
    MODE_VERTICAL = 0,
    MODE_HORIZONTAL
} MenuMode;

typedef struct Camera{
    uint8_t start_index;      // 镜头第一行的索引
    uint8_t selected_index;     // 当前选中项的索引
    uint8_t displayed_index;    // 镜头中选中项的索引
} Camera;


#define TOWARD_CHILD 0
#define TOWARD_PARENT 1
#define TOWARD_NONE 2

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

    Camera camera_index[4];     // 镜头索引数组(即支持四层菜单嵌套)
    uint8_t depth;              // 当前菜单深度

    uint8_t toward;         // 菜单进出方向

    MenuMode mode;              // 菜单显示模式
} MenuManager;


extern MenuManager menu_manager;

void Menu_Init(void);
MenuItem* Menu_CreateItem(const char* text, void (*action)(void));
void Menu_InitItem(const char* text, MenuItem* item, void (*action)(void));
void Menu_AddChild(MenuItem* parent, MenuItem* child);
void Menu_EnterChild(void);
void Menu_GoBack(void);
void Menu_SelectNext(void);
void Menu_SelectPrev(void);
void Menu_ExecuteAction(void);
void Menu_Display(void);

#endif
