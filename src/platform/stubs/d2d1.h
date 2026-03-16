// Linux stub for <d2d1.h>
#pragma once

#ifndef _D2D1_COLOR_F_DEFINED
#define _D2D1_COLOR_F_DEFINED

struct D2D1_COLOR_F
{
    float r, g, b, a;
};

#endif // _D2D1_COLOR_F_DEFINED

struct D2D1_POINT_2F
{
    float x, y;
};

struct D2D1_RECT_F
{
    float left, top, right, bottom;
};

struct D2D1_SIZE_F
{
    float width, height;
};

struct D2D1_SIZE_U
{
    unsigned int width, height;
};
