#include "Ava_Persian.h"

struct AvaPersianForm
{
    uint32_t code;
    uint32_t isolated;
    uint32_t initial;
    uint32_t medial;
    uint32_t finalForm;
    bool dual;
};

static const AvaPersianForm AVA_PERSIAN_FORMS[] = {
    {0x0621,0xFE80,0xFE80,0xFE80,0xFE80,false},
    {0x0622,0xFE81,0xFE81,0xFE81,0xFE82,false},
    {0x0623,0xFE83,0xFE83,0xFE83,0xFE84,false},
    {0x0624,0xFE85,0xFE85,0xFE85,0xFE86,false},
    {0x0625,0xFE87,0xFE87,0xFE87,0xFE88,false},
    {0x0626,0xFE89,0xFE8B,0xFE8C,0xFE8A,true},
    {0x0627,0xFE8D,0xFE8D,0xFE8D,0xFE8E,false},
    {0x0628,0xFE8F,0xFE91,0xFE92,0xFE90,true},
    {0x0629,0xFE93,0xFE93,0xFE93,0xFE94,false},
    {0x062A,0xFE95,0xFE97,0xFE98,0xFE96,true},
    {0x062B,0xFE99,0xFE9B,0xFE9C,0xFE9A,true},
    {0x062C,0xFE9D,0xFE9F,0xFEA0,0xFE9E,true},
    {0x062D,0xFEA1,0xFEA3,0xFEA4,0xFEA2,true},
    {0x062E,0xFEA5,0xFEA7,0xFEA8,0xFEA6,true},
    {0x062F,0xFEA9,0xFEA9,0xFEA9,0xFEAA,false},
    {0x0630,0xFEAB,0xFEAB,0xFEAB,0xFEAC,false},
    {0x0631,0xFEAD,0xFEAD,0xFEAD,0xFEAE,false},
    {0x0632,0xFEAF,0xFEAF,0xFEAF,0xFEB0,false},
    {0x0633,0xFEB1,0xFEB3,0xFEB4,0xFEB2,true},
    {0x0634,0xFEB5,0xFEB7,0xFEB8,0xFEB6,true},
    {0x0635,0xFEB9,0xFEBB,0xFEBC,0xFEBA,true},
    {0x0636,0xFEBD,0xFEBF,0xFEC0,0xFEBE,true},
    {0x0637,0xFEC1,0xFEC3,0xFEC4,0xFEC2,true},
    {0x0638,0xFEC5,0xFEC7,0xFEC8,0xFEC6,true},
    {0x0639,0xFEC9,0xFECB,0xFECC,0xFECA,true},
    {0x063A,0xFECD,0xFECF,0xFED0,0xFECE,true},
    {0x0641,0xFED1,0xFED3,0xFED4,0xFED2,true},
    {0x0642,0xFED5,0xFED7,0xFED8,0xFED6,true},
    {0x0643,0xFED9,0xFEDB,0xFEDC,0xFEDA,true},
    {0x0644,0xFEDD,0xFEDF,0xFEE0,0xFEDE,true},
    {0x0645,0xFEE1,0xFEE3,0xFEE4,0xFEE2,true},
    {0x0646,0xFEE5,0xFEE7,0xFEE8,0xFEE6,true},
    {0x0647,0xFEE9,0xFEEB,0xFEEC,0xFEEA,true},
    {0x0648,0xFEED,0xFEED,0xFEED,0xFEEE,false},
    {0x0649,0xFEEF,0xFEEF,0xFEF0,0xFEF0,true},
    {0x064A,0xFEF1,0xFEF3,0xFEF4,0xFEF2,true},
    {0x067E,0xFB56,0xFB58,0xFB59,0xFB57,true},
    {0x0686,0xFB7A,0xFB7C,0xFB7D,0xFB7B,true},
    {0x0698,0xFB8A,0xFB8A,0xFB8A,0xFB8B,false},
    {0x06A9,0xFB8E,0xFB90,0xFB91,0xFB8F,true},
    {0x06AF,0xFB92,0xFB94,0xFB95,0xFB93,true},
    {0x06CC,0xFBFC,0xFBFE,0xFBFF,0xFBFD,true}
};

static int avaPersianFind(uint32_t code)
{
    for (size_t i = 0; i < sizeof(AVA_PERSIAN_FORMS) / sizeof(AVA_PERSIAN_FORMS[0]); ++i)
    {
        if (AVA_PERSIAN_FORMS[i].code == code)
            return (int)i;
    }
    return -1;
}

static bool avaPersianDecode(const char* text, size_t& pos, uint32_t& code)
{
    const uint8_t c0 = (uint8_t)text[pos];

    if (c0 < 0x80)
    {
        code = c0;
        pos++;
        return true;
    }

    if ((c0 & 0xE0) == 0xC0)
    {
        code = ((uint32_t)(c0 & 0x1F) << 6) |
               (uint32_t)(text[pos + 1] & 0x3F);
        pos += 2;
        return true;
    }

    if ((c0 & 0xF0) == 0xE0)
    {
        code = ((uint32_t)(c0 & 0x0F) << 12) |
               ((uint32_t)(text[pos + 1] & 0x3F) << 6) |
               (uint32_t)(text[pos + 2] & 0x3F);
        pos += 3;
        return true;
    }

    if ((c0 & 0xF8) == 0xF0)
    {
        code = ((uint32_t)(c0 & 0x07) << 18) |
               ((uint32_t)(text[pos + 1] & 0x3F) << 12) |
               ((uint32_t)(text[pos + 2] & 0x3F) << 6) |
               (uint32_t)(text[pos + 3] & 0x3F);
        pos += 4;
        return true;
    }

    code = 0xFFFD;
    pos++;
    return false;
}

static void avaPersianAppendUTF8(String& out, uint32_t code)
{
    if (code <= 0x7F)
    {
        out += (char)code;
    }
    else if (code <= 0x7FF)
    {
        out += (char)(0xC0 | (code >> 6));
        out += (char)(0x80 | (code & 0x3F));
    }
    else if (code <= 0xFFFF)
    {
        out += (char)(0xE0 | (code >> 12));
        out += (char)(0x80 | ((code >> 6) & 0x3F));
        out += (char)(0x80 | (code & 0x3F));
    }
    else
    {
        out += (char)(0xF0 | (code >> 18));
        out += (char)(0x80 | ((code >> 12) & 0x3F));
        out += (char)(0x80 | ((code >> 6) & 0x3F));
        out += (char)(0x80 | (code & 0x3F));
    }
}

static bool avaPersianIsRTL(uint32_t code)
{
    return avaPersianFind(code) >= 0;
}

bool avaPersianHasRTL(const String& text)
{
    size_t pos = 0;
    while (pos < text.length())
    {
        uint32_t code = 0;
        avaPersianDecode(text.c_str(), pos, code);
        if (avaPersianIsRTL(code))
            return true;
    }
    return false;
}

String avaPersianShape(const String& text)
{
    String out;
    out.reserve(text.length() + 16);

    uint32_t codes[192];
    size_t count = 0;

    size_t pos = 0;
    while (pos < text.length() && count < 192)
    {
        uint32_t code = 0;
        avaPersianDecode(text.c_str(), pos, code);
        codes[count++] = code;
    }

    for (size_t i = 0; i < count; ++i)
    {
        const int formIndex = avaPersianFind(codes[i]);

        if (formIndex < 0)
        {
            avaPersianAppendUTF8(out, codes[i]);
            continue;
        }

        const AvaPersianForm& form = AVA_PERSIAN_FORMS[formIndex];

        const int prevIndex =
            (i > 0) ? avaPersianFind(codes[i - 1]) : -1;

        const int nextIndex =
            (i + 1 < count) ? avaPersianFind(codes[i + 1]) : -1;

        const bool joinsToPrev =
            prevIndex >= 0 &&
            AVA_PERSIAN_FORMS[prevIndex].dual;

        const bool joinsToNext =
            nextIndex >= 0 &&
            form.dual &&
            AVA_PERSIAN_FORMS[nextIndex].dual;

        uint32_t shaped = form.isolated;

        if (form.dual)
        {
            if (joinsToPrev && joinsToNext)
                shaped = form.medial;
            else if (joinsToPrev)
                shaped = form.finalForm;
            else if (joinsToNext)
                shaped = form.initial;
        }
        else if (joinsToPrev)
        {
            shaped = form.finalForm;
        }

        avaPersianAppendUTF8(out, shaped);
    }

    return out;
}
