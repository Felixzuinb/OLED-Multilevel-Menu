#include "Menu.h"
#include "OLED.h"
#include <stdlib.h>
#include <string.h>

/***** 基本功能函数 *****/

MenuManager menu_manager;

// 初始化菜单管理器
void Menu_Init(void) {
    menu_manager.current_menu = NULL;
    menu_manager.current_item = NULL;
    menu_manager.display_start = 0;
    menu_manager.selected_index = 0;
    menu_manager.mode = MODE_VERTICAL;
}

// 创建新菜单项
MenuItem* Menu_CreateItem(const char* text, void (*action)(void)) {
    MenuItem* item = (MenuItem*)malloc(sizeof(MenuItem));
    if (item != NULL) {
        strncpy(item->text, text, sizeof(item->text) - 1);
        item->text[sizeof(item->text) - 1] = '\0';
        item->action = action;
        item->parent = NULL;
        item->child = NULL;
        item->next = NULL;
        item->prev = NULL;
    }
    return item;
}

MenuItem* Menu_InitItem(const char* text, MenuItem* item, void (*action)(void)) {
    if (item != NULL) {
        strncpy(item->text, text, sizeof(item->text) - 1);
        item->text[sizeof(item->text) - 1] = '\0';
        item->action = action;
        item->parent = NULL;
        item->child = NULL;
        item->next = NULL;
        item->prev = NULL;
    }
    return item;
}

// 添加子菜单
void Menu_AddChild(MenuItem* parent, MenuItem* child) {
    if (parent == NULL || child == NULL) return;
    
    child->parent = parent;
    
    if (parent->child == NULL) {
        parent->child = child;
    } else {
        MenuItem* last = parent->child;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = child;
        child->prev = last;
    }
}

/***** 菜单导航函数 *****/

// 进入子菜单
void Menu_EnterChild(void) {
    if (menu_manager.current_item != NULL && 
        menu_manager.current_item->child != NULL) {
        menu_manager.current_menu = menu_manager.current_item;
        menu_manager.current_item = menu_manager.current_menu->child;
        menu_manager.display_start = 0;
        menu_manager.selected_index = 0;
    }
}

// 返回父菜单
void Menu_GoBack(void) {
    if (menu_manager.current_menu != NULL && 
        menu_manager.current_menu->parent != NULL) {
        // 找到在父菜单中的位置
        MenuItem* temp = menu_manager.current_menu->parent->child;
        uint8_t index = 0;
        while (temp != NULL && temp != menu_manager.current_menu) {
            temp = temp->next;
            index++;
        }
        
        menu_manager.current_item = menu_manager.current_menu;
        menu_manager.current_menu = menu_manager.current_menu->parent;
        menu_manager.selected_index = index;
        
        // 调整显示起始位置
        if (menu_manager.selected_index >= menu_manager.display_start + 4) {
            menu_manager.display_start = menu_manager.selected_index - 3;
        } else if (menu_manager.selected_index < menu_manager.display_start) {
            menu_manager.display_start = menu_manager.selected_index;
        }
    }
}

// 选择下一个菜单项
void Menu_SelectNext(void) {
    if (menu_manager.current_item != NULL && 
        menu_manager.current_item->next != NULL) {
        menu_manager.current_item = menu_manager.current_item->next;
        menu_manager.selected_index++;
        
        // 如果超出显示范围，滚动显示
        if (menu_manager.selected_index >= menu_manager.display_start + 4) {
            menu_manager.display_start++;
        }
    }
}

// 选择上一个菜单项
void Menu_SelectPrev(void) {
    if (menu_manager.current_item != NULL && 
        menu_manager.current_item->prev != NULL) {
        menu_manager.current_item = menu_manager.current_item->prev;
        menu_manager.selected_index--;
        
        // 如果超出显示范围，滚动显示
        if (menu_manager.selected_index < menu_manager.display_start) {
            menu_manager.display_start--;
        }
    }
}

// 执行当前菜单项的动作
void Menu_ExecuteAction(void) {
    if (menu_manager.current_item != NULL) {
        if (menu_manager.current_item->action != NULL) {
            menu_manager.current_item->action();
        } else if (menu_manager.current_item->child != NULL) {
            Menu_EnterChild();
        }
    }
}

/***** 显示函数 *****/

// 显示当前菜单
void Menu_Display(void) {
    // 清屏
    OLED_Clear();
    
    
    
    OLED_Update();
}
