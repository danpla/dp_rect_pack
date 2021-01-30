
#pragma once


struct Rect {
    int x;
    int y;
    int w;
    int h;

    Rect()
        : Rect(0, 0, 0, 0)
    {}

    Rect(int x, int y, int w, int h)
        : x(x)
        , y(y)
        , w(w)
        , h(h)
    {}
};
