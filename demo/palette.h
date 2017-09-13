
#pragma once


namespace palette {


struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;

    Color adjustBrightness(int a) const;
};


const int numColors = 6;
extern const Color colors[numColors];


}  // namespace palette
