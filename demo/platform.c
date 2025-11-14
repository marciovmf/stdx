
#include <stdx_opengl.h>
#define X_IMPL_OPENGL
#include <stdx_opengl.h>
#define X_IMPL_PLATFORM
#include <stdx_platform.h>




void toggle_fullscreen(XWindow window)
{
  bool fs = x_plat_window_fullscreen_get(window);
  printf("fullscreen is %d\n", fs);
  x_plat_window_fullscreen_set(window, !fs);
}

int main()
{
  if (! x_plat_init(X_PLATFORM_GRAPHICS_OPENGL_3_0, NULL))
    return 1;
  XWindow window = x_plat_window_create("Hello, Platform!", 800, 600, X_WINDOW_STATE_NORMAL);
  x_plat_window_icon_set(window, "banana.ico");

  u32 monitor_count = x_plat_monitor_count();
  XMonitorID window_monitor = x_plat_window_monitor(window);
  XMonitorInfo monitor_info = {0};
  x_plat_monitor_info(window_monitor, &monitor_info);
  float scale_x, scale_y;
  x_plat_window_content_scale_get(window, &scale_x, &scale_y);
  printf("DPI_SCALE %f x %f\n", scale_x, scale_y);

  printf("monitor_count = %d\n"
      "current_monitor = %d\n"
      "monitor_name  = %s\n"
      "is_primary    = %d\n"
      "width         = %d\n"
      "height        = %d\n"
      "dpi_x         = %f\n"
      "dpi_y         = %f\n"
      "scale_x       = %f\n"
      "scale_y       = %f\n",
      monitor_count,
      window_monitor,
      monitor_info.name,
      monitor_info.primary,
      monitor_info.width,
      monitor_info.height,
      monitor_info.dpi_x,
      monitor_info.dpi_y,
      monitor_info.scale_x,
      monitor_info.scale_y);

  size_t clipboard_len = x_plat_clipboard_text_len();
  char clipboard[1024] = {0};
  x_plat_clipboard_text_get(clipboard, X_ARRAY_COUNT(clipboard)-1);
  printf("Clipboard contents (%zu) = '%s'\n", clipboard_len, clipboard);
  x_plat_window_mouse_capture_set(window, true);

  while(true)
  {
    x_plat_events_wait(-1);

    XEvent e;
    while (x_plat_event_poll(&e))
    {
      if (e.type == X_EVENT_TYPE_KEYBOARD)
      {
        if (e.keyboardEvent.type == X_KEYBOARD_EVENT_KEY_DOWN)
        {
          printf("Key down = %d\n", e.keyboardEvent.keyCode);
          if (e.keyboardEvent.keyCode == X_KEYCODE_F11)
            toggle_fullscreen(window);
        }
        else if (e.keyboardEvent.type == X_KEYBOARD_EVENT_KEY_UP)
          printf("Key up = %d\n", e.keyboardEvent.keyCode);
      }
      else if (e.type == X_EVENT_TYPE_MOUSE_MOVE)
      {
        if (e.mouseEvent.type == X_MOUSE_EVENT_MOVE)
          printf("Mouse move = %d,%d\n", e.mouseEvent.cursorX, e.mouseEvent.cursorY);
      }
    }

    if (x_plat_window_close_flag_get(window))
    {
      if (x_plat_msg_box_yes_no("Close window ?", "Are you sure you want to close ?"))
        break;
      else
        x_plat_window_close_flag_clear(window);
    }
  }
  x_plat_window_destroy(window);
  x_plat_terminate();
  return 0;
}
