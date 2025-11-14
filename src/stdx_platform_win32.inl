#include <stdx_common.h>
#include "stdx_opengl.h"
#include <windowsx.h>
#include "stdx_platform.h"
#include <shellscalingapi.h>
#include <stdbool.h>
#include <stdlib.h>

/*** XInput specifics ***/
#define XINPUT_GAMEPAD_DPAD_UP        0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN     	0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT     	0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT     0x0008
#define XINPUT_GAMEPAD_START          0x0010
#define XINPUT_GAMEPAD_BACK           0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB     0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB    0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER	0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER	0x0200
#define XINPUT_GAMEPAD_A              0x1000
#define XINPUT_GAMEPAD_B              0x2000
#define XINPUT_GAMEPAD_X              0x4000
#define XINPUT_GAMEPAD_Y              0x8000
#define XINPUT_MAX_AXIS_VALUE         32767
#define XINPUT_MIN_AXIS_VALUE         -32768
#define XINPUT_MAX_TRIGGER_VALUE      255
//#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  9000
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD    30

typedef struct _XINPUT_GAMEPAD
{
  WORD  wButtons;
  BYTE  bLeftTrigger;
  BYTE  bRightTrigger;
  SHORT sThumbLX;
  SHORT sThumbLY;
  SHORT sThumbRX;
  SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE
{
  DWORD          dwPacketNumber;
  XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef struct _XINPUT_VIBRATION
{
  WORD wLeftMotorSpeed;
  WORD wRightMotorSpeed;
} XINPUT_VIBRATION, *PXINPUT_VIBRATION;

typedef DWORD (*XInputGetStateFunc)(DWORD dwUserIndex, XINPUT_STATE *pState);
typedef DWORD (*XInputSetStateFunc)(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);

DWORD s_xinput_state_get_DUMMY(DWORD dwUserIndex, XINPUT_STATE *pState)
{
  fprintf(stderr, "No XInput ...\n");
  return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD s_xinput_state_set_DUMMY(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
{
  fprintf(stderr, "No XInput ...\n");
  return ERROR_DEVICE_NOT_CONNECTED;
}

static BOOL s_client_rect_in_screen_get(HWND hwnd, RECT* out)
{
  RECT cr;
  if (!GetClientRect(hwnd, &cr)) return FALSE;

  POINT tl = { cr.left,  cr.top    };
  POINT br = { cr.right, cr.bottom };

  if (!ClientToScreen(hwnd, &tl)) return FALSE;
  if (!ClientToScreen(hwnd, &br)) return FALSE;

  out->left   = tl.x;
  out->top    = tl.y;
  out->right  = br.x;
  out->bottom = br.y;
  return TRUE;
}

static BOOL s_platform_graphics_is_opengl(XPlatformFlags flags)
{
  return (flags & X_PLATFORM_GRAPHICS_OPENGL_3_3 || flags & X_PLATFORM_GRAPHICS_OPENGL_3_0);
}

static struct 
{
  // input state
  XKeyboardState      keyboard_state;
  XMouseState         mouse_sate;
  XJoystickState      joystick_state[X_JOYSTICK_MAX];
  // Event queue
  XEvent              events[X_PLATFORM_MAX_EVENTS];
  u32                 events_count;
  u32                 events_poll_index;
  HCURSOR             default_cursor;
  HCURSOR             cursor_current;
  XPlatformFlags      flags;
  // XInput specifics
  XInputGetStateFunc  xinput_get_state_func;
  XInputSetStateFunc  xinput_set_state_func;
  bool  cursor_visible;

  #ifdef X_OPENGL_H
  XGL* gl;
  #endif
  
} s_platform = {0};

struct XWindow_t
{
  HWND            handle;
  HDC             dc;
  LONG_PTR        prev_style;
  POINT           relative_center;
  WINDOWPLACEMENT prev_placement;
  bool            close_flag;       // true if user requested closing the window.
  bool            fullscreen;       // true if fullscreen, false otherwise.
  bool            cursor_changed;   // true if using a cursor other than the default
  bool            relative_mode;    // true if relative mode is enabled
  bool            warping;          // true after warping cursor position back
                                    // to center when in relative mode. The next
                                    // WM_MOUSEMOVE event will be ignored and
                                    // the flag automatically clean.
};

void s_window_clip_rect_update(XWindow win)
{
  if (win && x_plat_cursor_confine_get(win))
  {
    RECT r;
    if (s_client_rect_in_screen_get(win->handle, &r))
      ClipCursor(&r);
  }
}

static bool s_xInputInit(void)
{
  memset(&s_platform.joystick_state, 0, sizeof(s_platform.joystick_state));

  char* xInputDllName = "xinput1_1.dll"; 
  HMODULE hXInput = LoadLibraryA(xInputDllName);
  if (!hXInput)
  {				
    xInputDllName = "xinput9_1_0.dll";
    hXInput = LoadLibraryA(xInputDllName);
  }

  if (!hXInput)
  {
    xInputDllName = "xinput1_3.dll";
    hXInput = LoadLibraryA(xInputDllName);
  }

  if (!hXInput)
  {
    fprintf(stderr, "could not initialize xinput. No valid xinput dll found\n");
    s_platform.xinput_get_state_func = (XInputGetStateFunc) s_xinput_state_get_DUMMY;
    s_platform.xinput_set_state_func = (XInputSetStateFunc) s_xinput_state_set_DUMMY;
    return false;
  }

  //get xinput function pointers
  s_platform.xinput_get_state_func = (XInputGetStateFunc) GetProcAddress(hXInput, "XInputGetState");
  s_platform.xinput_set_state_func = (XInputSetStateFunc) GetProcAddress(hXInput, "XInputSetState");

  if (!s_platform.xinput_get_state_func)
    s_platform.xinput_get_state_func = (XInputGetStateFunc) s_xinput_state_get_DUMMY;

  if (!s_platform.xinput_set_state_func)
    s_platform.xinput_set_state_func = (XInputSetStateFunc) s_xinput_state_set_DUMMY;
  return true;
}

static inline void s_XInputPollEvents(void)
{
  // get gamepad input
  for(i32 gamepadIndex = 0; gamepadIndex < X_JOYSTICK_MAX; gamepadIndex++)
  {
    XINPUT_STATE xinputState = {0};
    XJoystickState* joystick_state = &s_platform.joystick_state[gamepadIndex];

    // ignore unconnected controllers
    if (s_platform.xinput_get_state_func(gamepadIndex, &xinputState) == ERROR_DEVICE_NOT_CONNECTED)
    {
      joystick_state->connected = false;
      continue;
    }

    // digital buttons
    WORD buttons = xinputState.Gamepad.wButtons;
    u8 isDown=0;
    u8 wasDown=0;

    const u32 pressedBit = X_JOYSTICK_PRESSED_BIT;

    // Buttons
    isDown = (buttons & XINPUT_GAMEPAD_DPAD_UP) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_DPAD_UP] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_DPAD_UP] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_DPAD_LEFT) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_DPAD_LEFT] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_DPAD_LEFT] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_DPAD_RIGHT] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_DPAD_RIGHT] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_START) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_START] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_START] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_BACK) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_BACK] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_BACK] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_LEFT_THUMB) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_LEFT_THUMB] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_LEFT_THUMB] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_RIGHT_THUMB) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_RIGHT_THUMB] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_RIGHT_THUMB] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_LEFT_SHOULDER] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_LEFT_SHOULDER] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_RIGHT_SHOULDER] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_RIGHT_SHOULDER] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_A) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_A] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_A] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_B) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_B] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_B] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_X) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_X] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_X] = ((isDown ^ wasDown) << 1) | isDown;

    isDown = (buttons & XINPUT_GAMEPAD_Y) > 0;
    wasDown = joystick_state->button[X_JOYSTICK_BUTTON_Y] & pressedBit;
    joystick_state->button[X_JOYSTICK_BUTTON_Y] = ((isDown ^ wasDown) << 1) | isDown;

#define GAMEPAD_AXIS_VALUE(value) (value/(float)(value < 0 ? XINPUT_MIN_AXIS_VALUE * -1: XINPUT_MAX_AXIS_VALUE))
#define GAMEPAD_AXIS_IS_DEADZONE(value, deadzone) ( value > -deadzone && value < deadzone)

    // Left thumb axis
    i32 axisX = xinputState.Gamepad.sThumbLX;
    i32 axisY = xinputState.Gamepad.sThumbLY;
    i32 deadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

    // TODO(marcio): Implement deadZone filtering correctly. This is not enough!
    joystick_state->axis[X_JOYSTICK_AXIS_LX] = GAMEPAD_AXIS_IS_DEADZONE(axisX, deadZone) ? 0.0f :
      GAMEPAD_AXIS_VALUE(axisX);

    joystick_state->axis[X_JOYSTICK_AXIS_LY] = GAMEPAD_AXIS_IS_DEADZONE(axisY, deadZone) ? 0.0f :	
      GAMEPAD_AXIS_VALUE(axisY);

    // Right thumb axis
    axisX = xinputState.Gamepad.sThumbRX;
    axisY = xinputState.Gamepad.sThumbRY;
    deadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

    joystick_state->axis[X_JOYSTICK_AXIS_RX] = GAMEPAD_AXIS_IS_DEADZONE(axisX, deadZone) ? 0.0f :
      GAMEPAD_AXIS_VALUE(axisX);

    joystick_state->axis[X_JOYSTICK_AXIS_RY] = GAMEPAD_AXIS_IS_DEADZONE(axisY, deadZone) ? 0.0f :	
      GAMEPAD_AXIS_VALUE(axisY);

    // Left trigger
    axisX = xinputState.Gamepad.bLeftTrigger;
    axisY = xinputState.Gamepad.bRightTrigger;
    deadZone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

    joystick_state->axis[X_JOYSTICK_AXIS_LTRIGGER] = GAMEPAD_AXIS_IS_DEADZONE(axisX, deadZone) ? 0.0f :	
      axisX/(float) XINPUT_MAX_TRIGGER_VALUE;

    joystick_state->axis[X_JOYSTICK_AXIS_RTRIGGER] = GAMEPAD_AXIS_IS_DEADZONE(axisY, deadZone) ? 0.0f :	
      axisY/(float) XINPUT_MAX_TRIGGER_VALUE;

#undef GAMEPAD_AXIS_IS_DEADZONE
#undef GAMEPAD_AXIS_VALUE

    joystick_state->connected = true;
  }
}

/*** internal prototypes ***/

static LRESULT s_windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static XEvent* s_event_new(void);

static inline HWND s_win32_hwnd_from_xwindow(XWindow w)
{
  return w ? w->handle : NULL; 
}

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 -4
#endif

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE -3
#endif

static void s_enable_per_monitor_v2_awareness(void) {
  HMODULE user32 = GetModuleHandleA("user32.dll");
  BOOL (WINAPI *SetProcessDpiAwarenessContext_)(HANDLE) =
    (void*)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
  if (SetProcessDpiAwarenessContext_) {
    if (SetProcessDpiAwarenessContext_((HANDLE) DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
      return;
    SetProcessDpiAwarenessContext_((HANDLE) DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    return;
  }

  // Win 8.1 fallback
  HMODULE shcore = LoadLibraryA("Shcore.dll");
  if (shcore) {
    HRESULT (WINAPI *SetProcessDpiAwareness_)(int) =
      (void*)GetProcAddress(shcore, "SetProcessDpiAwareness");
    if (SetProcessDpiAwareness_) {
      // PROCESS_PER_MONITOR_DPI_AWARE = 2
      SetProcessDpiAwareness_(2);
      FreeLibrary(shcore);
      return;
    }
    FreeLibrary(shcore);
  }

  // Vista/7 fallback (system-DPI aware only)
  BOOL (WINAPI *SetProcessDPIAware_)(void) =
    (void*)GetProcAddress(GetModuleHandleA("user32.dll"), "SetProcessDPIAware");
  if (SetProcessDPIAware_) SetProcessDPIAware_();
}

/*** Initialization ***/
bool x_plat_init(XPlatformFlags flags, void* user_ctx)
{
  X_UNUSED(user_ctx);
  X_UNUSED(user_ctx);
  s_platform.flags = flags;

  // Enable DPI aware
  s_enable_per_monitor_v2_awareness();

  if (s_platform.default_cursor == 0)
    s_platform.default_cursor = LoadCursor(NULL, IDC_ARROW);


  if (s_platform_graphics_is_opengl(flags))
  {
    const u32 color_bits = 32;
    const u32 depth_bits = 24;
    u32 version_major = 0;
    u32 version_minor = 0;

    if (flags & X_PLATFORM_GRAPHICS_OPENGL_3_0)
    {
      version_major = 3;
      version_minor = 0;
    }
    else if (flags & X_PLATFORM_GRAPHICS_OPENGL_3_3)
    {
      version_major = 3;
      version_minor = 3;
    }

    s_platform.gl = x_gl_init(version_major, version_minor, color_bits, depth_bits);
  }

  return s_xInputInit() && s_platform.gl;
}

void x_plat_terminate(void)
{
}

/*** Windowing ***/

XWindow x_plat_window_create(const char* title, i32 width, i32 height, XWindowStateFlags flags)
{
  const char* window_class = "X_WINDOW_CLASS";
  HINSTANCE hInstance = GetModuleHandleA(NULL);
  WNDCLASSEXA wc = {0};

  // Calculate total window size
  RECT clientArea = {(LONG)0,(LONG)0, (LONG)width, (LONG)height};
  if (!AdjustWindowRect(&clientArea, WS_OVERLAPPEDWINDOW, FALSE))
  {
    fprintf(stderr, "Could not calculate window size");
  }

  if (! GetClassInfoExA(hInstance, window_class, &wc))
  {
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = s_windowProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = window_class;

    // Do not try registering the class multiple times
    if (! RegisterClassExA(&wc))
    {
      fprintf(stderr, "Could not register window class");
      return NULL;
    }
  }

  DWORD windowStyle = WS_OVERLAPPEDWINDOW;
  if (flags & X_WINDOW_STATE_NORESIZE)
    windowStyle &= ~WS_SIZEBOX;

  u32 windowWidth = clientArea.right - clientArea.left;
  u32 windowHeight = clientArea.bottom - clientArea.top;

  XWindow window = (XWindow) malloc(sizeof(struct XWindow_t));
  if (window == NULL)
    return NULL;

  HWND windowHandle = CreateWindowExA(
      0,
      window_class,
      title, 
      windowStyle,
      CW_USEDEFAULT, CW_USEDEFAULT,
      windowWidth, windowHeight,
      NULL, NULL,
      hInstance,
      window);

  if (! windowHandle)
  {
    fprintf(stderr, "Could not create a window");
    return NULL;
  }

  window->handle = windowHandle;
  window->dc = GetDC(windowHandle);
  window->fullscreen = false;
  window->close_flag = false;
  window->prev_style = 0;

  // Associate the native HWND with the XWindow address
  //SetWindowLongPtr(window->handle, GWLP_USERDATA, (LONG_PTR)window);

  DWORD showFlag = (flags & X_WINDOW_STATE_NORMAL) ? SW_SHOWNORMAL : 0;
  showFlag |= (flags & X_WINDOW_STATE_MINIMIZED) ? SW_SHOWMINIMIZED : 0;
  showFlag |= (flags & X_WINDOW_STATE_MAXIMIZED) ? SW_SHOWMAXIMIZED : 0;
  ShowWindow(windowHandle, showFlag);

#if 1

  bool is_opengl = s_platform_graphics_is_opengl(s_platform.flags);
  if(is_opengl)
  {
    int pixelFormat;
    int numPixelFormats = 0;
    PIXELFORMATDESCRIPTOR pfd;

    const int* pixelFormatAttribList  = (const int*) s_platform.gl->pixel_format_attribs;
    const int* contextAttribList      = (const int*) s_platform.gl->context_attribs;

    s_platform.gl->wglChoosePixelFormatARB(window->dc,
        pixelFormatAttribList,
        NULL,
        1,
        &pixelFormat,
        (UINT*) &numPixelFormats);

    if (numPixelFormats <= 0)
    {
      fprintf(stderr, "Unable to get a valid pixel format", 0);
      return NULL;
    }

    if (! SetPixelFormat(window->dc, pixelFormat, &pfd))
    {
      fprintf(stderr, "Unable to set a pixel format", 0);
      return NULL;
    }

    HGLRC sharedContext = s_platform.gl->shared_context;
    HGLRC rc = s_platform.gl->wglCreateContextAttribsARB(window->dc, sharedContext, contextAttribList);

    // The first context created will be used as a shared context for the rest
    // of the program execution
    if (! sharedContext)
    {
      s_platform.gl->shared_context = rc;
    }

    if (! rc)
    {
      fprintf(stderr, "Unable to create a valid OpenGL context", 0);
      return NULL;
    }

    s_platform.gl->rc = rc;
    if (! wglMakeCurrent(window->dc, rc))
    {
      fprintf(stderr, "Unable to set OpenGL context current", 0);
      return NULL;
    }
  }

  SetCursor(s_platform.default_cursor);
#endif
  return window;
}

void x_plat_window_destroy(XWindow win)
{
  DeleteDC(win->dc);
  DestroyWindow(win->handle);
  free(win);
}

XSize x_plat_window_size_get(XWindow win)
{
  RECT rect;
  GetWindowRect(win->handle, &rect);
  XSize size = { rect.right - rect.left, rect.bottom - rect.top };
  return size;
}

void x_plat_window_size_set(XWindow win, i32 width, i32 height)
{
  SetWindowPos(win->handle, NULL, 0, 0, width, height, SWP_NOMOVE);
}

XSize x_plat_window_client_size_get(XWindow win)
{
  RECT rect;
  GetClientRect(win->handle, &rect);
  XSize size = { .w = rect.right, .h = rect.bottom };
  return size;
}

void x_plat_window_client_size_set(XWindow win, i32 width, i32 height)
{
  RECT clientArea = {(LONG)0,(LONG)0, (LONG)width, (LONG)height};
  if (!AdjustWindowRect(&clientArea, WS_OVERLAPPEDWINDOW, FALSE))
    return;

  u32 windowWidth = clientArea.right - clientArea.left;
  u32 windowHeight = clientArea.bottom - clientArea.top;
  x_plat_window_size_set(win, windowWidth, windowHeight);
}

XPoint x_plat_window_position_get(XWindow win)
{
  RECT rect;
  GetClientRect(win->handle, &rect);
  XPoint position = { .x = rect.right, .y = rect.bottom };
  return position;
}

void x_plat_window_position_set(XWindow win, i32 x, i32 y)
{
  SetWindowPos(win->handle, NULL, x, y, 0, 0, SWP_NOSIZE);
}

bool x_plat_window_fullscreen_get(XWindow win)
{
  return win->fullscreen;
}

bool x_plat_window_fullscreen_set(XWindow win, bool enabled)
{
  HWND hWnd = win->handle;

  if (enabled && !win->fullscreen) // Enter full screen
  {
    // Get the monitor's handle
    HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    // Get the monitor's info
    MONITORINFO monitorInfo = { sizeof(monitorInfo) };
    GetMonitorInfo(hMonitor, &monitorInfo);
    // Save the window's current style and position
    win->prev_style = GetWindowLongPtr(hWnd, GWL_STYLE);
    win->prev_placement = win->prev_placement;
    GetWindowPlacement(hWnd, &win->prev_placement);

    // Set the window style to full screen
    SetWindowLongPtr(hWnd, GWL_STYLE, win->prev_style & ~(WS_CAPTION | WS_THICKFRAME));
    SetWindowPos(hWnd, NULL,
        monitorInfo.rcMonitor.left,
        monitorInfo.rcMonitor.top,
        monitorInfo.rcMonitor.right,
        monitorInfo.rcMonitor.bottom,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    // Set the display settings to full screen
    DEVMODE dmScreenSettings = { 0 };
    dmScreenSettings.dmSize = sizeof(dmScreenSettings);
    dmScreenSettings.dmPelsWidth = monitorInfo.rcMonitor.right;
    dmScreenSettings.dmPelsHeight = monitorInfo.rcMonitor.bottom;
    dmScreenSettings.dmBitsPerPel = 32;
    dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    LONG result = ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);
    if (result == DISP_CHANGE_FAILED)
    {
      //debugLogError("Failed to enter fullScreen mode");
      return false;
    }
    // Show the window in full screen mode
    ShowWindow(hWnd, SW_MAXIMIZE);
    win->fullscreen = true;
  }
  else if (!enabled && win->fullscreen) // Exit full screen
  {
    // restore window previous style and location
    SetWindowLongPtr(hWnd, GWL_STYLE, win->prev_style);
    SetWindowPlacement(hWnd, &win->prev_placement);
    ShowWindow(hWnd, SW_RESTORE);
    win->fullscreen = false;
  }
  return true;
}

bool x_plat_window_icon_set(XWindow win, const char* icon_path)
{
  HICON hIcon = (HICON)LoadImage(NULL, icon_path, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);

  if (hIcon == NULL)
  {
    fprintf(stderr, "Failed to load icon '%s'", icon_path);
    return FALSE;
  }

  SendMessage(win->handle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
  SendMessage(win->handle, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
  return TRUE;
}

static inline bool x_plat_window_close_flag_get(XWindow win)
{
  return win->close_flag;
}

static inline void x_plat_window_close_flag_set(XWindow win)
{
  win->close_flag = true;
}

static inline void x_plat_window_close_flag_clear(XWindow win)
{
  win->close_flag = false;
}

void x_plat_window_swap_buffers(XWindow win)
{
  SwapBuffers(win->dc);
}

size_t x_plat_window_title_len(XWindow win)
{
  return GetWindowTextLength(win->handle);
}

size_t x_plat_window_title_get(XWindow win, char* out_buf, size_t buf_cap)
{
  size_t length = GetWindowTextLengthA(win->handle);

  if (length >= buf_cap)
    return length;

  return (size_t) GetWindowTextA(win->handle, out_buf, (i32) buf_cap);
}

void x_plat_window_title_set(XWindow win, const char* title)
{
  SetWindowTextA(win->handle, title);
}

/*** Events ***/

/** 
 * Waits for OS events. 
 * timeout_ms < 0 → block indefinitely.
 */
void x_plat_events_wait(i32 timeout_ms)
{
  // Drain anything already pending so we don't immediately block.
  MSG msg;
  if (PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE))
  {
    return; // Messages already queued; let the caller poll/process them.
  }

  if (timeout_ms < 0)
  {
    // Block until any new message arrives.
    // WaitMessage wakes on user input, posted messages, timers, etc.
    // It does NOT remove the message; it only unblocks us.
    // We also handle the (rare) case where WaitMessage fails spuriously.
    for (;;)
    {
      if (PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE))
        break;
      if (WaitMessage())
        break; // Something arrived; caller can now poll/process.
               // If WaitMessage failed, loop and try again (GetLastError optional).
    }
  }
  else
  {
    // Timed wait: use the message queue as a waitable "object".
    // QS_ALLINPUT means wake on any kind of input/message.
    // MWMO_INPUTAVAILABLE wakes even if the message is already queued
    // (so we won't oversleep when something arrived just before the call).
    DWORD r = MsgWaitForMultipleObjectsEx(
        0, NULL,
        (DWORD)timeout_ms,
        QS_ALLINPUT,
        MWMO_INPUTAVAILABLE
        );
    X_UNUSED(r); // No need to branch on r here; caller will poll next.
  }
}

bool x_plat_event_poll(XEvent* event)
{
  MSG msg;

  if (s_platform.events_count == 0)
  {
    // clean up changed bit for keyboard keys
    for(int keyCode = 0; keyCode < X_KEYBOARD_MAX_KEYS; keyCode++)
    {
      s_platform.keyboard_state.key[keyCode] &= ~X_KEYBOARD_CHANGED_THIS_FRAME_BIT;
    }

    // clean up changed bit for mouse buttons
    for(int button = 0; button < X_MOUSE_MAX_BUTTONS; button++)
    {
      s_platform.mouse_sate.button[button] &= ~X_MOUSE_CHANGED_THIS_FRAME_BIT;
    }

    // reset wheel delta
    s_platform.mouse_sate.wheelDelta = 0;
    s_platform.mouse_sate.cursorRelative.x = 0;
    s_platform.mouse_sate.cursorRelative.y = 0;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  // clean up changed bit for joystick buttons
  for (u32 joystickId = 0; joystickId < X_JOYSTICK_MAX; joystickId++)
  {
    for(int button = 0; button < X_MOUSE_MAX_BUTTONS; button++)
    {
      s_platform.joystick_state[joystickId].button[button] &= ~X_JOYSTICK_CHANGED_THIS_FRAME_BIT;
    }
  }

  s_XInputPollEvents();

  // WindowProc might have enqueued some events...
  if (s_platform.events_poll_index < s_platform.events_count)
  {
    memcpy(event, &s_platform.events[s_platform.events_poll_index++], sizeof(XEvent));
    return true;
  }
  else
  {
    s_platform.events_count = 0;
    s_platform.events_poll_index = 0;
    event->type = X_EVENT_TYPE_NONE;
    return false;
  }
}

/*** Utility funcitons ***/

void x_plat_msg_box(const char* title, const char* msg)
{
  MessageBox(0, msg, title, 0);
}

bool x_plat_msg_box_yes_no(const char* title, const char* msg)
{
  return MessageBox(0, msg, title, MB_YESNO) == IDYES;
}

static XEvent* s_event_new(void)
{
  if (s_platform.events_count >= X_PLATFORM_MAX_EVENTS)
  {
    fprintf(stderr, "Reached the maximum number of OS events %d.\n", X_PLATFORM_MAX_EVENTS);
    s_platform.events_count = 0;
  }

  XEvent* eventPtr = &(s_platform.events[s_platform.events_count++]);
  return eventPtr;
}

static inline XWindow xw_from_msg(HWND h, UINT msg, LPARAM lp)
{
  if (msg == WM_NCCREATE)
  {
    const CREATESTRUCTW* cs = (const CREATESTRUCTW*)lp;
    XWindow xw = (XWindow)cs->lpCreateParams;   // passed via CreateWindowExW(..., lpParam=xw)
    if (!xw) return NULL;                       // abort in proc if this happens
    xw->handle = h;                             // roundtrip store
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)xw);
    return xw;
  }
  return (XWindow)GetWindowLongPtrW(h, GWLP_USERDATA);
}

/**
 * Some messages can be sent to the window before we have the opportunity to
 * bind the HWND to the XWindow address. We do not use the XWindow pointer on
 * those messages.
 */
static bool xw_expected_fully_created(UINT msg)
{
  switch (msg)
  {
    case WM_GETMINMAXINFO:
    case WM_NCCREATE:
    case WM_NCCALCSIZE:
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
    case WM_QUERYDRAGICON:
    case WM_GETICON:
    case WM_NCHITTEST:
    case WM_WINDOWPOSCHANGING:
    case WM_THEMECHANGED:
    case WM_SYSCOLORCHANGE:
      return false;
    default:
      return true;
  }
}

static LRESULT s_windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  bool isMouseButtonDownEvent = false;
  i32 mouseButtonId = -1;
  LRESULT returnValue = FALSE;

  XWindow window = xw_from_msg(hwnd, uMsg, lParam);
  if (xw_expected_fully_created(uMsg))
    X_ASSERT(window != NULL);

  switch(uMsg) 
  {
    case WM_NCHITTEST:
      {
        // Get the default behaviour but set the arrow cursor if it's in the client area
        LRESULT result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        if(result == HTCLIENT && window->cursor_changed)
        {
          window->cursor_changed = false;
          SetCursor(s_platform.default_cursor);
        }
        else
          window->cursor_changed = true;
        return result;
      }
      break;

    case WM_KILLFOCUS:
    case WM_ACTIVATE:
      {
        if (wParam == WA_INACTIVE)
        {
          if (GetCapture() == hwnd)
          {
            ReleaseCapture();
            window->relative_mode = false;
          }
        }


        s_window_clip_rect_update(window);

        bool activate = (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE);
        XEvent* e = s_event_new();
        e->type = X_EVENT_TYPE_WINDOW;
        e->windowEvent.type = activate ? X_WINDOW_EVENT_ACTIVATE : X_WINDOW_EVENT_DEACTIVATE;
        e->window = window;
      }
      break;
    case WM_CHAR:
      {
        XEvent* e = s_event_new();
        e->type                = X_EVENT_TYPE_TEXT;
        e->window = window;
        e->textEvent.character = (u32) wParam;
        e->textEvent.type      = ((u32) wParam == VK_BACK) ? X_TEXT_EVENT_BACKSPACE: X_TEXT_EVENT_CHARACTER_INPUT;
      }
      break;

    case WM_MOVE:
      s_window_clip_rect_update(window);
      break;

    case WM_SIZE:
      {
        //WM_SIZE is sent a lot of times in a row and would easily overflow our
        //event buffer. We just update the last event if it is a X_EVENT_TYPE_WINDOW instead of adding a new event
        XEvent* e = NULL;
        u32 lastEventIndex = s_platform.events_count - 1;
        if (s_platform.events_count > 0 && s_platform.events[lastEventIndex].type == X_EVENT_TYPE_WINDOW)
        {
          if(s_platform.events[lastEventIndex].windowEvent.type == X_WINDOW_EVENT_RESIZED)
            e = &s_platform.events[lastEventIndex];
        }

        s_window_clip_rect_update(window);

        if (e == NULL)
          e = s_event_new();

        e->type               = X_EVENT_TYPE_WINDOW;
        e->window             = window;
        e->windowEvent.type   = wParam == SIZE_MAXIMIZED ? X_WINDOW_EVENT_MAXIMIZED :
          wParam == SIZE_MINIMIZED ? X_WINDOW_EVENT_MINIMIZED : X_WINDOW_EVENT_RESIZED;
        e->windowEvent.width  = LOWORD(lParam);
        e->windowEvent.height = HIWORD(lParam);
      }
      break;

    case WM_KEYDOWN:
    case WM_KEYUP:
      {
        i32 isDown = !(lParam & (1 << 31)); // 0 = pressed, 1 = released
        i32 wasDown = (lParam & (1 << 30)) !=0;
        i32 state = (((isDown ^ wasDown) << 1) | isDown);
        i16 vkCode = (i16) wParam;
        s_platform.keyboard_state.key[vkCode] = (u8) state;

        XEvent* e = s_event_new();
        e->type = X_EVENT_TYPE_KEYBOARD;
        e->window = window;
        e->keyboardEvent.type = (wasDown && !isDown) ?
          X_KEYBOARD_EVENT_KEY_UP : (!wasDown && isDown) ?
          X_KEYBOARD_EVENT_KEY_DOWN : X_KEYBOARD_EVENT_KEY_HOLD;

        e->keyboardEvent.keyCode      = vkCode;
        e->keyboardEvent.ctrlIsDown   = s_platform.keyboard_state.key[X_KEYCODE_CONTROL];
        e->keyboardEvent.shiftIsDown  = s_platform.keyboard_state.key[X_KEYCODE_SHIFT];
        e->keyboardEvent.altIsDown    = s_platform.keyboard_state.key[X_KEYCODE_ALT];
      }
      break;

    case WM_MOUSEWHEEL:
      {
        i32 delta = GET_WHEEL_DELTA_WPARAM(wParam);
        s_platform.mouse_sate.wheelDelta = delta;

        // update cursor position
        i32 lastX = s_platform.mouse_sate.cursor.x;
        i32 lastY = s_platform.mouse_sate.cursor.y;
        s_platform.mouse_sate.cursor.x = GET_X_LPARAM(lParam);
        s_platform.mouse_sate.cursor.y = GET_Y_LPARAM(lParam); 
        s_platform.mouse_sate.cursorRelative.x = s_platform.mouse_sate.cursor.x - lastX;
        s_platform.mouse_sate.cursorRelative.y = s_platform.mouse_sate.cursor.y - lastY;

        XEvent* e = s_event_new();
        e->type = X_EVENT_TYPE_MOUSE_WHEEL;
        e->window = window;
        e->mouseEvent.wheelDelta = delta;
        e->mouseEvent.type = delta >= 0 ? X_MOUSE_EVENT_WHEEL_FORWARD : X_MOUSE_EVENT_WHEEL_BACKWARD;
        e->mouseEvent.cursorX = GET_X_LPARAM(lParam); 
        e->mouseEvent.cursorY = GET_Y_LPARAM(lParam); 
        e->mouseEvent.xRel    = s_platform.mouse_sate.cursorRelative.x;
        e->mouseEvent.yRel    = s_platform.mouse_sate.cursorRelative.y;
      }
      break;

    case WM_MOUSEMOVE:
      {
        // ignore forced move
        if (window->relative_mode && window->warping)
        {
          window->warping = FALSE;
          return 0;
        } 

        i32 lastX = s_platform.mouse_sate.cursor.x;
        i32 lastY = s_platform.mouse_sate.cursor.y;
        s_platform.mouse_sate.cursor.x = GET_X_LPARAM(lParam);
        s_platform.mouse_sate.cursor.y = GET_Y_LPARAM(lParam); 
        s_platform.mouse_sate.cursorRelative.x = s_platform.mouse_sate.cursor.x - lastX;
        s_platform.mouse_sate.cursorRelative.y = s_platform.mouse_sate.cursor.y - lastY;

        // if we're near an edge, recenter to avoid getting “stuck”
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ClientToScreen(hwnd, &pt);

        const int edge = 8; // pixels from client edge
        RECT cr; GetClientRect(hwnd, &cr);
        POINT tl = { cr.left, cr.top }, br = { cr.right, cr.bottom };
        ClientToScreen(hwnd, &tl); ClientToScreen(hwnd, &br);

        if (pt.x - tl.x < edge || br.x - pt.x < edge || 
            pt.y - tl.y < edge || br.y - pt.y < edge) {
          window->warping = TRUE;
          SetCursorPos(window->relative_center.x, window->relative_center.y);
        }

        XEvent* e = s_event_new();
        e->type = X_EVENT_TYPE_MOUSE_MOVE;
        e->mouseEvent.xRel    = e->mouseEvent.cursorX - lastX;
        e->mouseEvent.yRel    = e->mouseEvent.cursorY - lastY;
        e->window = window;
        e->mouseEvent.type = X_MOUSE_EVENT_MOVE;
        e->mouseEvent.cursorX = GET_X_LPARAM(lParam); 
        e->mouseEvent.cursorY = GET_Y_LPARAM(lParam); 
        e->mouseEvent.xRel    = s_platform.mouse_sate.cursorRelative.x;
        e->mouseEvent.yRel    = s_platform.mouse_sate.cursorRelative.y;
      }
      break;

    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
      if (mouseButtonId == -1)
        mouseButtonId = GET_XBUTTON_WPARAM (wParam) == XBUTTON1 ? X_MOUSE_BUTTON_EXTRA_0 : X_MOUSE_BUTTON_EXTRA_1;
      returnValue = TRUE;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
      if (mouseButtonId == -1)
        mouseButtonId = X_MOUSE_BUTTON_LEFT;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
      if (mouseButtonId == -1)
        mouseButtonId = X_MOUSE_BUTTON_MIDDLE;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
      {
        if (mouseButtonId == -1)
          mouseButtonId = X_MOUSE_BUTTON_RIGHT;

        isMouseButtonDownEvent = uMsg == WM_XBUTTONDOWN || uMsg == WM_LBUTTONDOWN || uMsg == WM_MBUTTONDOWN || uMsg == WM_RBUTTONDOWN;

        unsigned char* buttonLeft   = (unsigned char*) &(s_platform.mouse_sate.button[X_MOUSE_BUTTON_LEFT]);
        unsigned char* buttonRight  = (unsigned char*) &(s_platform.mouse_sate.button[X_MOUSE_BUTTON_RIGHT]);
        unsigned char* buttonMiddle = (unsigned char*) &(s_platform.mouse_sate.button[X_MOUSE_BUTTON_MIDDLE]);
        unsigned char* buttonExtra1 = (unsigned char*) &(s_platform.mouse_sate.button[X_MOUSE_BUTTON_EXTRA_0]);
        unsigned char* buttonExtra2 = (unsigned char*) &(s_platform.mouse_sate.button[X_MOUSE_BUTTON_EXTRA_1]);
        unsigned char isDown, wasDown;

        isDown        = (unsigned char) ((wParam & MK_LBUTTON) > 0);
        wasDown       = *buttonLeft;
        *buttonLeft   = (((isDown ^ wasDown) << 1) | isDown);

        isDown        = (unsigned char) ((wParam & MK_RBUTTON) > 0);
        wasDown       = *buttonRight;
        *buttonRight  = (((isDown ^ wasDown) << 1) | isDown);

        isDown        = (unsigned char) ((wParam & MK_MBUTTON) > 0);
        wasDown       = *buttonMiddle;
        *buttonMiddle = (((isDown ^ wasDown) << 1) | isDown);

        isDown        = (unsigned char) ((wParam & MK_XBUTTON1) > 0);
        wasDown       = *buttonExtra1;
        *buttonExtra1 = (((isDown ^ wasDown) << 1) | isDown);

        isDown        = (unsigned char) ((wParam & MK_XBUTTON2) > 0);
        wasDown       = *buttonExtra2;
        *buttonExtra2 = (((isDown ^ wasDown) << 1) | isDown);

        // update cursor position
        i32 lastX = s_platform.mouse_sate.cursor.x;
        i32 lastY = s_platform.mouse_sate.cursor.y;
        s_platform.mouse_sate.cursor.x = GET_X_LPARAM(lParam);
        s_platform.mouse_sate.cursor.y = GET_Y_LPARAM(lParam); 
        s_platform.mouse_sate.cursorRelative.x = s_platform.mouse_sate.cursor.x - lastX;
        s_platform.mouse_sate.cursorRelative.y = s_platform.mouse_sate.cursor.y - lastY;

        XEvent* e = s_event_new();
        e->type = X_EVENT_TYPE_MOUSE_BUTTON;
        e->window = window;
        e->mouseEvent.type = isMouseButtonDownEvent ? X_MOUSE_EVENT_BUTTON_DOWN : X_MOUSE_EVENT_BUTTON_UP;
        e->mouseEvent.mouseButton = mouseButtonId;
        e->mouseEvent.cursorX = GET_X_LPARAM(lParam);
        e->mouseEvent.cursorY = GET_Y_LPARAM(lParam);
        e->mouseEvent.xRel    = s_platform.mouse_sate.cursorRelative.x;
        e->mouseEvent.yRel    = s_platform.mouse_sate.cursorRelative.y;
      }
      break;

    case WM_DPICHANGED:
      {
        // Suggested new size/pos in lParam
        RECT* const prcNewWindow = (RECT*)lParam;
        SetWindowPos(hwnd, NULL,
            prcNewWindow->left, prcNewWindow->top,
            prcNewWindow->right - prcNewWindow->left,
            prcNewWindow->bottom - prcNewWindow->top,
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);

        XEvent* e = s_event_new();
        e->window = window;
        e->type = X_EVENT_TYPE_WINDOW;
        e->windowEvent.type = X_WINDOW_EVENT_CONTENT_SCALE_CHANGED;
        e->windowEvent.width  = (u32)(prcNewWindow->right  - prcNewWindow->left);
        e->windowEvent.height = (u32)(prcNewWindow->bottom - prcNewWindow->top);
      } break;

    case WM_CLOSE:
      {
        XEvent* e = s_event_new();
        e->type = X_EVENT_TYPE_WINDOW;
        e->windowEvent.type = X_WINDOW_EVENT_CLOSE;
        e->window = window;
        window->close_flag = true;
      }
      break;
    default:
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
  return returnValue;
}

/** Keyboard input */

void x_plat_keyboard_state_get(XKeyboardState* outState)
{
  memcpy(outState, &s_platform.keyboard_state, sizeof(XKeyboardState));
}

bool x_plat_keyboard_key_is_pressed(const XKeyboardState* state, XKeycode keycode)
{
  return (state->key[keycode] & X_KEYBOARD_PRESSED_BIT) == X_KEYBOARD_PRESSED_BIT;
}

bool x_plat_keyboard_key_down(const XKeyboardState* state, XKeycode keycode)
{
  u32 mask = X_KEYBOARD_PRESSED_BIT | X_KEYBOARD_CHANGED_THIS_FRAME_BIT;
  return (state->key[keycode] & mask) == mask;
}

bool x_plat_keyboard_key_up(const XKeyboardState* state, XKeycode keycode)
{
  return state->key[keycode] == X_KEYBOARD_CHANGED_THIS_FRAME_BIT;
}

/*** Mouse input ***/

void x_plat_mouse_state_get(XMouseState* outState)
{
  memcpy(outState, &s_platform.mouse_sate, sizeof(XMouseState));
}

bool x_plat_mouse_button_is_pressed(const XMouseState* state, XMouseButton button)
{
  return (state->button[button] & X_MOUSE_PRESSED_BIT) == X_MOUSE_PRESSED_BIT;
}

bool x_plat_mouse_button_down(const XMouseState* state, XMouseButton button)
{
  u32 mask = X_MOUSE_PRESSED_BIT | X_MOUSE_CHANGED_THIS_FRAME_BIT;
  return (state->button[button] & mask) == mask;
}

bool x_plat_mouse_button_up(const XMouseState* state, XMouseButton button)
{
  return state->button[button] == X_MOUSE_CHANGED_THIS_FRAME_BIT;
}

XPoint x_plat_mouse_cursor(const XMouseState* state)
{
  return state->cursor;
}

XPoint x_plat_mouse_cursor_relative(const XMouseState* state)
{
  return state->cursorRelative;
}

void x_plat_window_mouse_capture_set(XWindow win, bool enabled)
{
  HWND h = s_win32_hwnd_from_xwindow(win);
  if (!h) return;
  if (enabled)
  {
    SetCapture(h);
  }
  else
  {
    if (GetCapture() == h) ReleaseCapture();
  }
  win->relative_mode = enabled;
}

bool x_plat_window_mouse_capture_get(XWindow win)
{
  return win->relative_mode;
}

i32 x_plat_mouse_wheel_delta(const XMouseState* state)
{
  return state->wheelDelta;
}

/*** Joystick input ***/

void x_plat_joystick_state_get(XJoystickState* out_state, XJoystickID id)
{
  X_ASSERT( id == X_JOYSTICK_0 || id == X_JOYSTICK_1 || id == X_JOYSTICK_2 || id == X_JOYSTICK_3);
  memcpy(out_state, &s_platform.joystick_state[id], sizeof(XJoystickState));
}

bool x_plat_joystick_button_is_pressed(const XJoystickState* state, XJoystickButton key)
{
  return 	state->connected && (state->button[key] & X_JOYSTICK_PRESSED_BIT) == X_JOYSTICK_PRESSED_BIT;
}

bool x_plat_joystick_button_down(const XJoystickState* state, XJoystickButton key)
{
  u32 mask = X_JOYSTICK_PRESSED_BIT | X_JOYSTICK_CHANGED_THIS_FRAME_BIT;
  return 	state->connected && (state->button[key] & mask) == mask;
}

bool x_plat_joystick_button_up(const XJoystickState* state, XJoystickButton key)
{
  return state->connected && state->button[key] == X_JOYSTICK_CHANGED_THIS_FRAME_BIT;
}

float x_plat_joystick_axis_get(const XJoystickState* state, XJoystickAxis axis)
{
  if (!state->connected)
    return 0.0f;

  return state->axis[axis];
}

u32 x_plat_joystick_count(void)
{
  u32 count = 0;
  for (u32 i = 0; i < X_JOYSTICK_MAX; i++)
  {
    if (s_platform.joystick_state[i].connected)
      count++;
  }
  return count;
}

u32 x_plat_joystick_is_connected(XJoystickID id)
{
  const bool connected = (s_platform.joystick_state[id].connected);
  return connected;
}

void x_plat_joystick_vibration_left_set(XJoystickID id, float speed)
{
  X_ASSERT( id == X_JOYSTICK_0 || id == X_JOYSTICK_1 || id == X_JOYSTICK_2 || id == X_JOYSTICK_3);
  if (!s_platform.joystick_state[id].connected)
    return;

  XINPUT_VIBRATION vibration;
  if (speed < 0.0f) speed = 0.0f;
  if (speed > 1.0f) speed = 1.0f;

  // we store speed as floats
  s_platform.joystick_state[id].vibrationLeft = speed;

  // xinput wants them as short int
  WORD speed_left = (WORD) (0xFFFF * speed);
  WORD speed_right = (WORD) (s_platform.joystick_state[id].vibrationRight * 0XFFFF);

  vibration.wLeftMotorSpeed = speed_left;
  vibration.wRightMotorSpeed = speed_right;
  s_platform.xinput_set_state_func(id, &vibration);
}

void x_plat_joystick_vibration_right_set(XJoystickID id, float speed)
{
  X_ASSERT( id == X_JOYSTICK_0 || id == X_JOYSTICK_1 || id == X_JOYSTICK_2 || id == X_JOYSTICK_3);
  if (!s_platform.joystick_state[id].connected)
    return;

  XINPUT_VIBRATION vibration;
  if (speed < 0.0f) speed = 0.0f;
  if (speed > 1.0f) speed = 1.0f;

  // we store speed as floats
  s_platform.joystick_state[id].vibrationRight = speed;

  // xinput wants them as short int
  WORD speed_left = (WORD) (s_platform.joystick_state[id].vibrationLeft * 0XFFFF);
  WORD speed_right = (WORD) (0xFFFF * speed);

  vibration.wLeftMotorSpeed = speed_left;
  vibration.wRightMotorSpeed = speed_right;
  s_platform.xinput_set_state_func(id, &vibration);
}

float x_plat_joystick_vibration_left_get(XJoystickID id)
{
  X_ASSERT( id == X_JOYSTICK_0 || id == X_JOYSTICK_1 || id == X_JOYSTICK_2 || id == X_JOYSTICK_3);
  return s_platform.joystick_state[id].vibrationLeft;
}

float x_plat_joystick_vibration_right_get(XJoystickID id)
{
  X_ASSERT( id == X_JOYSTICK_0 || id == X_JOYSTICK_1 || id == X_JOYSTICK_2 || id == X_JOYSTICK_3);
  return s_platform.joystick_state[id].vibrationRight;
}

/* Dynamic load for GetDpiForMonitor to avoid hard link on older Windows */
typedef HRESULT (WINAPI *PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
static PFN_GetDpiForMonitor s_GetDpiForMonitor(void)
{
  static PFN_GetDpiForMonitor p = NULL;
  static int tried = 0;
  if (!tried)
  {
    HMODULE h = LoadLibraryA("Shcore.dll");
    if (h) p = (PFN_GetDpiForMonitor)GetProcAddress(h, "GetDpiForMonitor");
    tried = 1;
  }
  return p;
}

/* DPI helpers */
static float s_dpi_to_scale(UINT dpi) { return dpi ? (float)dpi / 96.0f : 1.0f; }

/* Cursor state */

/* Client rect in screen coords */
static BOOL s_client_rect_screen(HWND hwnd, RECT* out)
{
  RECT r;
  if (!GetClientRect(hwnd, &r)) return FALSE;
  POINT tl = { r.left, r.top }, br = { r.right, r.bottom };
  if (!ClientToScreen(hwnd, &tl) || !ClientToScreen(hwnd, &br)) return FALSE;
  out->left = tl.x; out->top = tl.y; out->right = br.x; out->bottom = br.y;
  return TRUE;
}

/*** Clipboard (UTF-8) ***/

size_t x_plat_clipboard_text_len(void)
{
  size_t result = 0;
  if (!OpenClipboard(NULL)) return 0;
  HANDLE h = GetClipboardData(CF_UNICODETEXT);
  if (h)
  {
    const wchar_t* w = (const wchar_t*)GlobalLock(h);
    if (w)
    {
      int need = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
      if (need > 0) result = (size_t)(need - 1); /* exclude NUL */
      GlobalUnlock(h);
    }
  }
  CloseClipboard();
  return result;
}

bool x_plat_clipboard_text_set(const char* text, size_t len)
{
  if (!text) return false;
  if (len == 0) len = (size_t)strlen(text);

  /* UTF-8 → UTF-16 */
  int wlen = MultiByteToWideChar(CP_UTF8, 0, text, (int)len, NULL, 0);
  if (wlen <= 0) return false;

  HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)((wlen + 1) * sizeof(wchar_t)));
  if (!hglob) return false;

  wchar_t* wbuf = (wchar_t*)GlobalLock(hglob);
  if (!wbuf) { GlobalFree(hglob); return false; }

  MultiByteToWideChar(CP_UTF8, 0, text, (int)len, wbuf, wlen);
  wbuf[wlen] = L'\0';
  GlobalUnlock(hglob);

  if (!OpenClipboard(NULL)) { GlobalFree(hglob); return false; }
  EmptyClipboard();
  if (!SetClipboardData(CF_UNICODETEXT, hglob))
  {
    CloseClipboard();
    GlobalFree(hglob);
    return false;
  }
  CloseClipboard();
  /* ownership of hglob transferred to clipboard */
  return true;
}

size_t x_plat_clipboard_text_get(char* out, size_t cap)
{
  size_t written = 0;
  if (!OpenClipboard(NULL)) return 0;
  HANDLE h = GetClipboardData(CF_UNICODETEXT);
  if (h)
  {
    const wchar_t* w = (const wchar_t*)GlobalLock(h);
    if (w)
    {
      if (!out || cap == 0)
      {
        int need = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
        if (need > 0) written = (size_t)(need - 1);
      }
      else
      {
        int need = WideCharToMultiByte(CP_UTF8, 0, w, -1, out, (int)cap, NULL, NULL);
        if (need > 0) { written = (size_t)(need - 1); out[(need-1) < (int)cap ? (need-1) : (int)cap-1] = '\0'; }
        else if (cap) out[0] = '\0';
      }
      GlobalUnlock(h);
    }
  }
  CloseClipboard();
  return written;
}

/*** Cursor control ***/
void x_plat_cursor_visible_set(bool visible)
{
  /* Win32 keeps a display count; normalize to requested state. */
  CURSORINFO ci = { sizeof(CURSORINFO) };
  s_platform.cursor_visible = visible ? TRUE : FALSE;

  /* Best-effort normalization loop (bounded) */
  for (int i = 0; i < 8; ++i)
  {
    if (!GetCursorInfo(&ci)) break;
    BOOL isShown = (ci.flags & CURSOR_SHOWING) != 0;
    if (isShown == s_platform.cursor_visible) break;
    ShowCursor(s_platform.cursor_visible);
  }
}

bool x_plat_cursor_visible_get(void)
{
  CURSORINFO ci = { sizeof(CURSORINFO) };
  if (!GetCursorInfo(&ci)) return s_platform.cursor_visible ? true : false;
  return (ci.flags & CURSOR_SHOWING) != 0;
}

static HCURSOR s_cursor_from_shape(XCursorShape s)
{
  LPCSTR idc = IDC_ARROW;
  switch (s)
  {
    default:
    case X_CURSOR_DEFAULT:    idc = IDC_ARROW; break;
    case X_CURSOR_TEXT:       idc = IDC_IBEAM; break;
    case X_CURSOR_HAND:       idc = IDC_HAND;  break;
    case X_CURSOR_CROSSHAIR:  idc = IDC_CROSS; break;
    case X_CURSOR_RESIZE_NS:  idc = IDC_SIZENS; break;
    case X_CURSOR_RESIZE_EW:  idc = IDC_SIZEWE; break;
    case X_CURSOR_RESIZE_NESW:idc = IDC_SIZENESW; break;
    case X_CURSOR_RESIZE_NWSE:idc = IDC_SIZENWSE; break;
    case X_CURSOR_RESIZE_ALL: idc = IDC_SIZEALL; break;
    case X_CURSOR_NOT_ALLOWED:idc = IDC_NO; break;
  }
  return LoadCursorA(NULL, idc);
}

void x_plat_cursor_shape_set(XCursorShape shape)
{
  s_platform.cursor_current = s_cursor_from_shape(shape);
  SetCursor(s_platform.cursor_current);
}

void x_plat_cursor_confine_set(XWindow win, bool enabled) {
  HWND hwnd = s_win32_hwnd_from_xwindow(win);
  if (!hwnd) return;

  if (enabled) {
    // Avoid clipping to a zero/invalid rect (e.g., before first ShowWindow)
    if (!IsWindowVisible(hwnd)) return;

    RECT r;
    if (s_client_rect_in_screen_get(hwnd, &r) &&
        r.right > r.left && r.bottom > r.top)
    {
      ClipCursor(&r);
    }
  } else {
    ClipCursor(NULL);
  }
}

bool x_plat_cursor_confine_get(XWindow win)
{
  (void)win;
  RECT r;
  BOOL ok = GetClipCursor(&r);
  /* If clip rect equals full virtual screen, consider it "not confined" */
  if (!ok) return false;
  RECT full = { GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
    GetSystemMetrics(SM_XVIRTUALSCREEN)+GetSystemMetrics(SM_CXVIRTUALSCREEN),
    GetSystemMetrics(SM_YVIRTUALSCREEN)+GetSystemMetrics(SM_CYVIRTUALSCREEN) };
  return !(r.left == full.left && r.top == full.top && r.right == full.right && r.bottom == full.bottom);
}

bool x_plat_cursor_relative_mode_set(XWindow win, bool enabled)
{
  win->relative_mode = enabled ? TRUE : FALSE;
  win->warping = FALSE;
  x_plat_window_mouse_capture_set(win, enabled);   // capture so you keep getting moves
  x_plat_cursor_visible_set(!enabled);             // hide when in look mode
  x_plat_cursor_confine_set(win, enabled);         // optional: keep inside window
  if (enabled)
  {
    RECT r; GetClientRect(win->handle, &r);
    POINT tl = { r.left, r.top }, br = { r.right, r.bottom };
    ClientToScreen(win->handle, &tl); ClientToScreen(win->handle, &br);
    win->relative_center.x = (tl.x + br.x)/2; 
    win->relative_center.y = (tl.y + br.y)/2;
    win->warping = TRUE;
    SetCursorPos(win->relative_center.x, win->relative_center.y);
  }
  return true;
}

bool x_plat_cursor_relative_mode_get(XWindow win)
{
  return win->relative_mode;
}

/*** Monitors & DPI ***/

typedef struct
{
  XMonitorInfo* arr;
  u32           max;
  u32           count;
  u32           primary_index;
} s_MonEnumCtx;

static BOOL CALLBACK s_enum_mon_proc(HMONITOR hMon, HDC hdc, LPRECT rect, LPARAM lp)
{
  X_UNUSED(hdc);
  X_UNUSED(rect);
  s_MonEnumCtx* ctx = (s_MonEnumCtx*)lp;
  if (ctx->count >= ctx->max) return TRUE;

  MONITORINFOEXW mi;
  mi.cbSize = sizeof(mi);
  if (!GetMonitorInfoW(hMon, (MONITORINFO*)&mi)) return TRUE;

  XMonitorInfo* out = &ctx->arr[ctx->count];
  ZeroMemory(out, sizeof(*out));

  /* Name */
  int nbytes = WideCharToMultiByte(CP_UTF8, 0, mi.szDevice, -1, out->name, (int)X_ARRAY_COUNT(out->name), NULL, NULL);
  if (nbytes <= 0) { out->name[0] = 0; }

  out->primary = (mi.dwFlags & MONITORINFOF_PRIMARY) ? true : false;
  if (out->primary) ctx->primary_index = ctx->count;

  out->x = mi.rcMonitor.left;  out->y = mi.rcMonitor.top;
  out->width  = mi.rcMonitor.right - mi.rcMonitor.left;
  out->height = mi.rcMonitor.bottom - mi.rcMonitor.top;

  out->work_x = mi.rcWork.left;  out->work_y = mi.rcWork.top;
  out->work_width = mi.rcWork.right - mi.rcWork.left;
  out->work_height= mi.rcWork.bottom - mi.rcWork.top;

  /* DPI */
  UINT dx=0, dy=0;
  PFN_GetDpiForMonitor p = s_GetDpiForMonitor();
  if (p)
  {
    if (SUCCEEDED(p(hMon, MDT_EFFECTIVE_DPI, &dx, &dy)))
    {
      out->dpi_x = (float)dx;
      out->dpi_y = (float)dy;
      out->scale_x = s_dpi_to_scale(dx);
      out->scale_y = s_dpi_to_scale(dy);
    }
  }
  if (out->dpi_x == 0.0f || out->dpi_y == 0.0f)
  {
    HDC sdc = GetDC(NULL);
    if (sdc)
    {
      int lpx = GetDeviceCaps(sdc, LOGPIXELSX);
      int lpy = GetDeviceCaps(sdc, LOGPIXELSY);
      out->dpi_x = (float)lpx; out->dpi_y = (float)lpy;
      out->scale_x = s_dpi_to_scale((UINT)lpx);
      out->scale_y = s_dpi_to_scale((UINT)lpy);
      ReleaseDC(NULL, sdc);
    }
    else
    {
      out->dpi_x = out->dpi_y = 96.0f;
      out->scale_x = out->scale_y = 1.0f;
    }
  }

  ctx->count++;
  return TRUE;
}

u32 x_plat_monitor_count(void)
{
  XMonitorInfo tmp[16]; s_MonEnumCtx ctx = { tmp, X_ARRAY_COUNT(tmp), 0, 0 };
  EnumDisplayMonitors(NULL, NULL, s_enum_mon_proc, (LPARAM)&ctx);
  return ctx.count;
}

XMonitorID x_plat_monitor_primary(void)
{
  XMonitorInfo tmp[16]; s_MonEnumCtx ctx = { tmp, X_ARRAY_COUNT(tmp), 0, 0 };
  EnumDisplayMonitors(NULL, NULL, s_enum_mon_proc, (LPARAM)&ctx);
  return ctx.primary_index < ctx.count ? (XMonitorID)ctx.primary_index : 0;
}

bool x_plat_monitor_info(XMonitorID id, XMonitorInfo* out)
{
  if (!out) return false;
  XMonitorInfo tmp[16]; s_MonEnumCtx ctx = { tmp, X_ARRAY_COUNT(tmp), 0, 0 };
  EnumDisplayMonitors(NULL, NULL, s_enum_mon_proc, (LPARAM)&ctx);
  if (id >= ctx.count) return false;
  *out = tmp[id];
  return true;
}

XMonitorID x_plat_window_monitor(XWindow win)
{
  HWND hwnd = s_win32_hwnd_from_xwindow(win);
  if (!hwnd) return 0;
  HMONITOR h = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

  XMonitorInfo tmp[16]; s_MonEnumCtx ctx = { tmp, X_ARRAY_COUNT(tmp), 0, 0 };
  EnumDisplayMonitors(NULL, NULL, s_enum_mon_proc, (LPARAM)&ctx);

  /* Find index of h */
  for (u32 i = 0; i < ctx.count; ++i)
  {
    /* crude match by rect */
    MONITORINFO mi; mi.cbSize = sizeof(mi);
    HMONITOR h2 = MonitorFromRect(&(RECT){ tmp[i].x, tmp[i].y, tmp[i].x+tmp[i].width, tmp[i].y+tmp[i].height }, MONITOR_DEFAULTTONULL);
    if (h == h2) return (XMonitorID)i;
  }
  return ctx.primary_index < ctx.count ? (XMonitorID)ctx.primary_index : 0;
}

void x_plat_window_content_scale_get(XWindow win, float* out_sx, float* out_sy)
{
  HWND hwnd = s_win32_hwnd_from_xwindow(win);
  float sx=1.0f, sy=1.0f;
  if (hwnd)
  {
    /* Prefer per-window DPI if available (Win10) */
    UINT dpi = 0;
    /* GetDpiForWindow exists on Win10; load dynamically to be safe */
    UINT (WINAPI *pGetDpiForWindow)(HWND) = (void*)GetProcAddress(GetModuleHandleA("user32.dll"), "GetDpiForWindow");
    if (pGetDpiForWindow)
    {
      dpi = pGetDpiForWindow(hwnd);
      sx = sy = s_dpi_to_scale(dpi);
    }
    else
    {
      /* Fall back to monitor DPI */
      HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
      PFN_GetDpiForMonitor p = s_GetDpiForMonitor();
      UINT dx=0, dy=0;
      if (p && SUCCEEDED(p(hMon, MDT_EFFECTIVE_DPI, &dx, &dy)))
      {
        sx = s_dpi_to_scale(dx); sy = s_dpi_to_scale(dy);
      }
      else
      {
        HDC dc = GetDC(NULL);
        if (dc)
        {
          int lpx = GetDeviceCaps(dc, LOGPIXELSX);
          int lpy = GetDeviceCaps(dc, LOGPIXELSY);
          sx = s_dpi_to_scale((UINT)lpx);
          sy = s_dpi_to_scale((UINT)lpy);
          ReleaseDC(NULL, dc);
        }
      }
    }
  }
  if (out_sx) *out_sx = sx;
  if (out_sy) *out_sy = sy;
}

