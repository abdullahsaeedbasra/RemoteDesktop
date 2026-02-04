#pragma once
#include <afxwin.h>
#include <vector>
using namespace std;

enum MessageType : uint32_t
{
    MSG_MOUSEMOVE = 1,
    MSG_LEFT_BUTTON_DOWN = 2,
    MSG_LEFT_BUTTON_UP = 3,
    MSG_RIGHT_BUTTON_DOWN = 4,
    MSG_RIGHT_BUTTON_UP = 5,
    MSG_LEFT_BUTTON_DOUBLE_CLICK = 6,
    MSG_MOUSE_WHEEL_UP = 7,
    MSG_MOUSE_WHEEL_DOWN = 8,
    MSG_KEYBOARD = 9,
};

#pragma pack(push, 1)
struct MessageHeader
{
    uint32_t type;
    int size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct MouseMoveData
{
    int percentage_x;
    int percentage_y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct KeyData
{
    uint32_t flags;
    uint32_t vk;
};
#pragma pack(pop)