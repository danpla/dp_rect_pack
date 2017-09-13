
#include "palette.h"

#include <cassert>


namespace palette {


const Color colors[numColors] = {
    {0xFF, 0x77, 0x77},
    {0x77, 0xFF, 0x77},
    {0x77, 0x77, 0xFF},
    {0xFF, 0x77, 0xFF},
    {0xFF, 0xFF, 0x77},
    {0x77, 0xFF, 0xFF},
};


static inline int adjustComponent(int c, int a)
{
    c += a;
    if (c < 0)
        c = 0;
    else if (c > 255)
        c = 255;
    return c;
}


Color Color::adjustBrightness(int a) const
{
    Color result = *this;
    result.r = adjustComponent(result.r, a);
    result.g = adjustComponent(result.g, a);
    result.b = adjustComponent(result.b, a);
    return result;
}


}  // namespace palette
