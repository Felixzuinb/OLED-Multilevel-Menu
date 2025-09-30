#ifndef __MENU_H
#define __MENU_H

#include "stm32f1xx_hal.h"
#include "Text.h"
#include "MenuAnim.h"

#define UP 0
#define DOWN 1

#define A_VALID 1
#define A_INVALID 0

#define ITEM_FULL 1
#define ITEM_UNFULL 0

typedef enum {
    ANIM_NONE,
    ANIM_MENU_SLIDE,
    ANIM_ITEM_SELECTED,
    ANIM_BOUNCE,
    ANIM_ERROR,
    ANIM_NUM
} AnimType;

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

// 菜单动画结构体
typedef struct MenuAnimState {
    AnimType type;              // 当前动画类型
    uint8_t is_running;         // 动画是否在执行
    uint8_t current_frame;      // 当前帧序号
    uint8_t total_frames;        // 动画总帧数
    uint8_t is_valid;           // 动画函数是否有效
    uint8_t time_per_frame;     // 帧时长（延迟）
    volatile uint8_t timer;     // 计时器(使用SysTick中断增加，同时可能在中断或主函数中访问，需要使用volatile)

    /*** 各动画所需要的参数 ***/
    int8_t *anim_func;
    int8_t step;      // 每一步滚动的行数
    int8_t start_pos; // 滚动过程中可能滚出的内容起始位置
    uint8_t direction;
    uint8_t is_tracking_the_other_side;
    uint8_t initial_index;
    uint8_t target_index;
    int8_t bias;
    MenuItem *item;
    uint8_t are_items_full;
} MenuAnimState;

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
    MenuAnimState anim_state;      // 动画状态
} MenuManager;

extern MenuManager menu_manager;
extern Language currentLanguage;

void Menu_Init(void);
void Menu_InitItem(TextID textID, MenuItem* item, void (*action)(void));
void Menu_AddChild(MenuItem* parent, MenuItem* child);
void Menu_GoBack(void);
void Menu_SelectNext(void);
void Menu_SelectPrev(void);
void Menu_ExecuteAction(void);
void Menu_Display(void);
void Menu_RunOneFrameAnim(void);

#endif
