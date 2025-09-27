#include "Menu.h"
#include "OLED.h"
#include <stdlib.h>
#include <string.h>

/***** 基本功能函数 *****/

MenuManager menu_manager;

// animition pointer(ap)
static int8_t ap_linear[] = {8, 4, 2, 2};
static int8_t ap_touch_end_func[] = {5, 3, 2, 1, -8, -2, -1};
static int8_t ap_error[] = {3, 2, 1, -2, -3, -2, -1, 2};

// animiton length(AL)
#define AL_LINEAR (sizeof(ap_linear) / sizeof(ap_linear[0]))
#define AL_TOUCH_END (sizeof(ap_touch_end_func) / sizeof(ap_touch_end_func[0]))
#define AL_ERROR (sizeof(ap_error) / sizeof(ap_error[0]))

// 初始化菜单管理器
void Menu_Init(void)
{
    menu_manager.current_menu = NULL;
    menu_manager.current_item = NULL;
    menu_manager.depth = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        menu_manager.camera_index[i].start_index = 0;
        menu_manager.camera_index[i].displayed_index = 0;
        menu_manager.camera_index[i].selected_index = 0;
    }

    menu_manager.mode = MODE_VERTICAL;
    menu_manager.toward = TOWARD_NONE;
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
    // 成功进入子菜单
    if (menu_manager.current_item != NULL &&
        menu_manager.current_item->child != NULL)
    {
        menu_manager.current_menu = menu_manager.current_item;
        menu_manager.current_item = menu_manager.current_menu->child;

        menu_manager.depth++; // 增加菜单深度

        menu_manager.toward = TOWARD_CHILD;
        Menu_Display();
        return;
    }

    // 进入子菜单失败(播放错误动画)
    Menu_RunErrorAnimition(ap_error, AL_ERROR, 20);
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

        menu_manager.toward = TOWARD_PARENT;
        Menu_Display();
    }
}

// 选择下一个菜单项
void Menu_SelectNext(void)
{
    if (menu_manager.current_item == NULL)
        return;

    // 还未触底
    if (menu_manager.current_item->next != NULL)
    {
        menu_manager.current_item = menu_manager.current_item->next;
        menu_manager.camera_index[menu_manager.depth].selected_index++; // 实际选中索引增加

        // 如果超出显示范围，滚动显示（当前显示索引不变）
        if ((menu_manager.camera_index[menu_manager.depth].selected_index - menu_manager.camera_index[menu_manager.depth].start_index) >= 4)
        {
            menu_manager.camera_index[menu_manager.depth].start_index++; // 显示起始索引增加

            Menu_RunScrollAnimition(0, 1, ap_linear, AL_LINEAR, 20);
        }
        else // 未超出显示范围，屏幕中显示的选中索引增加
        {
            menu_manager.camera_index[menu_manager.depth].displayed_index++; // 屏幕中显示的选中索引增加

            Menu_RunScrollAnimition(0, 0, ap_linear, AL_LINEAR, 20);
        }

        return;
    }
    // 触底操作
    uint8_t item_count = 0;
    MenuItem *item = menu_manager.current_menu->child;
    while (item != NULL)
    {
        item_count++;
        item = item->next;
    }

    // 检查上触顶时，菜单项数是否少于4个
    if (item_count < 4)
    {
        Menu_RunEndBounceAnimition(0, 0, ap_touch_end_func, AL_TOUCH_END, 20); // 不追踪另一侧
    }
    else
    {
        Menu_RunEndBounceAnimition(0, 1, ap_touch_end_func, AL_TOUCH_END, 20);
    }
}

// 选择上一个菜单项
void Menu_SelectPrev(void)
{
    if (menu_manager.current_item == NULL)
        return;

    // 还未触顶
    if (menu_manager.current_item->prev != NULL)
    {
        menu_manager.current_item = menu_manager.current_item->prev;
        menu_manager.camera_index[menu_manager.depth].selected_index--; // 实际选中索引减少

        // 如果超出显示范围，滚动显示
        if (menu_manager.camera_index[menu_manager.depth].selected_index < menu_manager.camera_index[menu_manager.depth].start_index)
        {
            menu_manager.camera_index[menu_manager.depth].start_index--; // 显示起始索引减少

            Menu_RunScrollAnimition(1, 1, ap_linear, AL_LINEAR, 20);
        }
        else
        {
            menu_manager.camera_index[menu_manager.depth].displayed_index--; // 屏幕中显示的选中索引增加

            Menu_RunScrollAnimition(1, 0, ap_linear, AL_LINEAR, 20);
        }

        return;
    }
    // 触顶操作
    uint8_t item_count = 0;
    MenuItem *item = menu_manager.current_menu->child;
    while (item != NULL)
    {
        item_count++;
        item = item->next;
    }
    // 检查下触顶时，菜单项数是否少于4个
    if (item_count < 4)
    {
        Menu_RunEndBounceAnimition(1, 0, ap_touch_end_func, AL_TOUCH_END, 20); // 不追踪另一侧
    }
    else
    {
        Menu_RunEndBounceAnimition(1, 1, ap_touch_end_func, AL_TOUCH_END, 20);
    }
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

    // 更新选中项反相效果
    if (menu_manager.toward == TOWARD_CHILD)
    {
        uint8_t initial_index = menu_manager.camera_index[menu_manager.depth - 1].displayed_index; // 初始父菜单反相位置
        if (initial_index != 0)                                                                    // 父菜单选中项不是第一行
        {
            OLED_ReverseArea(0, initial_index * 16, 128, 16); // 保持之前的反相效果
            OLED_Update();

            // 移动到当前选中项（进入子菜单默认第一项）
            Menu_RunUpdateReverseAnimition(ap_linear, AL_LINEAR, initial_index, 0, 20);
        }

        menu_manager.toward = TOWARD_NONE; // 重置方向
    }
    else if (menu_manager.toward == TOWARD_PARENT)
    {
        uint8_t initial_index = menu_manager.camera_index[menu_manager.depth + 1].displayed_index; // 初始子菜单反相位置
        uint8_t target_index = menu_manager.camera_index[menu_manager.depth].displayed_index;      // 目标父菜单反相位置

        if (initial_index != target_index) // 父菜单选中项不是当前选中项
        {
            OLED_ReverseArea(0, initial_index * 16, 128, 16); // 保持之前的反相效果
            OLED_Update();

            Menu_RunUpdateReverseAnimition(ap_linear, AL_LINEAR, initial_index, target_index, 20);
        }

        menu_manager.toward = TOWARD_NONE; // 重置方向
        // 重置子菜单镜头位置
        menu_manager.camera_index[menu_manager.depth + 1].start_index = 0;
        menu_manager.camera_index[menu_manager.depth + 1].displayed_index = 0;
        menu_manager.camera_index[menu_manager.depth + 1].selected_index = 0;
    }
    else
    {
        uint8_t reverse_pos = menu_manager.camera_index[menu_manager.depth].displayed_index * 16;
        OLED_ReverseArea(0, reverse_pos, 128, 16);
        OLED_Update();
    }
}

/***** 动画函数 *****/

static void Menu_RunScrollAnimition(uint8_t direction, uint8_t is_show_new, int8_t *animition_func, uint8_t step_num, uint8_t time)
{
    // 检查动画函数有效性
    int8_t total_movement = 0;
    for (uint8_t i = 0; i < step_num; i++)
    {
        total_movement += animition_func[i];
    }
    if (total_movement != 16)
    {
        return; // 动画函数无效，直接返回
    }

    int8_t step;      // 每一步滚动的行数
    int8_t start_pos; // 新显示内容起始位置

    if (direction == 0)
        start_pos = 4 * 16; // 向上滚动，起始位置为第5行
    else
        start_pos = -16; // 向下滚动，起始位置为-1行

    if (is_show_new) // 显示新内容
    {
        for (uint8_t i = 0; i < step_num; i++)
        {
            step = animition_func[i]; // 获取当前步进行数
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

            HAL_Delay(time);
        }
    }
    else // 不显示新内容，仅移动选中反相框
    {
        uint8_t target_index = menu_manager.camera_index[menu_manager.depth].displayed_index;
        uint8_t initial_index = (direction == 0) ? (target_index - 1) : (target_index + 1);
        Menu_RunUpdateReverseAnimition(ap_linear, AL_LINEAR, initial_index, target_index, 20);
    }
}

static void Menu_RunEndBounceAnimition(uint8_t direction, uint8_t is_tracking_the_other_side, int8_t *animition_func, uint8_t step_num, uint8_t time)
{
    // 检查动画函数有效性
    int8_t total_movement = 0;
    for (uint8_t i = 0; i < step_num; i++)
    {
        total_movement += animition_func[i];
    }
    if (total_movement != 0)
    {
        return; // 动画函数无效，直接返回
    }

    int8_t step;      // 每一步滚动的行数
    int8_t start_pos; // 滚动过程中可能滚出的内容起始位置

    MenuItem *item = menu_manager.current_menu->child;
    uint8_t item_index = 0;

    if (direction == 0) // 下触底，向上滚动
    {
        start_pos = 0; // 滚出内容起始位置为第1行

        if (is_tracking_the_other_side == 0) // 下触底时，不跟踪另一侧内容
        {
            // 定位到当前选中项位置
            while (item->next != NULL && (item_index < menu_manager.camera_index[menu_manager.depth].selected_index))
            {
                item = item->next;
                item_index++;
            }

            item = menu_manager.current_menu->child; // 重置item指针为第一个菜单项

            for (uint8_t i = 0; i < step_num; i++)
            {
                step = animition_func[i]; // 获取当前步进行数

                OLED_ReverseArea(0, item_index * 16, 128, 16);        // 取消之前的反相效果
                OLED_Scroll(-step);                                   // 已有内容向上滚动step行
                start_pos -= step;                                    // 更新新内容起始位置
                OLED_ShowString(0, start_pos, item->text, OLED_8X16); // 显示滚出的内容
                OLED_ReverseArea(0, item_index * 16, 128, 16);        // 重新反相选中效果
                OLED_Update();

                HAL_Delay(time);
            }
        }
        else
        {
            // 定位到滚出内容
            while (item->next != NULL && (item_index < menu_manager.camera_index[menu_manager.depth].start_index))
            {
                item = item->next;
                item_index++;
            }

            for (uint8_t i = 0; i < step_num; i++)
            {
                step = animition_func[i]; // 获取当前步进行数

                OLED_ReverseArea(0, 3 * 16, 128, 16);                 // 取消之前的反相效果
                OLED_Scroll(-step);                                   // 已有内容向上滚动step行
                start_pos -= step;                                    // 更新新内容起始位置
                OLED_ShowString(0, start_pos, item->text, OLED_8X16); // 显示滚出的内容
                OLED_ReverseArea(0, 3 * 16, 128, 16);                 // 重新反相选中效果
                OLED_Update();

                HAL_Delay(time);
            }
        }
    }
    else
    {
        if (is_tracking_the_other_side == 0) // 上触顶时，不跟踪另一侧内容
        {
            for (uint8_t i = 0; i < step_num; i++)
            {
                step = animition_func[i]; // 获取当前步进行数

                OLED_ReverseArea(0, 0, 128, 16); // 取消之前的反相效果
                OLED_Scroll(step);               // 已有内容向下滚动step行
                OLED_ReverseArea(0, 0, 128, 16); // 重新反相选中效果
                OLED_Update();

                HAL_Delay(time);
            }
        }
        else // 上触顶时，跟踪另一侧内容
        {
            start_pos = 3 * 16; // 上触底，向下滚动，起始位置为第（4）行
            while (item->next != NULL && (item_index < menu_manager.camera_index[menu_manager.depth].start_index + 3))
            {
                item = item->next;
                item_index++;
            }

            for (uint8_t i = 0; i < step_num; i++)
            {
                step = animition_func[i]; // 获取当前步进行数

                OLED_ReverseArea(0, 0, 128, 16);                      // 取消之前的反相效果
                OLED_Scroll(step);                                    // 已有内容向下滚动step行
                start_pos += step;                                    // 更新新内容起始位置
                OLED_ShowString(0, start_pos, item->text, OLED_8X16); // 显示滚出的内容
                OLED_ReverseArea(0, 0, 128, 16);                      // 重新反相选中效果
                OLED_Update();

                HAL_Delay(time);
            }
        }
    }
}

static void Menu_RunErrorAnimition(int8_t *animition_func, uint8_t step_num, uint8_t time)
{
    // 检查动画函数有效性
    int8_t total_movement = 0;
    for (uint8_t i = 0; i < step_num; i++)
    {
        total_movement += animition_func[i];
    }
    if (total_movement != 0)
    {
        return; // 动画函数无效，直接返回
    }

    int8_t step;         // 每一步滚动的行数
    uint8_t reverse_pos; // 反相位置

    reverse_pos = menu_manager.camera_index[menu_manager.depth].displayed_index * 16;

    for (uint8_t i = 0; i < step_num; i++)
    {
        step = animition_func[i]; // 获取当前步进行数

        OLED_ReverseArea(0, reverse_pos, 128, 16); // 取消之前的反相效果
        reverse_pos += step;
        OLED_ReverseArea(0, reverse_pos, 128, 16);
        OLED_Update();

        HAL_Delay(time);
    }
}

static void Menu_RunUpdateReverseAnimition(int8_t *animition_func, uint8_t step_num, uint8_t initial_index, uint8_t target_index, uint8_t time)
{
    // 检查动画函数有效性
    int8_t total_movement = 0;
    for (uint8_t i = 0; i < step_num; i++)
    {
        total_movement += animition_func[i];
    }
    if (total_movement != 16)
    {
        return; // 动画函数无效，直接返回
    }

    int8_t step;         // 每一步滚动的行数
    uint8_t reverse_pos; // 反相位置

    reverse_pos = initial_index * 16; // 记录当前的反相位置

    int8_t bias = (target_index - initial_index) * 16; // 初始选中项到目标选中项的偏移

    for (uint8_t i = 0; i < step_num; i++)
    {
        step = animition_func[i] * bias / 16;      // 获取当前步进行数
        OLED_ReverseArea(0, reverse_pos, 128, 16); // 取消之前的反向效果
        reverse_pos += step;                       // 更新反相位置
        OLED_ReverseArea(0, reverse_pos, 128, 16); // 重新反相选中效果
        OLED_Update();

        HAL_Delay(time);
    }
}
