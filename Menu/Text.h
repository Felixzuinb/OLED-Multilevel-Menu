#ifndef __TEXT_H
#define __TEXT_H

typedef enum {
    LANG_ENGLISH = 0,
    LANG_CHINESE,
    LANG_COUNT
} Language;

typedef enum {
    TID_NULL = 0,
    TID_ROOT,
    TID_SETTINGS,
    TID_DISPLAY, TID_SOUND,TID_LANGUAGE,TID_CHINESE,TID_ENGLISH,
    TID_INFO,
    TID_VERSION, TID_STATUS,
    TID_GAMES,
    TID_SNAKE, TID_TETRIS,
    TID_TIME,
    TID_ABOUT,
    TID_TIMERS,
    TEXT_COUNT
} TextID;

extern const char* textMap[LANG_COUNT][TEXT_COUNT];

#endif
