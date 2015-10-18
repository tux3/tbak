#ifndef VT100_H
#define VT100_H

#include <string>
#include <iostream>

namespace vt100
{

struct MANIPULATOR
{
    MANIPULATOR() : s{'\033', '['} {}
    operator const char*() const {return &s[0];}
    char s[6];
};

struct MOVEUP : public MANIPULATOR
{
    explicit MOVEUP(int n)
    {
        s[2] = (char)('0'+n);
        s[3] = 'A';
        if (!n)
            s[0] = 0;
    }
};

struct MOVEDOWN : public MANIPULATOR
{
    explicit MOVEDOWN(int n)
    {
        s[2] = (char)('0'+n);
        s[3] = 'B';
        if (!n)
            s[0] = 0;
    }
};

struct MOVEBACK : public MANIPULATOR
{
    explicit MOVEBACK(int n)
    {
        s[2] = (char)('0'+n);
        s[3] = 'D';
        if (!n)
            s[0] = 0;
    }
};

struct CLEARLINE : public MANIPULATOR
{
    CLEARLINE()
    {
        s[2] = '2';
        s[3] = 'K';
        s[4] = '\r';
    }
};

struct STYLE_RESET : public MANIPULATOR
{
    STYLE_RESET()
    {
        s[2] = '0';
        s[3] = 'm';
    }
};

struct STYLE_ACTIVE : public MANIPULATOR
{
    STYLE_ACTIVE()
    {
        s[2] = '1';
        s[3] = 'm';
    }
};

struct STYLE_ERROR : public MANIPULATOR
{
    STYLE_ERROR()
    {
        s[2] = '3';
        s[3] = '1';
        s[4] = 'm';
    }
};

}
#endif // VT100_H

