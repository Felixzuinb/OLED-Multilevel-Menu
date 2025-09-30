#include "Menu.h"
#include "OLED.h"

/***** 基本功能函数 *****/

MenuManager menu_manager;
Language currentLanguage; // 当前语言

// animition pointer(ap)
static int8_t ap_linear[] = {8, 4, 2, 2};
static int8_t ap_touch_end_func[] = {5, 3, 2, 1, -8, -2, -1};
static int8_t ap_error[] = {3, 2, 1, -2, -3, -2, -1, 2};

// animiton length(AL)
#define AL_LINEAR (sizeof(ap_linear) / sizeof(ap_linear[0]))
#define AL_TOUCH_END (sizeof(ap_touch_end_func) / sizeof(ap_touch_end_func[0]))
#define AL_ERROR (sizeof(ap_error) / sizeof(ap_error[0]))

/***** 静态函数声明 *****/

static uint8_t Menu_CheckAnimValid(AnimType type, int8_t *anim_func, uint8_t step_num);
static void Menu_StateSetBasic(AnimType type, int8_t *anim_func, uint8_t total_frame, uint8_t time_per_frame);

static void Menu_StartErrorAnim(int8_t *anim_func, uint8_t step_num, uint8_t time_pre_frame);
static void Menu_StartScrollAnim(uint8_t direction, int8_t *anim_func, uint8_t step_num, uint8_t time_pre_frame);
static void Menu_StartUpdateSelectedAnim(int8_t *anim_func, uint8_t step_num, uint8_t initial_index, uint8_t target_index, uint8_t time_pre_frame);
static void Menu_StartBounceAnim(uint8_t direction, uint8_t are_items_full, int8_t *anim_func, uint8_t step_num, uint8_t time_pre_frame);

static const char *getMenuItemText(const MenuItem *item);
static void refreshAllMenuItemTexts(MenuItem *item);
static void refreshMenuItemText(MenuItem *item);
static void Menu_DisplayItem(MenuMode mode);
static void Menu_EnterChild(void);

/************* 菜单基本功能函数 *************/

// 初始化菜单管理器
void Menu_Init(void)
{
    menu_manager.current_menu = NULL;
    menu_manager.current_item = NULL;
    menu_manager.root = NULL;
    menu_manager.depth = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        menu_manager.camera_index[i].start_index = 0;
        menu_manager.camera_index[i].displayed_index = 0;
        menu_manager.camera_index[i].selected_index = 0;
    }

    menu_manager.mode = MODE_VERTICAL;
    menu_manager.toward = TOWARD_NONE;
    menu_manager.text = getMenuItemText;                // 默认获取菜单文本函数
    menu_manager.refreshText = refreshAllMenuItemTexts; // 默认刷新菜单文本函数
}

void Menu_InitItem(TextID textID, MenuItem *item, void (*action)(void))
{
    if (item != NULL)
    {
        item->text_id = textID;
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

/************* 菜单导航函数 *************/

// 进入子菜单
static void Menu_EnterChild(void)
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
    // Menu_RunErrorAnimition(ap_error, AL_ERROR, 20);
    Menu_StartErrorAnim(ap_error, AL_ERROR, 20);
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

            // Menu_RunScrollAnimition(UP, 1, ap_linear, AL_LINEAR, 20);
            Menu_StartScrollAnim(UP, ap_linear, AL_LINEAR, 20);
        }
        else // 未超出显示范围，屏幕中显示的选中索引增加
        {
            menu_manager.camera_index[menu_manager.depth].displayed_index++; // 屏幕中显示的选中索引增加

            // Menu_RunScrollAnimition(UP, 0, ap_linear, AL_LINEAR, 20);
            uint8_t target_index = menu_manager.camera_index[menu_manager.depth].displayed_index;
            uint8_t initial_index = target_index - 1;
            Menu_StartUpdateSelectedAnim(ap_linear, AL_LINEAR, initial_index, target_index, 20);
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
        // Menu_RunEndBounceAnimition(UP, 0, ap_touch_end_func, AL_TOUCH_END, 20); // 不追踪另一侧
        Menu_StartBounceAnim(UP, ITEM_UNFULL, ap_touch_end_func, AL_TOUCH_END, 20);
    }
    else
    {
        // Menu_RunEndBounceAnimition(UP, 1, ap_touch_end_func, AL_TOUCH_END, 20);
        Menu_StartBounceAnim(UP, ITEM_FULL, ap_touch_end_func, AL_TOUCH_END, 20);
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

            Menu_StartScrollAnim(DOWN, ap_linear, AL_LINEAR, 20);
        }
        else
        {
            menu_manager.camera_index[menu_manager.depth].displayed_index--; // 屏幕中显示的选中索引增加

            uint8_t target_index = menu_manager.camera_index[menu_manager.depth].displayed_index;
            int8_t initial_index = target_index + 1;
            Menu_StartUpdateSelectedAnim(ap_linear, AL_LINEAR, initial_index, target_index, 20);
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
        Menu_StartBounceAnim(DOWN, ITEM_UNFULL, ap_touch_end_func, AL_TOUCH_END, 20);
    }
    else
    {
        Menu_StartBounceAnim(DOWN, ITEM_FULL, ap_touch_end_func, AL_TOUCH_END, 20);
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
        else
        {
            Menu_EnterChild();
        }
    }
}

/************* 菜单功能函数 *************/

// 获取菜单项的当前语言文本
static const char *getMenuItemText(const MenuItem *item)
{
    return item ? item->text : "";
}

// 刷新菜单项文本（当语言切换时调用）
static void refreshMenuItemText(MenuItem *item)
{
    if (item == NULL || currentLanguage >= LANG_COUNT)
        return; // 访问意外

    if (item->text_id >= TEXT_COUNT)
    {
        item->text = textMap[currentLanguage][TID_NULL]; // 无效时设为空字符串
        return;
    }
    item->text = textMap[currentLanguage][item->text_id];
}

// 刷新所有菜单项文本（当语言切换时调用）
static void refreshAllMenuItemTexts(MenuItem *root)
{
    if (root == NULL)
        return;
    refreshMenuItemText(root); // 刷新当前项

    if (root->next != NULL)
        refreshAllMenuItemTexts(root->next); // 递归刷新兄弟项

    if (root->child != NULL)
        refreshAllMenuItemTexts(root->child); // 递归刷新子菜单
}

// 显示当前菜单
void Menu_Display(void)
{
    // 清屏
    OLED_Clear();

    if (menu_manager.current_menu == NULL)
        return;

    // 显示菜单项
    Menu_DisplayItem(menu_manager.mode);

    // 更新选中项反相效果
    switch (menu_manager.toward)
    {
    case TOWARD_CHILD:
    {
        uint8_t initial_index = menu_manager.camera_index[menu_manager.depth - 1].displayed_index; // 初始父菜单反相位置

        // 移动到当前选中项（进入子菜单默认第一项）
        // Menu_RunUpdateReverseAnimition(ap_linear, AL_LINEAR, initial_index, 0, 20);
        Menu_StartUpdateSelectedAnim(ap_linear, AL_LINEAR, initial_index, 0, 20);

        menu_manager.toward = TOWARD_NONE; // 重置方向
        break;
    }

    case TOWARD_PARENT:
    {
        uint8_t initial_index = menu_manager.camera_index[menu_manager.depth + 1].displayed_index; // 初始子菜单反相位置
        uint8_t target_index = menu_manager.camera_index[menu_manager.depth].displayed_index;      // 目标父菜单反相位置

        // Menu_RunUpdateReverseAnimition(ap_linear, AL_LINEAR, initial_index, target_index, 20);
        Menu_StartUpdateSelectedAnim(ap_linear, AL_LINEAR, initial_index, target_index, 20);

        menu_manager.toward = TOWARD_NONE; // 重置方向
        // 重置子菜单镜头位置
        menu_manager.camera_index[menu_manager.depth + 1].start_index = 0;
        menu_manager.camera_index[menu_manager.depth + 1].displayed_index = 0;
        menu_manager.camera_index[menu_manager.depth + 1].selected_index = 0;
        break;
    }

    case TOWARD_NONE: // 只应该在初始化时出现
    {
        uint8_t reverse_pos = menu_manager.camera_index[menu_manager.depth].displayed_index * 16;
        OLED_ReverseArea(0, reverse_pos, 128, 16);
        OLED_Update();
        break;
    }

    default:
        break;
    }
}

/************* 动画配置函数 *************/

static void Menu_StartScrollAnim(uint8_t direction, int8_t *anim_func, uint8_t step_num, uint8_t time_pre_frame)
{
    MenuAnimState *anim_state = &menu_manager.anim_state;
    anim_state->is_valid = Menu_CheckAnimValid(ANIM_MENU_SLIDE, anim_func, step_num);

    if (anim_state->is_valid == A_VALID)
    {
        Menu_StateSetBasic(ANIM_MENU_SLIDE, anim_func, step_num, time_pre_frame);

        anim_state->direction = direction;
        if (direction == 0)
            anim_state->start_pos = 4 * 16; // 向上滚动，起始位置为第5行
        else
            anim_state->start_pos = -16; // 向下滚动，起始位置为-1行
    }

    Menu_RunOneFrameAnim();
}

static void Menu_StartBounceAnim(uint8_t direction, uint8_t are_items_full, int8_t *anim_func, uint8_t step_num, uint8_t time_pre_frame)
{
    MenuAnimState *anim_state = &menu_manager.anim_state;
    anim_state->is_valid = Menu_CheckAnimValid(ANIM_BOUNCE, anim_func, step_num);

    if (anim_state->is_valid == A_VALID)
    {
        Menu_StateSetBasic(ANIM_BOUNCE, anim_func, step_num, time_pre_frame);

        anim_state->direction = direction;
        anim_state->are_items_full = are_items_full;

        MenuItem *item = menu_manager.current_menu->child;
        uint8_t item_index = 0;

        if (direction == 0)
        {
            anim_state->start_pos = 0;

            if (are_items_full)
            {
                // 定位到滚出内容
                while (item->next != NULL && (item_index < menu_manager.camera_index[menu_manager.depth].start_index))
                {
                    item = item->next;
                    item_index++;
                }
                anim_state->item = item;
            }
            else
            {
                // 定位到当前选中项位置
                while (item->next != NULL && (item_index < menu_manager.camera_index[menu_manager.depth].selected_index))
                {
                    item = item->next;
                    item_index++;
                }

                item = menu_manager.current_menu->child; // 重置item指针为第一个菜单项
                anim_state->item = item;
                anim_state->initial_index = item_index;
            }
        }

        else
        {
            if (are_items_full)
            {
                anim_state->start_pos = 3 * 16; // 上触底，向下滚动，起始位置为第（4）行
                while (item->next != NULL && (item_index < menu_manager.camera_index[menu_manager.depth].start_index + 3))
                {
                    item = item->next;
                    item_index++;
                }
                anim_state->item = item;
            }
        }

        Menu_RunOneFrameAnim();
    }
}

static void Menu_StartErrorAnim(int8_t *anim_func, uint8_t step_num, uint8_t time_pre_frame)
{
    MenuAnimState *anim_state = &menu_manager.anim_state;
    anim_state->is_valid = Menu_CheckAnimValid(ANIM_ERROR, anim_func, step_num);

    if (anim_state->is_valid == A_VALID)
    {
        Menu_StateSetBasic(ANIM_ERROR, anim_func, step_num, time_pre_frame);

        anim_state->start_pos = menu_manager.camera_index[menu_manager.depth].displayed_index * 16;

        Menu_RunOneFrameAnim(); // 直接运行第一帧，避免首帧延迟
    }
    else
    {
        return;
    }
}

static void Menu_StartUpdateSelectedAnim(int8_t *anim_func, uint8_t step_num, uint8_t initial_index, uint8_t target_index, uint8_t time_pre_frame)
{
    MenuAnimState *anim_state = &menu_manager.anim_state;
    anim_state->is_valid = Menu_CheckAnimValid(ANIM_ITEM_SELECTED, anim_func, step_num);
    if (anim_state->is_running)
        return; // 正在运行则不开始新动画

    if (anim_state->is_valid == A_VALID)
    {
        Menu_StateSetBasic(ANIM_ITEM_SELECTED, anim_func, step_num, time_pre_frame);

        anim_state->initial_index = initial_index;
        anim_state->target_index = target_index;
        anim_state->start_pos = initial_index * 16;
        anim_state->bias = (target_index - initial_index) * 16;

        // 执行初始帧, 避免首帧延迟
        if (menu_manager.toward != TOWARD_NONE)
        {
            OLED_ReverseArea(0, initial_index * 16, 128, 16); // 保持之前的反相效果
            OLED_Update();

            anim_state->timer = 0; // 计时器置零
        }
        else
        {
            Menu_RunOneFrameAnim();
        }
    }
}

static void Menu_DisplayItem(MenuMode mode)
{
    switch (mode)
    {
    case MODE_VERTICAL:
    {
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
        break;
    }

    case MODE_HORIZONTAL:
        break;
    default:
        break;
    }
}

/************* 动画帧函数 *************/

static uint8_t Menu_CheckAnimValid(AnimType type, int8_t *anim_func, uint8_t step_num)
{
    // 检查动画函数有效性
    int8_t total_movement = 0;
    int8_t valid_total_movement;
    for (uint8_t i = 0; i < step_num; i++)
    {
        total_movement += anim_func[i];
    }

    switch (type)
    {
        /*** 均为0（最终位置与初始位置相同） ***/
    case ANIM_ERROR:
    case ANIM_BOUNCE:
    {
        valid_total_movement = 0;
        break;
    }
    /*** 均为16（移动一行文本宽度） ***/
    case ANIM_ITEM_SELECTED:
    case ANIM_MENU_SLIDE:
    {
        valid_total_movement = 16;
        break;
    }
    case ANIM_NONE:
        return A_INVALID;

    default:
        return A_INVALID;
    }

    if (total_movement != valid_total_movement)
    {
        return A_INVALID;
    }
    return A_VALID;
}

static void Menu_StateSetBasic(AnimType type, int8_t *anim_func, uint8_t total_frame, uint8_t time_per_frame)
{
    MenuAnimState *anim_state = &menu_manager.anim_state;

    anim_state->type = type;
    anim_state->current_frame = 0;
    anim_state->total_frames = total_frame;
    anim_state->time_per_frame = time_per_frame;
    anim_state->is_running = 1;
    anim_state->anim_func = anim_func;
    anim_state->timer = 0;
}

void Menu_RunOneFrameAnim(void)
{
    MenuAnimState *anim_state = &menu_manager.anim_state;
    // 是否已经有动画在播放
    if (!anim_state->is_running)
        return;

    // 帧时长未到则不执行下一帧
    if (anim_state->timer < anim_state->time_per_frame)
        return;

    // 根据动画类型播放对应帧
    switch (anim_state->type)
    {
    case ANIM_MENU_SLIDE:
    {
        // 1. 判断当前帧是否超出总帧数（动画结束）
        if (anim_state->current_frame >= anim_state->total_frames)
        {
            // 动画结束：重置状态
            anim_state->is_running = 0;
            anim_state->type = ANIM_NONE;
            anim_state->current_frame = 0;
            return;
        }

        int8_t step = anim_state->anim_func[anim_state->current_frame]; // 当前步长

        if (anim_state->direction == 0)
        {

            anim_state->start_pos -= step;
            OLED_ReverseArea(0, 3 * 16, 128, 16);                                      // 取消选中反相效果
            OLED_Scroll(-step);                                                        // 已有内容向上滚动step行
            OLED_ShowString(0, anim_state->start_pos, menu_manager.current_item->text, OLED_8X16); // 显示新内容
            OLED_ReverseArea(0, 3 * 16, 128, 16);                                      // 重新反相选中效果
            OLED_Update();
        }
        else
        {
            anim_state->start_pos += step;
            OLED_ReverseArea(0, 0, 128, 16);
            OLED_Scroll(step);
            OLED_ShowString(0, anim_state->start_pos, menu_manager.current_item->text, OLED_8X16);
            OLED_ReverseArea(0, 0, 128, 16);
            OLED_Update();
        }

        anim_state->current_frame++;
        anim_state->timer = 0; // 计时器置零
        break;
    }

    case ANIM_ITEM_SELECTED:
    {
        // 1. 判断当前帧是否超出总帧数（动画结束）
        if (anim_state->current_frame >= anim_state->total_frames)
        {
            // 动画结束：重置状态
            anim_state->is_running = 0;
            anim_state->type = ANIM_NONE;
            anim_state->current_frame = 0;
            return;
        }

        int8_t step = anim_state->anim_func[anim_state->current_frame] * anim_state->bias / 16; // 获取当前步进行数
        OLED_ReverseArea(0, anim_state->start_pos, 128, 16);                                    // 取消之前的反向效果
        anim_state->start_pos += step;                                                          // 更新反相位置
        OLED_ReverseArea(0, anim_state->start_pos, 128, 16);                                    // 重新反相选中效果
        OLED_Update();

        anim_state->current_frame++;
        anim_state->timer = 0; // 计时器置零
        break;
    }
    case ANIM_BOUNCE:
    {
        // 1. 判断当前帧是否超出总帧数（动画结束）
        if (anim_state->current_frame >= anim_state->total_frames)
        {
            // 动画结束：重置状态
            anim_state->is_running = 0;
            anim_state->type = ANIM_NONE;
            anim_state->current_frame = 0;
            return;
        }

        int8_t step = anim_state->anim_func[anim_state->current_frame]; // 获取当前步进行数
        int8_t selected_pos = anim_state->initial_index * 16;
        MenuItem *item = anim_state->item;

        if (anim_state->direction == 0) // 下触底，向上滚动
        {

            if (anim_state->are_items_full == 0) // 下触底时，不跟踪另一侧内容
            {

                OLED_ReverseArea(0, selected_pos, 128, 16);                       // 取消之前的反相效果
                OLED_Scroll(-step);                                               // 已有内容向上滚动step行
                anim_state->start_pos -= step;                                    // 更新新内容起始位置
                OLED_ShowString(0, anim_state->start_pos, item->text, OLED_8X16); // 显示滚出的内容
                OLED_ReverseArea(0, selected_pos, 128, 16);                       // 重新反相选中效果
                OLED_Update();
            }
            else
            {
                OLED_ReverseArea(0, 3 * 16, 128, 16);                             // 取消之前的反相效果
                OLED_Scroll(-step);                                               // 已有内容向上滚动step行
                anim_state->start_pos -= step;                                    // 更新新内容起始位置
                OLED_ShowString(0, anim_state->start_pos, item->text, OLED_8X16); // 显示滚出的内容
                OLED_ReverseArea(0, 3 * 16, 128, 16);                             // 重新反相选中效果
                OLED_Update();
            }
        }
        else
        {
            if (anim_state->are_items_full == 0) // 上触顶时，不跟踪另一侧内容
            {
                OLED_ReverseArea(0, 0, 128, 16); // 取消之前的反相效果
                OLED_Scroll(step);               // 已有内容向下滚动step行
                OLED_ReverseArea(0, 0, 128, 16); // 重新反相选中效果
                OLED_Update();
            }
            else // 上触顶时，跟踪另一侧内容
            {
                OLED_ReverseArea(0, 0, 128, 16);                                  // 取消之前的反相效果
                OLED_Scroll(step);                                                // 已有内容向下滚动step行
                anim_state->start_pos += step;                                    // 更新新内容起始位置
                OLED_ShowString(0, anim_state->start_pos, item->text, OLED_8X16); // 显示滚出的内容
                OLED_ReverseArea(0, 0, 128, 16);                                  // 重新反相选中效果
                OLED_Update();
            }
        }

        anim_state->current_frame++;
        anim_state->timer = 0; // 计时器置零
        break;
    }
    case ANIM_ERROR:
    {
        // 1. 判断当前帧是否超出总帧数（动画结束）
        if (anim_state->current_frame >= anim_state->total_frames)
        {
            // 动画结束：重置状态
            anim_state->is_running = 0;
            anim_state->type = ANIM_NONE;
            anim_state->current_frame = 0;
            return;
        }

        int8_t step = anim_state->anim_func[anim_state->current_frame]; // 当前步长

        OLED_ReverseArea(0, anim_state->start_pos, 128, 16); // 取消之前的反相效果
        anim_state->start_pos += step;
        OLED_ReverseArea(0, anim_state->start_pos, 128, 16);
        OLED_Update();

        // HAL_Delay(20);

        anim_state->current_frame++;
        anim_state->timer = 0; // 计时器置零
        break;
    }

    case ANIM_NONE:
        break;
    default:
        break;
    }
}
