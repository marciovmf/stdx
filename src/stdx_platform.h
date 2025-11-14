#ifndef X_PLATFORM_H
#define X_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#define X_PLATFORM_VERSION_MAJOR 1
#define X_PLATFORM_VERSION_MINOR 0
#define X_PLATFORM_VERSION_PATCH 0

#define X_PLATFORM_VERSION (X_PLATFORM_VERSION_MAJOR * 10000 + X_PLATFORM_VERSION_MINOR * 100 + X_PLATFORM_VERSION_PATCH)

#ifndef X_PLATFORM_MAX_EVENTS
#define X_PLATFORM_MAX_EVENTS 64
#endif

#include <stdx_common.h>
#include <stdint.h>
#include <stdbool.h>

  typedef enum
  {
    X_PLATFORM_GRAPHICS_OPENGL_3_0 = 0,
    X_PLATFORM_GRAPHICS_OPENGL_3_3 = 1,
  } XPlatformFlags;

  /* Event System Types */

  typedef struct XWindow_t* XWindow;

  typedef enum
  {
    /* Event Types */
    X_EVENT_TYPE_NONE             = 0,
    X_EVENT_TYPE_GAME             = 1 << 0,
    X_EVENT_TYPE_WINDOW           = 1 << 1,
    X_EVENT_TYPE_TEXT             = 1 << 2,
    X_EVENT_TYPE_APPLICATION      = 1 << 3,
    X_EVENT_TYPE_KEYBOARD         = 1 << 4,
    X_EVENT_TYPE_MOUSE_MOVE       = 1 << 5,
    X_EVENT_TYPE_MOUSE_BUTTON     = 1 << 6,
    X_EVENT_TYPE_MOUSE_WHEEL      = 1 << 7,
    X_EVENT_TYPE_FRAME_BEFORE     = 1 << 8,
    X_EVENT_TYPE_FRAME_AFTER      = 1 << 9,
    X_EVENT_TYPE_ANY              = 0xFFFFFFFF,
  } XEventType;

  typedef enum
  {
    /* Keyboard Event types */
    X_KEYBOARD_EVENT_KEY_DOWN     = 1,
    X_KEYBOARD_EVENT_KEY_UP       = 2,
    X_KEYBOARD_EVENT_KEY_HOLD     = 3,
  } XKeyboardEventType;

  typedef enum
  {
    /* Text Event types */
    X_TEXT_EVENT_CHARACTER_INPUT  = 4,
    X_TEXT_EVENT_BACKSPACE        = 5,
    X_TEXT_EVENT_DEL              = 6,
  } XTextEventType;

  typedef enum
  {
    /* Mouse event types */
    X_MOUSE_EVENT_MOVE            = 7,
    X_MOUSE_EVENT_BUTTON_DOWN     = 8,
    X_MOUSE_EVENT_BUTTON_UP       = 9,
    X_MOUSE_EVENT_WHEEL_FORWARD   = 10,
    X_MOUSE_EVENT_WHEEL_BACKWARD  = 11,
  } XMouseEventType;

  typedef enum
  {
    /* Window event types */
    X_WINDOW_EVENT_RESIZED        = 14,
    X_WINDOW_EVENT_CLOSE          = 15,
    X_WINDOW_EVENT_ACTIVATE       = 16,
    X_WINDOW_EVENT_DEACTIVATE     = 17,
    X_WINDOW_EVENT_MINIMIZED      = 18,
    X_WINDOW_EVENT_MAXIMIZED      = 19,
    X_WINDOW_EVENT_CONTENT_SCALE_CHANGED = 20,
  } XWindowEventType;

  typedef struct
  {
    XTextEventType type;
    u32 character;
  } XTextEvent;

  typedef struct
  {
    XWindowEventType type;
    u32 width;
    u32 height;
  } XWindowEvent;

  typedef struct
  {
    XKeyboardEventType type;
    u32 keyCode; 
    bool ctrlIsDown;
    bool shiftIsDown;
    bool altIsDown;
  } XKeyboardEvent;

  typedef struct
  {
    XMouseEventType type;
    i32 cursorX;
    i32 cursorY;
    i32 xRel;
    i32 yRel;

    union
    {
      i32 wheelDelta;
      i32 mouseButton;
    };
  } XMouseEvent;

  typedef struct
  {
    u64 ticks;
    float deltaTime;
  } XFrameEvent;

  typedef struct
  {
    XWindow window;
    XEventType type;
    union
    {
      XTextEvent        textEvent;
      XWindowEvent      windowEvent;
      XKeyboardEvent    keyboardEvent;
      XMouseEvent       mouseEvent;
      XFrameEvent       frameEvent;
    };
  } XEvent;

  /* Input System types and defines*/

#define X_KEYBOARD_KEYCODES \
  X(X_KEYCODE_INVALID, "", 0x00) \
  X(X_KEYCODE_BACKSPACE, "BACK", 0x08) \
  X(X_KEYCODE_TAB, "TAB", 0x09) \
  X(X_KEYCODE_CLEAR, "CLEAR", 0x0C) \
  X(X_KEYCODE_RETURN, "RETURN", 0x0D) \
  X(X_KEYCODE_SHIFT, "SHIFT", 0x10) \
  X(X_KEYCODE_CONTROL, "CONTROL", 0x11) \
  X(X_KEYCODE_ALT, "ALT", 0x12) \
  X(X_KEYCODE_PAUSE, "PAUSE", 0x13) \
  X(X_KEYCODE_CAPITAL, "CAPITAL", 0x14) \
  X(X_KEYCODE_ESCAPE, "ESCAPE", 0x1B) \
  X(X_KEYCODE_CONVERT, "CONVERT", 0x1C) \
  X(X_KEYCODE_NONCONVERT, "NONCONVERT", 0x1D) \
  X(X_KEYCODE_ACCEPT, "ACCEPT", 0x1E) \
  X(X_KEYCODE_MODECHANGE, "MODECHANGE", 0x1F) \
  X(X_KEYCODE_SPACE, "SPACE", 0x20) \
  X(X_KEYCODE_PRIOR, "PRIOR", 0x21) \
  X(X_KEYCODE_NEXT, "NEXT", 0x22) \
  X(X_KEYCODE_END, "END", 0x23) \
  X(X_KEYCODE_HOME, "HOME", 0x24) \
  X(X_KEYCODE_LEFT, "LEFT", 0x25) \
  X(X_KEYCODE_UP, "UP", 0x26) \
  X(X_KEYCODE_RIGHT, "RIGHT", 0x27) \
  X(X_KEYCODE_DOWN, "DOWN", 0x28) \
  X(X_KEYCODE_SELECT, "SELECT", 0x29) \
  X(X_KEYCODE_PRINT, "PRINT", 0x2A) \
  X(X_KEYCODE_EXECUTE, "EXECUTE", 0x2B) \
  X(X_KEYCODE_SNAPSHOT, "SNAPSHOT", 0x2C) \
  X(X_KEYCODE_INSERT, "INSERT", 0x2D) \
  X(X_KEYCODE_DELETE, "DELETE", 0x2E) \
  X(X_KEYCODE_HELP, "HELP", 0x2F) \
  X(X_KEYCODE_0, "0", 0x30) \
  X(X_KEYCODE_1, "1", 0x31) \
  X(X_KEYCODE_2, "2", 0x32) \
  X(X_KEYCODE_3, "3", 0x33) \
  X(X_KEYCODE_4, "4", 0x34) \
  X(X_KEYCODE_5, "5", 0x35) \
  X(X_KEYCODE_6, "6", 0x36) \
  X(X_KEYCODE_7, "7", 0x37) \
  X(X_KEYCODE_8, "8", 0x38) \
  X(X_KEYCODE_9, "9", 0x39) \
  X(X_KEYCODE_A, "A", 0x41) \
  X(X_KEYCODE_B, "B", 0x42) \
  X(X_KEYCODE_C, "C", 0x43) \
  X(X_KEYCODE_D, "D", 0x44) \
  X(X_KEYCODE_E, "E", 0x45) \
  X(X_KEYCODE_F, "F", 0x46) \
  X(X_KEYCODE_G, "G", 0x47) \
  X(X_KEYCODE_H, "H", 0x48) \
  X(X_KEYCODE_I, "I", 0x49) \
  X(X_KEYCODE_J, "J", 0x4A) \
  X(X_KEYCODE_K, "K", 0x4B) \
  X(X_KEYCODE_L, "L", 0x4C) \
  X(X_KEYCODE_M, "M", 0x4D) \
  X(X_KEYCODE_N, "N", 0x4E) \
  X(X_KEYCODE_O, "O", 0x4F) \
  X(X_KEYCODE_P, "P", 0x50) \
  X(X_KEYCODE_Q, "Q", 0x51) \
  X(X_KEYCODE_R, "R", 0x52) \
  X(X_KEYCODE_S, "S", 0x53) \
  X(X_KEYCODE_T, "T", 0x54) \
  X(X_KEYCODE_U, "U", 0x55) \
  X(X_KEYCODE_V, "V", 0x56) \
  X(X_KEYCODE_W, "W", 0x57) \
  X(X_KEYCODE_X, "X", 0x58) \
  X(X_KEYCODE_Y, "Y", 0x59) \
  X(X_KEYCODE_Z, "Z", 0x5A) \
  X(X_KEYCODE_NUMPAD0, "NUMPAD0", 0x60) \
  X(X_KEYCODE_NUMPAD1, "NUMPAD1", 0x61) \
  X(X_KEYCODE_NUMPAD2, "NUMPAD2", 0x62) \
  X(X_KEYCODE_NUMPAD3, "NUMPAD3", 0x63) \
  X(X_KEYCODE_NUMPAD4, "NUMPAD4", 0x64) \
  X(X_KEYCODE_NUMPAD5, "NUMPAD5", 0x65) \
  X(X_KEYCODE_NUMPAD6, "NUMPAD6", 0x66) \
  X(X_KEYCODE_NUMPAD7, "NUMPAD7", 0x67) \
  X(X_KEYCODE_NUMPAD8, "NUMPAD8", 0x68) \
  X(X_KEYCODE_NUMPAD9, "NUMPAD9", 0x69) \
  X(X_KEYCODE_MULTIPLY, "MULTIPLY", 0x6A) \
  X(X_KEYCODE_ADD, "ADD", 0x6B) \
  X(X_KEYCODE_SEPARATOR, "SEPARATOR", 0x6C) \
  X(X_KEYCODE_SUBTRACT, "SUBTRACT", 0x6D) \
  X(X_KEYCODE_DECIMAL, "DECIMAL", 0x6E) \
  X(X_KEYCODE_DIVIDE, "DIVIDE", 0x6F) \
  X(X_KEYCODE_F1, "F1", 0x70) \
  X(X_KEYCODE_F2, "F2", 0x71) \
  X(X_KEYCODE_F3, "F3", 0x72) \
  X(X_KEYCODE_F4, "F4", 0x73) \
  X(X_KEYCODE_F5, "F5", 0x74) \
  X(X_KEYCODE_F6, "F6", 0x75) \
  X(X_KEYCODE_F7, "F7", 0x76) \
  X(X_KEYCODE_F8, "F8", 0x77) \
  X(X_KEYCODE_F9, "F9", 0x78) \
  X(X_KEYCODE_F10, "F10", 0x79) \
  X(X_KEYCODE_F11, "F11", 0x7A) \
  X(X_KEYCODE_F12, "F12", 0x7B) \
  X(X_KEYCODE_F13, "F13", 0x7C) \
  X(X_KEYCODE_F14, "F14", 0x7D) \
  X(X_KEYCODE_F15, "F15", 0x7E) \
  X(X_KEYCODE_F16, "F16", 0x7F) \
  X(X_KEYCODE_F17, "F17", 0x80) \
  X(X_KEYCODE_F18, "F18", 0x81) \
  X(X_KEYCODE_F19, "F19", 0x82) \
  X(X_KEYCODE_F20, "F20", 0x83) \
  X(X_KEYCODE_F21, "F21", 0x84) \
  X(X_KEYCODE_F22, "F22", 0x85) \
  X(X_KEYCODE_F23, "F23", 0x86) \
  X(X_KEYCODE_F24, "F24", 0x87) \
  X(X_KEYCODE_OEM1, "OEM1", 0xBA) \
  X(X_KEYCODE_OEM2, "OEM2", 0xBF) \
  X(X_KEYCODE_OEM3, "OEM3", 0xC0) \
  X(X_KEYCODE_OEM4, "OEM4", 0xDB) \
  X(X_KEYCODE_OEM5, "OEM5", 0xDC) \
  X(X_KEYCODE_OEM6, "OEM6", 0xDD) \
  X(X_KEYCODE_OEM7, "OEM7", 0xDE) \
  X(X_KEYCODE_OEM8, "OEM8", 0xDF) \
  X(X_NUM_KEYCODES, "", 0xE0)

#define X(keycode, name, value) keycode = value,

  typedef enum
  {
    X_KEYBOARD_KEYCODES
  } XKeycode;

  enum
  {
    X_KEYBOARD_PRESSED_BIT = 1,
    X_KEYBOARD_CHANGED_THIS_FRAME_BIT = 1 << 1,
    X_KEYBOARD_MAX_KEYS = 256
  };

  typedef struct 
  {
    unsigned char key[X_KEYBOARD_MAX_KEYS];
  }XKeyboardState;

#undef X

  typedef enum
  {
    X_MOUSE_BUTTON_LEFT     = 0,
    X_MOUSE_BUTTON_RIGHT    = 1,
    X_MOUSE_BUTTON_MIDDLE   = 2,
    X_MOUSE_BUTTON_EXTRA_0  = 3,
    X_MOUSE_BUTTON_EXTRA_1  = 4,
  } XMouseButton;

  enum {
    X_MOUSE_CHANGED_THIS_FRAME_BIT = 1 << 1,
    X_MOUSE_PRESSED_BIT     = 1,
    X_MOUSE_MAX_BUTTONS     = 5
  };

  typedef struct 
  {
    i32 wheelDelta;
    XPoint cursor;
    XPoint cursorRelative;
    unsigned char button[X_MOUSE_MAX_BUTTONS];
  } XMouseState;

#define X_JOYSTICK_BUTTONS \
  X(X_JOYSTICK_BUTTON_DPAD_UP,      "X_JOYSTICK_BUTTON_DPAD_UP",   0x00) \
  X(X_JOYSTICK_BUTTON_DPAD_DOWN,    "X_JOYSTICK_BUTTON_DPAD_DOWN", 0x01) \
  X(X_JOYSTICK_BUTTON_DPAD_LEFT,    "X_JOYSTICK_BUTTON_DPAD_LEFT", 0x02) \
  X(X_JOYSTICK_BUTTON_DPAD_RIGHT,   "X_JOYSTICK_BUTTON_DPAD_RIGHT",0x03) \
  X(X_JOYSTICK_BUTTON_START,        "X_JOYSTICK_BUTTON_START",     0x04) \
  X(X_JOYSTICK_BUTTON_FN1,          "X_JOYSTICK_BUTTON_FN1",       0x04) \
  X(X_JOYSTICK_BUTTON_BACK,         "X_JOYSTICK_BUTTON_BACK",      0x05) \
  X(X_JOYSTICK_BUTTON_FN2,          "X_JOYSTICK_BUTTON_FN2",       0x05) \
  X(X_JOYSTICK_BUTTON_LEFT_THUMB,   "X_JOYSTICK_BUTTON_LEFT_THUMB",    0x06) \
  X(X_JOYSTICK_BUTTON_RIGHT_THUMB,  "X_JOYSTICK_BUTTON_RIGHT_THUMB",    0x07) \
  X(X_JOYSTICK_BUTTON_LEFT_SHOULDER, "X_JOYSTICK_BUTTON_LEFT_SHOULDER", 0x08) \
  X(X_JOYSTICK_BUTTON_RIGHT_SHOULDER, "X_JOYSTICK_BUTTON_RIGHT_SHOULDER", 0x09) \
  X(X_JOYSTICK_BUTTON_A,            "X_JOYSTICK_BUTTON_A", 0x0A) \
  X(X_JOYSTICK_BUTTON_B,            "X_JOYSTICK_BUTTON_B", 0x0B) \
  X(X_JOYSTICK_BUTTON_X,            "X_JOYSTICK_BUTTON_X", 0x0C) \
  X(X_JOYSTICK_BUTTON_Y,            "X_JOYSTICK_BUTTON_Y", 0x0D) \
  X(X_JOYSTICK_BUTTON_BTN1,         "X_JOYSTICK_BUTTON1", 0x0A) \
  X(X_JOYSTICK_BUTTON_BTN2,         "X_JOYSTICK_BUTTON2", 0x0B) \
  X(X_JOYSTICK_BUTTON_BTN3,         "X_JOYSTICK_BUTTON3", 0x0C) \
  X(X_JOYSTICK_BUTTON_BTN4,         "X_JOYSTICK_BUTTON4", 0x0D)

#define X(keycode, name, value) keycode = value,
  typedef enum
  {
    X_JOYSTICK_CHANGED_THIS_FRAME_BIT = 1 << 1,
    X_JOYSTICK_PRESSED_BIT     = 1,
    X_JOYSTICK_NUM_BUTTONS     = 14, // NOTICE that some entry values are repeated
    X_JOYSTICK_BUTTONS // expand X macro ...
  } XJoystickButton;
#undef X

#define X_JOYSTICK_AXIS \
  X(X_JOYSTICK_AXIS_LX, "X_JOYSTICK_AXIS_LX", 0x00) \
  X(X_JOYSTICK_AXIS_LY, "X_JOYSTICK_AXIS_LY", 0x01) \
  X(X_JOYSTICK_AXIS_RX, "X_JOYSTICK_AXIS_RX", 0x02) \
  X(X_JOYSTICK_AXIS_RY, "X_JOYSTICK_AXIS_RY", 0x03) \
  X(X_JOYSTICK_AXIS_LTRIGGER, "X_JOYSTICK_AXIS_LTRIGGER", 0x04) \
  X(X_JOYSTICK_AXIS_RTRIGGER, "X_JOYSTICK_AXIS_RTRIGGER", 0x05) \

#define X_JOYSTICK_MAX 4

  typedef enum
  {
    X_JOYSTICK_0,
    X_JOYSTICK_1,
    X_JOYSTICK_2,
    X_JOYSTICK_3,
  } XJoystickID;

#define X(keycode, name, value) keycode = value,
  typedef enum
  {
    X_JOYSTICK_NUM_AXIS = 6,
    X_JOYSTICK_AXIS
  } XJoystickAxis;
#undef X

  typedef struct
  {
    u32 button[X_JOYSTICK_NUM_BUTTONS];
    float axis[X_JOYSTICK_NUM_AXIS];
    bool connected;
    float vibrationLeft;
    float vibrationRight;
  } XJoystickState;

  /*** Initialization ***/
  bool   x_plat_init(XPlatformFlags flags, void* user_ctx);   // or just void if not needed
  void   x_plat_terminate(void);

  /*** Windowing ***/
  typedef enum
  {
    X_WINDOW_STATE_NORMAL              = 1 << 0,
    X_WINDOW_STATE_MAXIMIZED           = 1 << 1,
    X_WINDOW_STATE_MINIMIZED           = 1 << 2,
    X_WINDOW_STATE_NORESIZE            = 1 << 3,
  } XWindowStateFlags;

  XWindow x_plat_window_create(const char* title, i32 width, i32 height, XWindowStateFlags flags);
  void    x_plat_window_destroy(XWindow win);
  XSize   x_plat_window_size_get(XWindow win);
  void    x_plat_window_size_set(XWindow win, i32 width, i32 height);
  XSize   x_plat_window_client_size_get(XWindow win);
  void    x_plat_window_client_size_set(XWindow win, i32 width, i32 height);
  XPoint  x_plat_window_position_get(XWindow win);
  void    x_plat_window_position_set(XWindow win, i32 x, i32 y);
  bool    x_plat_window_fullscreen_get(XWindow win);
  bool    x_plat_window_fullscreen_set(XWindow win, bool enabled);
  bool    x_plat_window_icon_set(XWindow win, const char* icon_path);
  bool    x_plat_window_close_flag_get(XWindow win);
  void    x_plat_window_close_flag_set(XWindow win);
  void    x_plat_window_close_flag_clear(XWindow win);
  void    x_plat_window_swap_buffers(XWindow win);
  size_t  x_plat_window_title_len(XWindow win);
  size_t  x_plat_window_title_get(XWindow win, char* out_buf, size_t buf_cap);
  void    x_plat_window_title_set(XWindow win, const char* title);

  /*** Events ***/
  void x_plat_events_wait(i32 timeout_ms);
  bool x_plat_event_poll(XEvent* out_event);  // returns false when no event

  /*** Keyboard input ***/
  void x_plat_keyboard_state_get(XKeyboardState* outState);
  bool x_plat_keyboard_key_is_pressed(const XKeyboardState* state, XKeycode keycode); // True while the key is pressed
  bool x_plat_keyboard_key_down(const XKeyboardState* state, XKeycode keycode); // True during the frame the key was pressed
  bool x_plat_keyboard_key_up(const XKeyboardState* state, XKeycode keycode); // True during the frame the key was released

  /*** Mouse input ***/
  void x_plat_mouse_state_get(XMouseState* out_state);
  bool x_plat_mouse_button_is_pressed(const XMouseState* state, XMouseButton button);
  bool x_plat_mouse_button_down(const XMouseState* state, XMouseButton button);
  bool x_plat_mouse_button_up(const XMouseState* state, XMouseButton button);
  XPoint x_plat_mouse_cursor(const XMouseState* state);
  XPoint x_plat_mouse_cursor_relative(const XMouseState* state);
  void x_plat_window_mouse_capture_set(XWindow win, bool enabled);
  bool x_plat_window_mouse_capture_get(const XWindow win);
  i32 x_plat_mouse_wheel_delta(const XMouseState* state);

  /*** Joystick ***/
  void x_plat_joystick_state_get(XJoystickState* out_state, XJoystickID id);
  bool x_plat_joystick_button_is_pressed(const XJoystickState* state, XJoystickButton key);
  bool x_plat_joystick_button_down(const XJoystickState* state, XJoystickButton key);
  bool x_plat_joystick_button_up(const XJoystickState* state, XJoystickButton key);
  float x_plat_joystick_axis_get(const XJoystickState* state, XJoystickAxis axis);
  u32 x_plat_joystick_count(void);
  u32 x_plat_joystick_is_connected(const XJoystickID id);
  void x_plat_joystick_vibration_left_set(XJoystickID id, float speed);
  void x_plat_joystick_vibration_right_set(XJoystickID id, float speed);
  float x_plat_joystick_vibration_left_get(const XJoystickID id);
  float x_plat_joystick_vibration_right_get(const XJoystickID id);

  /*** Clipboard (UTF-8 text only) ***/
  size_t x_plat_clipboard_text_len(void); // Returns number of bytes (excluding NUL) currently on the clipboard as UTF-8.
  bool   x_plat_clipboard_text_set(const char* text, size_t len); // Copies UTF-8 text to the clipboard. If len==0, treats 'text' as NUL-terminated.
  size_t x_plat_clipboard_text_get(char* out, size_t cap); // Reads UTF-8 clipboard text into 'out' (NUL-terminated if cap>0). Returns bytes written (excluding NUL). If out==NULL, returns required size.

  /*** Cursor control ***/
  typedef enum
  {
    X_CURSOR_DEFAULT = 0,   /* Arrow */
    X_CURSOR_TEXT,          /* I-beam */
    X_CURSOR_HAND,          /* Link hand */
    X_CURSOR_CROSSHAIR,
    X_CURSOR_RESIZE_NS,
    X_CURSOR_RESIZE_EW,
    X_CURSOR_RESIZE_NESW,
    X_CURSOR_RESIZE_NWSE,
    X_CURSOR_RESIZE_ALL,    /* Move/size all */
    X_CURSOR_NOT_ALLOWED,
  } XCursorShape;

  /* Show/Hide cursor globally while this app is active. */
  void x_plat_cursor_visible_set(bool visible);
  bool x_plat_cursor_visible_get(void);

  /* Set a standard cursor shape. */
  void x_plat_cursor_shape_set(XCursorShape shape);

  /* Confine the cursor to the client area of 'win' (true) or release (false). */
  void x_plat_cursor_confine_set(XWindow win, bool enabled);
  bool x_plat_cursor_confine_get(XWindow win);

  /* Relative mode = hide + confine + deliver motion as deltas (no OS acceleration).
     Useful for FPS camera. */
  bool x_plat_cursor_relative_mode_set(XWindow win, bool enabled);
  bool x_plat_cursor_relative_mode_get(XWindow win);


  /*** Monitors / DPI / Content Scale ***/

  typedef u32 XMonitorID;

  typedef struct
  {
    char  name[64];
    bool  primary;
    /* Pixel-space bounds */
    i32   x;
    i32   y;
    i32   width;
    i32   height;

    /* Work area in pixels (taskbar excluded) */
    i32   work_x;
    i32   work_y;
    i32   work_width;
    i32   work_height;

    /* DPI and scale (physical DPI may be unavailable on some OSes) */
    float dpi_x;
    float dpi_y;

    /* Relative to 1.0 */
    float scale_x;
    float scale_y;
  } XMonitorInfo;

  u32  x_plat_monitor_count(void); // How many monitors are present.
  bool x_plat_monitor_info(XMonitorID id, XMonitorInfo* out); // Fill info for monitor 'id'. Returns false if 'id' is invalid
  XMonitorID x_plat_monitor_primary(void); // Returns an ID in range [0, count). Stable for the lifetime of the process.
  XMonitorID x_plat_window_monitor(XWindow win); // Get the monitor hosting (most of) this window.

  /*** Window content scale / DPI change events ***/
  void x_plat_window_content_scale_get(XWindow win, float* out_sx, float* out_sy); // Returns the current content scale applied to 'win' (1.0 = 96 DPI baseline).

  /*** Utility functions ***/
  void x_plat_msg_box(const char* title, const char* msg);
  bool x_plat_msg_box_yes_no(const char* title, const char* msg);

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_PLATFORM

#if defined(X_OS_WINDOWS)
#include "stdx_platform_win32.inl"
#else
#error "Unsupported platform"
#endif

#endif // X_IMPL_PLATFORM      
#endif // X_PLATFORM_H
