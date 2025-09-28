#ifndef KEY_H
#define KEY_H

#include "main.h"
#include "stm32f1xx_hal.h"

// 按键状态获取函数声明
extern unsigned char hal_getKey1State(void);
extern unsigned char hal_getKey2State(void);
extern unsigned char hal_getKey3State(void);

extern unsigned char Key_triggered;

// 宏定义仅用于生成声明（不再定义函数实体）
#define KEY_DECLARE(_name) unsigned char hal_get##_name##State(void);

//消抖实践计数(10 * 10ms = 100ms)
#define BTN_DEBOUNCE_TICKS  10

//长按确认时间计数(40 * 10ms = 400ms)
#define BTN_LONG_PRESS_TICKS    40

//双击间隔窗口计时(20 * 10ms = 200ms, 第二次按下的最大时间间隔)
#define BTN_DBL_CLICK_WINDOW_TICKS  20

//按键类型
typedef enum
{
    KEY1, //S代表是Switch开关的意思
    KEY2,
    KEY3,
    KEYNUM  //按键总数
} KEY_NAME;

typedef enum
{
    BTN_E_SINGLE_PRESSED,   //单击事件
    BTN_E_DOUBLE_PRESSED,   //双击事件
    BTN_E_LONG_PRESSED,  //长按事件
    BTN_E_NUM   //事件总数
} KEY_EVENT;

//按键状态定义
typedef enum
{
    BTN_S_IDLE,                     // 空闲状态 ：按键抬起
    BTN_S_DEBOUCE_PRESS,            // 消抖状态（按下）：按键已经按下，等待消抖确认
    BTN_S_PRESSED_DOWN,             // 稳定按下状态：确认按键已经按下，计时判断长按或者等待释放
    BTN_S_WATING_FOR_SECOND_PRESS,  // 等待第二次按下状态（用于双击）：第一次短按已经释放，在时间窗口内等待第二次按下
    BTN_S_LONG_PRESS_ACTIVE         // 长按激活状态： 以满足长按条件，事件已经触发，按键仍然被按住
} ButtonState_e;    //_e代表enumeration,表示枚举类型

// 回调函数类型定义（参数：按键名称、事件类型）
typedef void (*BtnCallback_t)(KEY_NAME name, KEY_EVENT event);

void hal_KeyInit(void);
// tim中断函数中添加该函数
void button_process_tick(void);
// while中调用该函数
void button_handle_event(unsigned char Key);
//回调函数注册函数
void button_set_callback(KEY_NAME name, KEY_EVENT event, BtnCallback_t cb);   

#endif

