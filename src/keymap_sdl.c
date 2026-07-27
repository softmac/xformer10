#ifndef _WIN32

#include <SDL2/SDL.h>
#include "gemtypes.h"

/* Forward: SDL scancode -> Win32 VK code (0 = unmapped) */
const int sdl_to_vk[512] = {
    /* Letters A-Z: SDL 4-29, VK 0x41-0x5A */
    [4]  = 'A', [5]  = 'B', [6]  = 'C', [7]  = 'D', [8]  = 'E',
    [9]  = 'F', [10] = 'G', [11] = 'H', [12] = 'I', [13] = 'J',
    [14] = 'K', [15] = 'L', [16] = 'M', [17] = 'N', [18] = 'O',
    [19] = 'P', [20] = 'Q', [21] = 'R', [22] = 'S', [23] = 'T',
    [24] = 'U', [25] = 'V', [26] = 'W', [27] = 'X', [28] = 'Y',
    [29] = 'Z',
    /* Digits 1-9, 0: SDL 30-39, VK 0x31-0x39, 0x30 */
    [30] = '1', [31] = '2', [32] = '3', [33] = '4', [34] = '5',
    [35] = '6', [36] = '7', [37] = '8', [38] = '9', [39] = '0',
    /* Control keys */
    [40] = VK_RETURN,   /* SDL_SCANCODE_RETURN */
    [41] = VK_ESCAPE,   /* SDL_SCANCODE_ESCAPE */
    [42] = VK_BACK,     /* SDL_SCANCODE_BACKSPACE */
    [43] = VK_TAB,      /* SDL_SCANCODE_TAB */
    [44] = VK_SPACE,    /* SDL_SCANCODE_SPACE */
    /* Punctuation (Win32 OEM VK codes) */
    [45] = 0xBD,  /* SDL_SCANCODE_MINUS      -> VK_OEM_MINUS */
    [46] = 0xBB,  /* SDL_SCANCODE_EQUALS     -> VK_OEM_PLUS */
    [47] = 0xDB,  /* SDL_SCANCODE_LEFTBRACKET  -> VK_OEM_4 */
    [48] = 0xDD,  /* SDL_SCANCODE_RIGHTBRACKET -> VK_OEM_6 */
    [49] = 0xDC,  /* SDL_SCANCODE_BACKSLASH   -> VK_OEM_5 */
    [51] = 0xBA,  /* SDL_SCANCODE_SEMICOLON   -> VK_OEM_1 */
    [52] = 0xDE,  /* SDL_SCANCODE_APOSTROPHE  -> VK_OEM_7 */
    [53] = 0xC0,  /* SDL_SCANCODE_GRAVE       -> VK_OEM_3 */
    [54] = 0xBC,  /* SDL_SCANCODE_COMMA       -> VK_OEM_COMMA */
    [55] = 0xBE,  /* SDL_SCANCODE_PERIOD      -> VK_OEM_PERIOD */
    [56] = 0xBF,  /* SDL_SCANCODE_SLASH       -> VK_OEM_2 */
    [57] = VK_CAPITAL,  /* SDL_SCANCODE_CAPSLOCK */
    /* Function keys F1-F12: SDL 58-69 */
    [58] = VK_F1,  [59] = VK_F2,  [60] = VK_F3,  [61] = VK_F4,
    [62] = VK_F5,  [63] = VK_F6,  [64] = VK_F7,  [65] = VK_F8,
    [66] = VK_F9,  [67] = VK_F10, [68] = VK_F11, [69] = VK_F12,
    /* Navigation cluster */
    [71] = VK_SCROLL,  /* SDL_SCANCODE_SCROLLLOCK */
    [72] = VK_PAUSE,   /* SDL_SCANCODE_PAUSE */
    [73] = VK_INSERT,  /* SDL_SCANCODE_INSERT */
    [74] = VK_HOME,    /* SDL_SCANCODE_HOME */
    [75] = VK_PRIOR,   /* SDL_SCANCODE_PAGEUP */
    [76] = VK_DELETE,  /* SDL_SCANCODE_DELETE */
    [77] = VK_END,     /* SDL_SCANCODE_END */
    [78] = VK_NEXT,    /* SDL_SCANCODE_PAGEDOWN */
    [79] = VK_RIGHT,   /* SDL_SCANCODE_RIGHT */
    [80] = VK_LEFT,    /* SDL_SCANCODE_LEFT */
    [81] = VK_DOWN,    /* SDL_SCANCODE_DOWN */
    [82] = VK_UP,      /* SDL_SCANCODE_UP */
    [83] = VK_NUMLOCK, /* SDL_SCANCODE_NUMLOCKCLEAR */
    /* Numpad: KP_DIVIDE=84 KP_MULTIPLY=85 KP_MINUS=86 KP_PLUS=87 KP_ENTER=88
               KP_1=89 KP_2=90 KP_3=91 KP_4=92 KP_5=93 KP_6=94
               KP_7=95 KP_8=96 KP_9=97 KP_0=98 KP_PERIOD=99 */
    [84] = VK_DIVIDE,    [85] = VK_MULTIPLY, [86] = VK_SUBTRACT, [87] = VK_ADD,
    [88] = VK_RETURN,    /* KP_ENTER: same VK as main Enter */
    [89] = VK_NUMPAD1,   [90] = VK_NUMPAD2,  [91] = VK_NUMPAD3,
    [92] = VK_NUMPAD4,   [93] = VK_NUMPAD5,  [94] = VK_NUMPAD6,
    [95] = VK_NUMPAD7,   [96] = VK_NUMPAD8,  [97] = VK_NUMPAD9,
    [98] = VK_NUMPAD0,   [99] = VK_DECIMAL,
    /* Modifier keys */
    [224] = VK_LCONTROL, /* SDL_SCANCODE_LCTRL */
    [225] = VK_LSHIFT,   /* SDL_SCANCODE_LSHIFT */
    [226] = VK_LMENU,    /* SDL_SCANCODE_LALT */
    [228] = VK_RCONTROL, /* SDL_SCANCODE_RCTRL */
    [229] = VK_RSHIFT,   /* SDL_SCANCODE_RSHIFT */
    [230] = VK_RMENU,    /* SDL_SCANCODE_RALT */
};

/* SDL scancode -> PS/2 Set 1 OEM scan code (0 = unmapped) */
const unsigned char sdl_to_ps2[512] = {
    /* Letters */
    [SDL_SCANCODE_A]=0x1E, [SDL_SCANCODE_B]=0x30, [SDL_SCANCODE_C]=0x2E,
    [SDL_SCANCODE_D]=0x20, [SDL_SCANCODE_E]=0x12, [SDL_SCANCODE_F]=0x21,
    [SDL_SCANCODE_G]=0x22, [SDL_SCANCODE_H]=0x23, [SDL_SCANCODE_I]=0x17,
    [SDL_SCANCODE_J]=0x24, [SDL_SCANCODE_K]=0x25, [SDL_SCANCODE_L]=0x26,
    [SDL_SCANCODE_M]=0x32, [SDL_SCANCODE_N]=0x31, [SDL_SCANCODE_O]=0x18,
    [SDL_SCANCODE_P]=0x19, [SDL_SCANCODE_Q]=0x10, [SDL_SCANCODE_R]=0x13,
    [SDL_SCANCODE_S]=0x1F, [SDL_SCANCODE_T]=0x14, [SDL_SCANCODE_U]=0x16,
    [SDL_SCANCODE_V]=0x2F, [SDL_SCANCODE_W]=0x11, [SDL_SCANCODE_X]=0x2D,
    [SDL_SCANCODE_Y]=0x15, [SDL_SCANCODE_Z]=0x2C,
    /* Digits */
    [SDL_SCANCODE_1]=0x02, [SDL_SCANCODE_2]=0x03, [SDL_SCANCODE_3]=0x04,
    [SDL_SCANCODE_4]=0x05, [SDL_SCANCODE_5]=0x06, [SDL_SCANCODE_6]=0x07,
    [SDL_SCANCODE_7]=0x08, [SDL_SCANCODE_8]=0x09, [SDL_SCANCODE_9]=0x0A,
    [SDL_SCANCODE_0]=0x0B,
    /* Punctuation */
    [SDL_SCANCODE_RETURN]=0x1C,    [SDL_SCANCODE_ESCAPE]=0x01,
    [SDL_SCANCODE_BACKSPACE]=0x0E, [SDL_SCANCODE_TAB]=0x0F,
    [SDL_SCANCODE_SPACE]=0x39,     [SDL_SCANCODE_MINUS]=0x0C,
    [SDL_SCANCODE_EQUALS]=0x0D,    [SDL_SCANCODE_LEFTBRACKET]=0x1A,
    [SDL_SCANCODE_RIGHTBRACKET]=0x1B, [SDL_SCANCODE_BACKSLASH]=0x2B,
    [SDL_SCANCODE_SEMICOLON]=0x27, [SDL_SCANCODE_APOSTROPHE]=0x28,
    [SDL_SCANCODE_GRAVE]=0x29,     [SDL_SCANCODE_COMMA]=0x33,
    [SDL_SCANCODE_PERIOD]=0x34,    [SDL_SCANCODE_SLASH]=0x35,
    [SDL_SCANCODE_CAPSLOCK]=0x3A,
    /* Function keys */
    [SDL_SCANCODE_F1]=0x3B,  [SDL_SCANCODE_F2]=0x3C,
    [SDL_SCANCODE_F3]=0x3D,  [SDL_SCANCODE_F4]=0x3E,
    [SDL_SCANCODE_F5]=0x3F,  [SDL_SCANCODE_F6]=0x40,
    [SDL_SCANCODE_F7]=0x41,  [SDL_SCANCODE_F8]=0x42,
    [SDL_SCANCODE_F9]=0x43,  [SDL_SCANCODE_F10]=0x44,
    [SDL_SCANCODE_F11]=0x57, [SDL_SCANCODE_F12]=0x58,
    /* Navigation */
    [SDL_SCANCODE_INSERT]=0x52, [SDL_SCANCODE_HOME]=0x47,
    [SDL_SCANCODE_PAGEUP]=0x49, [SDL_SCANCODE_DELETE]=0x53,
    [SDL_SCANCODE_END]=0x4F,    [SDL_SCANCODE_PAGEDOWN]=0x51,
    [SDL_SCANCODE_RIGHT]=0x4D,  [SDL_SCANCODE_LEFT]=0x4B,
    [SDL_SCANCODE_DOWN]=0x50,   [SDL_SCANCODE_UP]=0x48,
    /* Numpad — base PS/2 codes (no extended bit); nav-cluster keys use same
       codes but the extended bit is set in make_key_lparam via lp|=0x01000000 */
    [SDL_SCANCODE_KP_DIVIDE]   = 0x35,  /* KP_/ → same PS/2 as main /  */
    [SDL_SCANCODE_KP_MULTIPLY] = 0x37,  /* KP_* */
    [SDL_SCANCODE_KP_MINUS]    = 0x4A,  /* KP_- */
    [SDL_SCANCODE_KP_PLUS]     = 0x4E,  /* KP_+ */
    [SDL_SCANCODE_KP_ENTER]    = 0x1C,  /* KP_Enter → same PS/2 as main Enter */
    [SDL_SCANCODE_KP_1]        = 0x4F,  [SDL_SCANCODE_KP_2] = 0x50,
    [SDL_SCANCODE_KP_3]        = 0x51,  [SDL_SCANCODE_KP_4] = 0x4B,
    [SDL_SCANCODE_KP_5]        = 0x4C,  [SDL_SCANCODE_KP_6] = 0x4D,
    [SDL_SCANCODE_KP_7]        = 0x47,  [SDL_SCANCODE_KP_8] = 0x48,
    [SDL_SCANCODE_KP_9]        = 0x49,  [SDL_SCANCODE_KP_0] = 0x52,
    [SDL_SCANCODE_KP_PERIOD]   = 0x53,
    /* Modifiers */
    [SDL_SCANCODE_LCTRL]=0x1D,  [SDL_SCANCODE_LSHIFT]=0x2A,
    [SDL_SCANCODE_LALT]=0x38,   [SDL_SCANCODE_RCTRL]=0x1D,
    [SDL_SCANCODE_RSHIFT]=0x36, [SDL_SCANCODE_RALT]=0x38,
};

/* Reverse: Win32 VK code -> SDL scancode (0 = unmapped) */
static int vk_to_sdl[256];

static void build_reverse(void)
{
    for (int sc = 1; sc < 512; sc++) {
        int vk = sdl_to_vk[sc];
        if (vk > 0 && vk < 256 && vk_to_sdl[vk] == 0)
            vk_to_sdl[vk] = sc;
    }
    /* Map generic shift/ctrl/alt to left-side variants */
    if (!vk_to_sdl[VK_SHIFT])   vk_to_sdl[VK_SHIFT]   = 225;
    if (!vk_to_sdl[VK_CONTROL]) vk_to_sdl[VK_CONTROL] = 224;
    if (!vk_to_sdl[VK_MENU])    vk_to_sdl[VK_MENU]    = 226;
}

SHORT sdl_get_async_key_state(int vk)
{
    static int initialized = 0;
    if (!initialized) { build_reverse(); initialized = 1; }
    if (vk < 0 || vk >= 256) return 0;
    int sc = vk_to_sdl[vk];
    if (sc == 0) return 0;
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    return state[sc] ? (SHORT)0x8000 : 0;
}

#endif /* !_WIN32 */
