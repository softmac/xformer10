#ifndef _WIN32

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <glob.h>
#include <limits.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#ifdef HAVE_LIBDRM
#include <xf86drm.h>
#endif
#include "gemtypes.h"
#include "atari800.h"
#include "res/resource.h"
#include "menu_sdl.h"
#include "ddlib_sdl.h"

void UninitThreads(void);
void LinuxDoCommand(int idm);
extern const int sdl_to_vk[];
void linux_get_client_rect(RECT *r);
extern int GetTileFromPos(int xPos, int yPos, void *ppt);
void ScrollTiles(void);
extern void OpenFolders(char *lpCmdLine, int *piFirstVM);

static void sigint_handler(int s) { (void)s; vi.fQuitting = TRUE; }

static LPARAM make_key_lparam(int sdl_sc, int is_up)
{
    extern const unsigned char sdl_to_ps2[];
    DWORD oem = (sdl_sc >= 0 && sdl_sc < 512) ? sdl_to_ps2[sdl_sc] : 0;
    if (is_up)
        return (LPARAM)((oem << 16) | 0xC0000001u);
    return (LPARAM)((oem << 16) | 1u);
}

static int gDrmFd = -1;  /* /dev/dri/card1 (vc4 display) for vblank sync */

static SDL_Joystick *gJoy;
static SDL_JoystickID gJoyID;
static int gJoyDir;   /* bits: 0=UP 1=DOWN 2=LEFT 3=RIGHT */

static void joy_key(int sc, int is_down)
{
    int vk = (sc >= 0 && sc < 512) ? sdl_to_vk[sc] : 0;
    if (!vk || v.iVM < 0) return;
    LPARAM lp = make_key_lparam(sc, !is_down) | 0x01000000;
    FWinMsgVM(v.iVM, vi.hWnd, is_down ? WM_KEYDOWN : WM_KEYUP, (WPARAM)vk, lp);
}

static void joy_update_dir(int new_dir)
{
    int changed = gJoyDir ^ new_dir;
    if (changed & 1) joy_key(SDL_SCANCODE_UP,    new_dir & 1);
    if (changed & 2) joy_key(SDL_SCANCODE_DOWN,  (new_dir >> 1) & 1);
    if (changed & 4) joy_key(SDL_SCANCODE_LEFT,  (new_dir >> 2) & 1);
    if (changed & 8) joy_key(SDL_SCANCODE_RIGHT, (new_dir >> 3) & 1);
    gJoyDir = new_dir;
}

int main(void)
{
    /* labwc/Wayland compositor does not phase-lock frame delivery to the display
       refresh, causing scroll jitter in games even at exactly 60 fps.  X11/XWayland
       has well-behaved vsync semantics.  Honour an explicit SDL_VIDEODRIVER override. */
    if (!getenv("SDL_VIDEODRIVER"))
        setenv("SDL_VIDEODRIVER", "x11", 0);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    if (SDL_NumJoysticks() > 0) {
        gJoy = SDL_JoystickOpen(0);
        if (gJoy) gJoyID = SDL_JoystickInstanceID(gJoy);
    }

    {
        const char *home = getenv("HOME");
        if (!home) home = "/tmp";
        snprintf(vi.szWindowsDir, sizeof(vi.szWindowsDir),
                 "%s/.config/xformer", home);
        mkdir(vi.szWindowsDir, 0755);
    }

    rgpvm = malloc(128 * (sizeof(VM) + sizeof(VMINST)));
    cpvm = 128;

    /* Initialize QPC cold-start values used by GetCycles()/GetMs() timing.
       On Windows this lives in WinMain; on Linux we must do it here first. */
    {
        LARGE_INTEGER qpf, qpc;
        QueryPerformanceFrequency(&qpf); vi.qpfCold = qpf.QuadPart;
        QueryPerformanceCounter(&qpc);   vi.qpcCold = qpc.QuadPart;
    }

    fBrakes = TRUE; /* normal speed; WinMain sets this on Windows */

    InitProperties();
    LoadProperties(NULL, FALSE);

    vi.szAppName = "Xformer";
    vi.szTitle   = "Xformer";

    sMaxTiles = 2;

    InitDrawing(X8, Y8, 8, NULL, FALSE);

    {   /* query display refresh rate; fallback 60 Hz */
        SDL_DisplayMode dm;
        v.vRefresh = (SDL_GetCurrentDisplayMode(0, &dm) == 0 && dm.refresh_rate > 1)
                     ? dm.refresh_rate : 60;
    }

    {   /* compute tile capacity from full display size so resize never needs CreateNewBitmaps */
        RECT rc; linux_get_client_rect(&rc);
        int cols = rc.right  > 0 ? rc.right  / (int)X8 : 1;
        int rows = rc.bottom > 0 ? rc.bottom / (int)Y8 : 1;
        sTilesPerRow = cols + 1;           /* +1: partial tiles on right */
        SDL_DisplayMode dm;
        int maxCols = sTilesPerRow, maxRows = rows + 2;
        if (SDL_GetCurrentDisplayMode(0, &dm) == 0) {
            maxCols = dm.w / (int)X8 + 3;
            maxRows = dm.h / (int)Y8 + 4;
        }
        sMaxTiles = maxCols * maxRows;
        if (sMaxTiles < 2) sMaxTiles = 2;
    }

    if (!CreateNewBitmaps()) {
        fprintf(stderr, "CreateNewBitmaps failed\n");
        SDL_Quit();
        return 1;
    }

    if (v.cVM == 0) {
        int iVM = AddVM(1, FALSE, FALSE);
        if (iVM >= 0) {
            FInitVM(iVM);
            ColdStart(iVM);
            SelectInstance(iVM);
        }
    }

    setenv("SDL_AUDIODRIVER", "pulseaudio", 1);
    {
        glob_t gl;
        if (glob("/run/user/*/pulse/native", GLOB_NOSORT, NULL, &gl) == 0
                && gl.gl_pathc > 0) {
            char buf[PATH_MAX];
            snprintf(buf, sizeof buf, "unix:%s", gl.gl_pathv[0]);
            setenv("PULSE_SERVER", buf, 1);
        }
        globfree(&gl);
    }

    InitSound();
    InitThreads();

    vi.fExecuting = TRUE;
    vi.fHaveFocus = TRUE;
    SDL_ShowCursor(SDL_DISABLE);

    signal(SIGINT, sigint_handler);

#ifdef HAVE_LIBDRM
    /* Open vc4-drm display controller for hardware vblank timing.
       card1 = vc4-drm (HDMI), card0 = v3d GPU.  Fall back to clock_nanosleep
       if the device can't be opened or vblank ioctl fails. */
    {
        const char *drm_cards[] = { "/dev/dri/card1", "/dev/dri/card0", NULL };
        for (int ci = 0; drm_cards[ci]; ci++) {
            int fd = open(drm_cards[ci], O_RDWR | O_CLOEXEC);
            if (fd < 0) continue;
            drmVBlank vbl;
            memset(&vbl, 0, sizeof(vbl));
            vbl.request.type = DRM_VBLANK_RELATIVE;
            vbl.request.sequence = 0;
            if (drmWaitVBlank(fd, &vbl) == 0) {
                gDrmFd = fd;
                fprintf(stderr, "DRM vblank: using %s\n", drm_cards[ci]);
                break;
            }
            close(fd);
        }
        if (gDrmFd < 0)
            fprintf(stderr, "DRM vblank: unavailable, falling back to clock_nanosleep\n");
    }
#endif

    struct timespec sNextFrame;
    clock_gettime(CLOCK_MONOTONIC, &sNextFrame);
    SDL_Event e;
    while (!vi.fQuitting) {
        while (SDL_PollEvent(&e)) {
            if (MenuHandleEvent(&e)) continue;
            if (e.type == SDL_QUIT) {
                vi.fQuitting = TRUE;
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                    vi.fHaveFocus = TRUE;
                    SDL_ShowCursor(SDL_DISABLE);
                } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    vi.fHaveFocus = FALSE;
                    SDL_ShowCursor(SDL_ENABLE);
                }
                else if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
                         && v.fTiling != 0 && v.cVM > 0
                         && (int)sTileSize.x > 0 && (int)sTileSize.y > 0) {
                    RECT rc; linux_get_client_rect(&rc);
                    int cols = rc.right > 0
                               ? (rc.right * 10 / (int)sTileSize.x + 5) / 10 : 1;
                    if (cols != sTilesPerRow) {
                        sTilesPerRow = cols;
                        InitThreads();
                    }
                }
            } else if (e.type == SDL_JOYDEVICEADDED) {
                if (!gJoy) {
                    gJoy = SDL_JoystickOpen(e.jdevice.which);
                    if (gJoy) gJoyID = SDL_JoystickInstanceID(gJoy);
                }
            } else if (e.type == SDL_JOYDEVICEREMOVED) {
                if (gJoy && e.jdevice.which == gJoyID) {
                    SDL_JoystickClose(gJoy);
                    gJoy = NULL;
                    joy_update_dir(0);   /* release held directions */
                    if (v.iVM >= 0)
                        FWinMsgVM(v.iVM, vi.hWnd, MM_JOY1BUTTONUP, 0, 0);
                }
            } else if (e.type == SDL_JOYBUTTONDOWN) {
                if (gJoy && (e.jbutton.button == 0 || e.jbutton.button == 1) && v.iVM >= 0)
                    FWinMsgVM(v.iVM, vi.hWnd, MM_JOY1BUTTONDOWN, JOY_BUTTON1, 0);
            } else if (e.type == SDL_JOYBUTTONUP) {
                if (gJoy && (e.jbutton.button == 0 || e.jbutton.button == 1) && v.iVM >= 0)
                    FWinMsgVM(v.iVM, vi.hWnd, MM_JOY1BUTTONUP, 0, 0);
            } else if (e.type == SDL_JOYAXISMOTION && gJoy) {
                int new_dir = gJoyDir;
                if (e.jaxis.axis == 0) {         /* X axis → LEFT/RIGHT */
                    new_dir &= ~(4 | 8);
                    if      (e.jaxis.value < -8192) new_dir |= 4;
                    else if (e.jaxis.value >  8192) new_dir |= 8;
                } else if (e.jaxis.axis == 1) {  /* Y axis → UP/DOWN */
                    new_dir &= ~(1 | 2);
                    if      (e.jaxis.value < -8192) new_dir |= 1;
                    else if (e.jaxis.value >  8192) new_dir |= 2;
                }
                joy_update_dir(new_dir);
            } else if (e.type == SDL_JOYHATMOTION && gJoy && e.jhat.hat == 0) {
                int hv = e.jhat.value;
                int new_dir = 0;
                if (hv & SDL_HAT_UP)    new_dir |= 1;
                if (hv & SDL_HAT_DOWN)  new_dir |= 2;
                if (hv & SDL_HAT_LEFT)  new_dir |= 4;
                if (hv & SDL_HAT_RIGHT) new_dir |= 8;
                joy_update_dir(new_dir);
            } else if (e.type == SDL_MOUSEMOTION && v.fTiling) {
                int hit = GetTileFromPos(e.motion.x, e.motion.y - MENU_H, NULL);
                if (hit != sVM) { sVM = hit; }
            } else if (e.type == SDL_MOUSEBUTTONDOWN
                       && e.button.button == SDL_BUTTON_LEFT
                       && v.fTiling && v.cVM > 0) {
                int hit = GetTileFromPos(e.button.x, e.button.y - MENU_H, NULL);
                if (hit >= 0) {
                    v.iVM = hit;
                    LinuxDoCommand(IDM_TILE);
                }
            } else if (e.type == SDL_MOUSEWHEEL && v.fTiling && v.cVM > 0
                       && sTilesPerRow > 0 && (int)sTileSize.y > 0) {
                v.sWheelOffset += e.wheel.y * (int)sTileSize.y;
                if (v.sWheelOffset > 0) v.sWheelOffset = 0;
                ScrollTiles();
            } else if (e.type == SDL_DROPFILE) {
                char *path = e.drop.file;
                if (path) {
                    size_t n = strlen(path);
                    int isGem = (n >= 4 && strcasecmp(path + n - 4, ".gem") == 0);
                    if (isGem) {
                        LoadProperties(path, TRUE);
                        LoadProperties(path, FALSE);
                        v.sWheelOffset = 0;
                        sVM = -1;
                        FixAllMenus(TRUE);
                        InitThreads();
                    } else {
                        int iVMx = -1;
                        OpenFolders(path, &iVMx);
                        if (iVMx >= 0)
                            SelectInstance(iVMx);
                        FixAllMenus(TRUE);
                        InitThreads();
                    }
                    SDL_free(path);
                }
            } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                int sc = (int)e.key.keysym.scancode;
                int vk = (sc >= 0 && sc < 512) ? sdl_to_vk[sc] : 0;
                if (vk && v.cVM > 0) {
                    int is_down = (e.type == SDL_KEYDOWN);
                    SDL_Keymod mod = SDL_GetModState();
                    LPARAM lp = make_key_lparam(sc, !is_down);
                    switch (sc) {
                    case SDL_SCANCODE_UP:    case SDL_SCANCODE_DOWN:
                    case SDL_SCANCODE_LEFT:  case SDL_SCANCODE_RIGHT:
                    case SDL_SCANCODE_HOME:  case SDL_SCANCODE_END:
                    case SDL_SCANCODE_INSERT: case SDL_SCANCODE_DELETE:
                    case SDL_SCANCODE_PAGEUP: case SDL_SCANCODE_PAGEDOWN:
                        lp |= 0x01000000;
                        break;
                    default:
                        break;
                    }
                    if (sc == SDL_SCANCODE_F4 && (mod & KMOD_ALT) && is_down) {
                        /* Alt+F4: close emulator */
                        vi.fQuitting = TRUE;
                    } else if (sc == SDL_SCANCODE_F4) {
                        /* F4 without Alt: xkey.c case 0x3e swallows bare F4,
                           so set the Win32 extended-key bit (bit 24) to make
                           the switch see 0x13E instead of 0x3E, falling to
                           default which passes scan 0x3E to CheckKey. */
                        FWinMsgVM(v.iVM, vi.hWnd,
                                  is_down ? WM_KEYDOWN : WM_KEYUP,
                                  (WPARAM)vk, lp | (LPARAM)0x01000000);
                    } else if (sc == SDL_SCANCODE_F5 && is_down) {
                        LinuxDoCommand(IDM_TILE);
                    } else if (sc == SDL_SCANCODE_F1 && (mod & KMOD_ALT) && is_down) {
                        LinuxDoCommand(IDM_TURBO);
                    } else if (sc == SDL_SCANCODE_F10 && is_down) {
                        if (mod & KMOD_ALT)
                            LinuxDoCommand(IDM_CHANGEVM);
                        else if (mod & KMOD_SHIFT)
                            LinuxDoCommand(IDM_TOGGLEBASIC);
                        else if (mod & KMOD_CTRL)
                            ColdStart(v.iVM);
                        else
                            FWarmbootVM(v.iVM);
                    } else if (sc == SDL_SCANCODE_F12 && !(mod & (KMOD_ALT|KMOD_SHIFT)) && is_down) {
                        LinuxDoCommand(IDM_STRETCH);
                    } else if (sc == SDL_SCANCODE_F12 && (mod & KMOD_ALT) && is_down) {
                        LinuxDoCommand(IDM_NTSCPAL);
                    } else if (sc == SDL_SCANCODE_F12 && (mod & KMOD_SHIFT) && is_down) {
                        LinuxDoCommand(IDM_COLORMONO);
                    } else if (sc == SDL_SCANCODE_RETURN && (mod & KMOD_ALT) && is_down) {
                        LinuxDoCommand(IDM_FULLSCREEN);
                    } else if (sc == SDL_SCANCODE_S && (mod & KMOD_ALT) && is_down) {
                        LinuxDoCommand(IDM_TOGGLESOUND);
                    } else {
                        FWinMsgVM(v.iVM, vi.hWnd,
                                  is_down ? WM_KEYDOWN : WM_KEYUP,
                                  (WPARAM)vk, lp);
                    }
                }
            }
        }
        {
            BOOL fEmuPAL = (!v.fTiling && v.cVM > 0 && v.iVM >= 0 && rgpvm[v.iVM]->fEmuPAL);

            /* Display rate governor: cap renders to min(70, display Hz) or 50 Hz PAL.
               Sets fRenderThisTime before threads run so xvideo.c can skip pixel work. */
            static Uint64 sLastRenderMs = 0;
            unsigned hz = fEmuPAL ? 50u
                                  : (unsigned)((v.vRefresh > 0 && v.vRefresh < 70)
                                               ? (unsigned)v.vRefresh + 1u : 70u);
            Uint64 nowMs = SDL_GetTicks64();
            fRenderThisTime = (nowMs - sLastRenderMs) >= (1000u / hz);
            if (fRenderThisTime)
                sLastRenderMs = nowMs;

            if (v.cVM > 0 && cThreads > 0 && !vi.fQuitting) {
                for (int t = 0; t < cThreads; t++)
                    SetEvent(ThreadStuff[t].hGoEvent);
                WaitForMultipleObjects(cThreads, hDoneEvent, TRUE, INFINITE);

                /* Frame throttle: wait for hardware vblank so each present is
                   phase-locked to the display refresh.  SDL_RenderPresent is
                   non-blocking on Pi OS / XWayland (confirmed avg=0ms), so
                   clock_nanosleep alone accumulates phase drift causing judder.
                   DRM vblank waits for the actual vc4 display controller vblank
                   and eliminates the drift.  Falls back to clock_nanosleep when
                   the DRM device isn't available (non-Pi, or turbo mode). */
                long jiffy_ns = fEmuPAL ? (1000000000L / PAL_FPS)
                                        : (1000000000L / NTSC_FPS);
                if (fBrakes) {
#ifdef HAVE_LIBDRM
                    if (gDrmFd >= 0) {
                        drmVBlank vbl;
                        memset(&vbl, 0, sizeof(vbl));
                        vbl.request.type = DRM_VBLANK_RELATIVE;
                        vbl.request.sequence = 1;
                        drmWaitVBlank(gDrmFd, &vbl);
                    } else
#endif
                    {
                        sNextFrame.tv_nsec += jiffy_ns;
                        if (sNextFrame.tv_nsec >= 1000000000L) {
                            sNextFrame.tv_nsec -= 1000000000L;
                            sNextFrame.tv_sec++;
                        }
                        struct timespec now;
                        clock_gettime(CLOCK_MONOTONIC, &now);
                        long long target = (long long)sNextFrame.tv_sec * 1000000000LL + sNextFrame.tv_nsec;
                        long long cur    = (long long)now.tv_sec        * 1000000000LL + now.tv_nsec;
                        if (target < cur - 2 * jiffy_ns)
                            sNextFrame = now;
                        else
                            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sNextFrame, NULL);
                    }
                } else {
                    clock_gettime(CLOCK_MONOTONIC, &sNextFrame);
                }

                if (fRenderThisTime)
                    RenderBitmap_SDL();
            }
        }
    }

    UninitThreads();
    if (v.fSaveOnExit)
        SaveProperties(NULL);
    UninitDrawing(TRUE);
    UninitSound();
#ifdef HAVE_LIBDRM
    if (gDrmFd >= 0) { close(gDrmFd); gDrmFd = -1; }
#endif
    SDL_Quit();
    return 0;
}

#endif /* !_WIN32 */
