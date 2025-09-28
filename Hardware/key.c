#include "key.h"

unsigned char Key_triggered = 10;   // 默认未触发

// 定时器+状态机

typedef struct Button_s //_s代表结构体标签
{
    ButtonState_e state;               // 按键当前状态
    uint8_t physical_pin_is_pressed;   // 物理引脚状态： 1 按下 0 释放
    uint16_t state_timer_ticks;        // 当前状态持续事件计时器（单位： 定时器中断周期 tick）
    uint8_t button_id;                 // 按键对应id
    uint8_t double_click_triggered;    // 双击已触发标志： 确保双击释放不会触发单击
    uint8_t long_press_triggered;      // 长按已触发标志： 确保长按事件只会执行一次
    BtnCallback_t callback[BTN_E_NUM]; // 不同事件的回调函数数组
} Button_t;                            //_t,typedef代表类型名别称

Button_t buttonArray[KEYNUM];


/***************
 * 使用宏生成函数定义（仅在key.c中定义一次）
 * 低电平为按下时，return !HAL_GPIO_ReadPin(_port, _pin);
 * 高电平为按下时，return HAL_GPIO_ReadPin(_port, _pin);
***************/
#define KEY_DEFINE(_name, _port, _pin) \
    unsigned char hal_get##_name##State(void) { \
        return !HAL_GPIO_ReadPin(_port, _pin); \
    }

// 定义各个按键的状态获取函数
KEY_DEFINE(Key1, LEFT_GPIO_Port, LEFT_Pin);
KEY_DEFINE(Key2, RIGHT_GPIO_Port, RIGHT_Pin);
KEY_DEFINE(Key3, OK_GPIO_Port, OK_Pin);

// 指向函数的指针数组。函数返回类型为unsigned char,没有参数输入
unsigned char (*getKeysState[KEYNUM])() = {hal_getKey1State,
                                           hal_getKey2State,
                                           hal_getKey3State};

void hal_KeyInit(void)
{
    unsigned char i;
    // 先在外部完成GPIO初始化
    for (i = 0; i < KEYNUM; i++)
    {
        buttonArray[i].state = BTN_S_IDLE;
        buttonArray[i].physical_pin_is_pressed = BTN_S_IDLE;
        buttonArray[i].button_id = i;
        buttonArray[i].double_click_triggered = 0;
        buttonArray[i].long_press_triggered = 0;
    }
}

/**
 * @brief 事件处理函数
 * 依据Key值判断处理具体按键发生的具体事件
 * @note 在while循环中调用，使用Key_triggered变量传入
 * @param Key 用于存储具体按键发生事件类型的数据
 */
void button_handle_event(unsigned char Key)
{
    uint8_t key_num = Key / KEYNUM;
    // 边界检查（确保在枚举范围内）
    if (key_num < KEYNUM)
    {
        // 显式类型转换（规范写法）
        KEY_NAME name = (KEY_NAME)key_num;
        uint8_t key_event_num = Key % KEYNUM;
        if (key_event_num < BTN_E_NUM)
        {
            KEY_EVENT event = (KEY_EVENT)key_event_num; // 具体按键事件
            // 调用回调函数
            if (name < KEYNUM && buttonArray[name].callback[event]) // 判断按键是否以外超出编号范围，检查其对应函数是否已经注册
            {
                Key_triggered = 10; // 重置
                buttonArray[name].callback[event](name, event); // 调用回调函数
            }
        }
    } // 具体按键编号 -> 映射到具体名称
    // 超出边界则不执行（后续可以添加错误抛出代码）
}

/**
 * @brief tick处理函数，状态判断函数
 * 判断当前按键处于什么状态，是否产生事件，若有则向事件处理函数传输具体事件。
 */
void button_process_tick(void)
{
    unsigned char i, Key;

    for (i = 0; i < KEYNUM; i++)
    {
        Key = 0;    //保证后续按钮没触发事件仍然影响前面的事件

        buttonArray[i].physical_pin_is_pressed = getKeysState[i](); // 更新每一个按键当前物理状态

        switch (buttonArray[i].state)
        {
        case BTN_S_IDLE:
            if (buttonArray[i].physical_pin_is_pressed) // 按键按下的话
            {
                buttonArray[i].state = BTN_S_DEBOUCE_PRESS; // 进入消抖状态
                buttonArray[i].state_timer_ticks = 0;       // 当前状态持续时间置零
                buttonArray[i].double_click_triggered = 0;  // 重置双击状态
            }
            else // 没按下的话保持空闲状态
            {
                buttonArray[i].state_timer_ticks = 0; // 当前状态持续时间置零
            }
            break;
        case BTN_S_DEBOUCE_PRESS:                                       // 消抖状态（确认当前按下）
            if (buttonArray[i].state_timer_ticks >= BTN_DEBOUNCE_TICKS) // 消抖时间已到
            {
                if (buttonArray[i].physical_pin_is_pressed) // 如果按键仍然为按下
                {
                    buttonArray[i].state = BTN_S_PRESSED_DOWN; // 处于按下稳定状态
                    buttonArray[i].state_timer_ticks = 0;      // 当前状态持续时间置零
                    buttonArray[i].long_press_triggered = 0;   // 重置长按状态
                }
                else // 按下时间过少或抖动
                {
                    buttonArray[i].state = BTN_S_IDLE; // 返回空闲状态
                }
            }
            break;
        case BTN_S_PRESSED_DOWN:
            if (!buttonArray[i].physical_pin_is_pressed) // 检测到物理按键释放
            {
                // 在长按时间到之前释放，则可能是单击，也可能变为双击
                buttonArray[i].state = BTN_S_WATING_FOR_SECOND_PRESS; // 等待第二次按下
                buttonArray[i].state_timer_ticks = 0;                 // 当前状态持续时间置零
            }
            else // 按键仍然按下
            {
                if (buttonArray[i].state_timer_ticks >= BTN_LONG_PRESS_TICKS) // 达到长按时间要求
                {
                    if (!buttonArray[i].long_press_triggered) // 长按事件没有被触发过
                    {
                        // 确认发生长按事件
                        Key = (i * KEYNUM) + BTN_E_LONG_PRESSED + 1; //+1为了标记有事件产生
                        buttonArray[i].long_press_triggered = 1;     // 长按事件已触发过
                    }
                    buttonArray[i].state = BTN_S_LONG_PRESS_ACTIVE; // 长按激活
                }
            }
            break;
        case BTN_S_WATING_FOR_SECOND_PRESS:
            if (buttonArray[i].physical_pin_is_pressed) // 等待时间窗口内检测到物理按键再次按下
            {
                if (buttonArray[i].state_timer_ticks >= BTN_DEBOUNCE_TICKS) // 第二次按键也超过了消抖时间
                {
                    // 确认双击事件发生
                    Key = (i * KEYNUM) + BTN_E_DOUBLE_PRESSED + 1;
                    buttonArray[i].double_click_triggered = 1;  // 双击已经触发过
                    buttonArray[i].state = BTN_S_DEBOUCE_PRESS; // 返回消抖状态，防止双击事件
                }
            }
            else if (buttonArray[i].state_timer_ticks >= BTN_DBL_CLICK_WINDOW_TICKS) // 超出等待时间窗口
            {
                if (buttonArray[i].double_click_triggered == 0)
                {
                    // 没有发生第二次按下，那么确定为单击
                    Key = (i * KEYNUM) + BTN_E_SINGLE_PRESSED + 1;
                }
                buttonArray[i].state = BTN_S_IDLE; // 返回空闲状态
            }
            break;
        case BTN_S_LONG_PRESS_ACTIVE:
            if (!buttonArray[i].physical_pin_is_pressed) // 长按后检测到物理按键释放
            {
                buttonArray[i].state = BTN_S_IDLE;       // 返回空闲状态
                buttonArray[i].long_press_triggered = 0; // 重置长按状态
            }
            // 否则保持当前状态
            break;
        default: // 理论上不应发生，作为保护，重置到已知状态
            buttonArray[i].state = BTN_S_IDLE;
            buttonArray[i].state_timer_ticks = 0;
        }

        if (Key) // 检测是否检测到按键值
        {
            Key_triggered = Key - 1; // 更新触发按键序号，后续在while中调用事件处理函数处理具体事件，减去1事件产生标记位
        }

        buttonArray[i].state_timer_ticks++; // 当前状态计时器增加
    }
}

// 设置指定按键的指定事件回调函数
void button_set_callback(KEY_NAME name, KEY_EVENT event, BtnCallback_t cb)
{
    if (name < KEYNUM && event < BTN_E_NUM)
    {
        buttonArray[name].callback[event] = cb;
    }
}
