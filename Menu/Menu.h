#ifndef __MENU_H
#define __MENU_H

#include "stm32f1xx_hal.h"
#include "Text.h"

#define UP 0
#define DOWN 1

typedef enum{
    MODE_VERTICAL = 0,
    MODE_HORIZONTAL
} MenuMode;

typedef enum{
    TOWARD_CHILD = 0,
    TOWARD_PARENT,
    TOWARD_NONE
} Toward;

typedef struct Camera{
    uint8_t start_index;      // 镜头第一行的索引
    uint8_t selected_index;     // 当前选中项的索引
    uint8_t displayed_index;    // 镜头中选中项的索引
} Camera;

// 菜单项结构体
typedef struct MenuItem {
    const char* text;              // 菜单显示文本
    TextID text_id;            // 菜单文本ID
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
    MenuItem *root;             // 根目录菜单（用于递归更改字体）

    Camera camera_index[4];     // 镜头索引数组(即支持四层菜单嵌套)
    uint8_t depth;              // 当前菜单深度

    Toward toward;         // 菜单进出方向

    const char* (*text)(const MenuItem*);   // 获取菜单文本的函数指针
    void (*refreshText)(MenuItem*); // 刷新菜单文本的函数指针

    MenuMode mode;              // 菜单显示模式
} MenuManager;


extern MenuManager menu_manager;
extern Language currentLanguage;

void Menu_Init(void);
void Menu_InitItem(TextID textID, MenuItem* item, void (*action)(void));
void Menu_AddChild(MenuItem* parent, MenuItem* child);
void Menu_EnterChild(void);
void Menu_GoBack(void);
void Menu_SelectNext(void);
void Menu_SelectPrev(void);
void Menu_ExecuteAction(void);
void Menu_Display(void);

#endif
