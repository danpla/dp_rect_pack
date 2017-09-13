
#pragma once


struct Rect {
    int x;
    int y;
    int w;
    int h;

    Rect(int x = 0, int y = 0, int w = 0, int h = 0)
        : x(x)
        , y(y)
        , w(w)
        , h(h)
    {}
};
