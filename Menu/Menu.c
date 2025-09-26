#include "Menu.h"
#include "OLED.h"
#include <stdlib.h>
#include <string.h>

/***** 基本功能函数 *****/

MenuManager menu_manager;

int8_t animition_linear[] = {8, 4, 2, 2};

// 初始化菜单管理器
void Menu_Init(void)
{
    menu_manager.current_menu = NULL;
    menu_manager.current_item = NULL;
    menu_manager.depth = 0;
    menu_manager.camera_index[menu_manager.depth].start_index = 0;
    menu_manager.camera_index[menu_manager.depth].displayed_index = 0;
    menu_manager.camera_index[menu_manager.depth].selected_index = 0;
    menu_manager.mode = MODE_VERTICAL;
}

// 创建新菜单项
MenuItem *Menu_CreateItem(const char *text, void (*action)(void))
{
    MenuItem *item = (MenuItem *)malloc(sizeof(MenuItem));
    if (item != NULL)
    {
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

void Menu_InitItem(const char *text, MenuItem *item, void (*action)(void))
{
    if (item != NULL)
    {
        strncpy(item->text, text, sizeof(item->text) - 1);
        item->text[sizeof(item->text) - 1] = '\0';
        item->action = action;
        item->parent = NULL;
        item->child = NULL;
        item->next = NULL;
        item->prev = NULL;
    }
}

// 添加子菜单
void Menu_AddChild(MenuItem *parent, MenuItem *child)
{
    if (parent == NULL || child == NULL)
        return;

    child->parent = parent;

    if (parent->child == NULL)
    {
        parent->child = child;
    }
    else
    {
        MenuItem *last = parent->child;
        while (last->next != NULL)
        {
            last = last->next;
        }
        last->next = child;
        child->prev = last;
    }
}

/***** 菜单导航函数 *****/

// 进入子菜单
void Menu_EnterChild(void)
{
    if (menu_manager.current_item != NULL &&
        menu_manager.current_item->child != NULL)
    {
        menu_manager.current_menu = menu_manager.current_item;
        menu_manager.current_item = menu_manager.current_menu->child;

        menu_manager.depth++; // 增加菜单深度
    }
}

// 返回父菜单
void Menu_GoBack(void)
{
    if (menu_manager.current_menu != NULL &&
        menu_manager.current_menu->parent != NULL)
    {

        menu_manager.current_item = menu_manager.current_menu;         // 当前菜单变为当前选中项
        menu_manager.current_menu = menu_manager.current_menu->parent; // 返回父菜单

        menu_manager.depth--; // 减少菜单深度
    }
}

void Menu_RunScrollAnimition(uint8_t direction, uint8_t is_end, uint8_t is_show_new, int8_t *animition_func, uint8_t step_num, uint8_t time)
{
    int8_t step;         // 每一步滚动的行数
    int8_t start_pos;    // 新显示内容起始位置
    uint8_t reverse_pos; // 反相位置

    MenuItem *item = menu_manager.current_menu->child;
    uint8_t scroll_out_index = 0;

    // 是否触底或者触顶
    if (is_end) // 触底或触顶动画
    {
        if (direction == 0)
        {
            start_pos = 0; // 下触底，向上滚动，滚出内容起始位置为第1行

            // 定位到滚出内容
            while (item->next != NULL && (scroll_out_index < menu_manager.camera_index[menu_manager.depth].start_index))
            {
                item = item->next;
                scroll_out_index++;
            }
        }
        else
        {
            start_pos = 3 * 16; // 上触底，向下滚动，起始位置为第（4）行
            while (item->next != NULL && (scroll_out_index < menu_manager.camera_index[menu_manager.depth].start_index + 3))
            {
                item = item->next;
                scroll_out_index++;
            }
        }

        int8_t touch_end_func[] = {4, 2, -2, -4};
        for (uint8_t i = 0; i < 4; i++)
        {
            step = touch_end_func[i]; // 获取当前步进行数

            if (direction == 0)
            {
                OLED_ReverseArea(0, 3 * 16, 128, 16);                                      // 取消之前的反相效果
                OLED_Scroll(-step);                                                        // 已有内容向上滚动step行
                start_pos -= step;                                                         // 更新新内容起始位置
                OLED_ShowString(0, start_pos, item->text, OLED_8X16); // 显示滚出的内容
                OLED_ReverseArea(0, 3 * 16, 128, 16);                                      // 重新反相选中效果
                OLED_Update();
            }
            else
            {
                OLED_ReverseArea(0, 0, 128, 16);                                      // 取消之前的反相效果
                OLED_Scroll(step);                                                        // 已有内容向下滚动step行
                start_pos += step;                                                         // 更新新内容起始位置
                OLED_ShowString(0, start_pos, item->text, OLED_8X16); // 显示滚出的内容
                OLED_ReverseArea(0, 0, 128, 16);                                      // 重新反相选中效果
                OLED_Update();
            }

            HAL_Delay(time);
        }
    }
    else // 滚动动画
    {
        if (direction == 0)
        {
            start_pos = 4 * 16; // 向上滚动，起始位置为第5行
            reverse_pos = menu_manager.camera_index[menu_manager.depth].displayed_index * 16 - 16;
        }
        else
        {
            start_pos = -16; // 向下滚动，起始位置为-1行
            reverse_pos = menu_manager.camera_index[menu_manager.depth].displayed_index * 16 + 16;
        }

        for (uint8_t i = 0; i < step_num; i++)
        {
            step = animition_func[i]; // 获取当前步进行数

            if (is_show_new) // 显示新内容
            {
                if (direction == 0)
                {
                    start_pos -= step;
                    OLED_ReverseArea(0, 3 * 16, 128, 16);                                      // 取消选中反相效果
                    OLED_Scroll(-step);                                                        // 已有内容向上滚动step行
                    OLED_ShowString(0, start_pos, menu_manager.current_item->text, OLED_8X16); // 显示新内容
                    OLED_ReverseArea(0, 3 * 16, 128, 16);                                      // 重新反相选中效果
                    OLED_Update();
                }
                else
                {
                    start_pos += step;
                    OLED_ReverseArea(0, 0, 128, 16);
                    OLED_Scroll(step);
                    OLED_ShowString(0, start_pos, menu_manager.current_item->text, OLED_8X16);
                    OLED_ReverseArea(0, 0, 128, 16);
                    OLED_Update();
                }
            }
            else // 不显示新内容，仅移动选中反相框
            {
                OLED_ReverseArea(0, reverse_pos, 128, 16); // 取消之前的反相效果
                reverse_pos += (direction == 0) ? step : -step;
                OLED_ReverseArea(0, reverse_pos, 128, 16);
                OLED_Update();
            }

            HAL_Delay(time);
        }
    }
}

// 选择下一个菜单项
void Menu_SelectNext(void)
{
    // 还未触底
    if (menu_manager.current_item != NULL &&
        menu_manager.current_item->next != NULL)
    {
        menu_manager.current_item = menu_manager.current_item->next;
        menu_manager.camera_index[menu_manager.depth].selected_index++; // 实际选中索引增加

        // 如果超出显示范围，滚动显示（当前显示索引不变）
        if ((menu_manager.camera_index[menu_manager.depth].selected_index - menu_manager.camera_index[menu_manager.depth].start_index) >= 4)
        {
            menu_manager.camera_index[menu_manager.depth].start_index++; // 显示起始索引增加

            Menu_RunScrollAnimition(0, 0, 1, animition_linear, 4, 20);
        }
        else // 未超出显示范围，屏幕中显示的选中索引增加
        {
            menu_manager.camera_index[menu_manager.depth].displayed_index++; // 屏幕中显示的选中索引增加

            Menu_RunScrollAnimition(0, 0, 0, animition_linear, 4, 20);
        }

        return;
    }
    // 触底操作
    Menu_RunScrollAnimition(0, 1, 1, animition_linear, 4, 20);
}

// 选择上一个菜单项
void Menu_SelectPrev(void)
{
    // 还未触顶
    if (menu_manager.current_item != NULL &&
        menu_manager.current_item->prev != NULL)
    {
        menu_manager.current_item = menu_manager.current_item->prev;
        menu_manager.camera_index[menu_manager.depth].selected_index--; // 实际选中索引减少

        // 如果超出显示范围，滚动显示
        if (menu_manager.camera_index[menu_manager.depth].selected_index < menu_manager.camera_index[menu_manager.depth].start_index)
        {
            menu_manager.camera_index[menu_manager.depth].start_index--; // 显示起始索引减少

            Menu_RunScrollAnimition(1, 0, 1, animition_linear, 4, 20);
        }
        else
        {
            menu_manager.camera_index[menu_manager.depth].displayed_index--; // 屏幕中显示的选中索引增加

            Menu_RunScrollAnimition(1, 0, 0, animition_linear, 4, 20);
        }

        return;
    }
    // 触顶操作
    Menu_RunScrollAnimition(1, 1, 1, animition_linear, 4, 20);
}

// 执行当前菜单项的动作
void Menu_ExecuteAction(void)
{
    if (menu_manager.current_item != NULL)
    {
        if (menu_manager.current_item->action != NULL)
        {
            menu_manager.current_item->action();
        }
        else if (menu_manager.current_item->child != NULL)
        {
            Menu_EnterChild();
        }
    }
}

/***** 显示函数 *****/

// 显示当前菜单
void Menu_Display(void)
{
    // 清屏
    OLED_Clear();

    if (menu_manager.current_menu == NULL)
        return;

    // 显示菜单项
    MenuItem *item = menu_manager.current_menu->child;
    uint8_t display_start = 0;

    // 跳转到显示起始位置
    while (item != NULL && display_start < menu_manager.camera_index[menu_manager.depth].start_index)
    {
        item = item->next;
        display_start++;
    }

    // 显示4个菜单项
    for (uint8_t i = 0; i < 4 && item != NULL; i++)
    {
        OLED_ShowString(0, i * 16, item->text, OLED_8X16);
        item = item->next;
    }
    // 选中项反相
    OLED_ReverseArea(0, menu_manager.camera_index[menu_manager.depth].displayed_index * 16, 128, 16);

    OLED_Update();
}
