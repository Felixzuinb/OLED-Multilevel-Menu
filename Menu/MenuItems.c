#include "MenuItems.h"
#include "Menu.h"

MenuItem mainMenu,
    menu_settings,
    menu_settings_display, menu_settings_sound, menu_settings_language,
    menu_language_english, menu_language_chinese,
    menu_info,
    menu_info_version, menu_info_status,
    menu_games,
    menu_games_snake, menu_games_tetris,
    menu_time,
    menu_timers,
    menu_about;

static void action_set_Chinese(void);
static void action_set_English(void);

void Init_Menu(void)
{
  Menu_Init(); // 初始化菜单管理器

  // 创建菜单项
  Menu_InitItem(TID_ROOT, &mainMenu, NULL);
  Menu_InitItem(TID_SETTINGS, &menu_settings, NULL);
  Menu_InitItem(TID_INFO, &menu_info, NULL);
  Menu_InitItem(TID_GAMES, &menu_games, NULL);
  Menu_InitItem(TID_DISPLAY, &menu_settings_display, NULL);
  Menu_InitItem(TID_SOUND, &menu_settings_sound, NULL);
  Menu_InitItem(TID_LANGUAGE, &menu_settings_language, NULL);
  Menu_InitItem(TID_VERSION, &menu_info_version, NULL);
  Menu_InitItem(TID_STATUS, &menu_info_status, NULL);
  Menu_InitItem(TID_SNAKE, &menu_games_snake, NULL);
  Menu_InitItem(TID_TETRIS, &menu_games_tetris, NULL);
  Menu_InitItem(TID_TIME, &menu_time, NULL);
  Menu_InitItem(TID_TIMERS, &menu_timers, NULL);
  Menu_InitItem(TID_ABOUT, &menu_about, NULL);
  Menu_InitItem(TID_CHINESE, &menu_language_chinese, action_set_Chinese);
  Menu_InitItem(TID_ENGLISH, &menu_language_english, action_set_English);

  // 设置菜单层级关系
  menu_manager.current_menu = &mainMenu;
  menu_manager.root = &mainMenu;

  Menu_AddChild(&mainMenu, &menu_settings);
  Menu_AddChild(&mainMenu, &menu_info);
  Menu_AddChild(&mainMenu, &menu_games);
  Menu_AddChild(&mainMenu, &menu_time);
  Menu_AddChild(&mainMenu, &menu_timers);
  Menu_AddChild(&mainMenu, &menu_about);
  Menu_AddChild(&menu_settings, &menu_settings_display);
  Menu_AddChild(&menu_settings, &menu_settings_sound);
  Menu_AddChild(&menu_settings, &menu_settings_language);
  Menu_AddChild(&menu_settings_language, &menu_language_english);
  Menu_AddChild(&menu_settings_language, &menu_language_chinese);
  Menu_AddChild(&menu_info, &menu_info_version);
  Menu_AddChild(&menu_info, &menu_info_status);
  Menu_AddChild(&menu_games, &menu_games_snake);
  Menu_AddChild(&menu_games, &menu_games_tetris);

  // 默认选中第一个菜单项
  if (mainMenu.child != NULL)
    menu_manager.current_item = mainMenu.child;

  currentLanguage = LANG_CHINESE;
  menu_manager.refreshText(&mainMenu);
}

static void action_set_Chinese(void)
{
  currentLanguage = LANG_CHINESE;

  menu_manager.refreshText(menu_manager.root);

  Menu_Display();
}

static void action_set_English(void)
{
  currentLanguage = LANG_ENGLISH;

  menu_manager.refreshText(menu_manager.root);

  Menu_Display();
}

