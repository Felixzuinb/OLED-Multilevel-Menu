static void Menu_RunScrollAnimition(uint8_t direction, uint8_t is_show_new, int8_t *animition_func, uint8_t step_num, uint8_t time);
static void Menu_RunEndBounceAnimition(uint8_t direction, uint8_t is_tracking_the_other_side, int8_t *animition_func, uint8_t step_num, uint8_t time);
static void Menu_RunErrorAnimition(int8_t *animition_func, uint8_t step_num, uint8_t time);
static void Menu_RunUpdateReverseAnimition(int8_t *animition_func, uint8_t step_num, uint8_t initial_index, uint8_t target_index, uint8_t time);


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

    if (menu_manager.toward != TOWARD_NONE)
    {
        OLED_ReverseArea(0, initial_index * 16, 128, 16); // 保持之前的反相效果
        OLED_Update();
    }

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

