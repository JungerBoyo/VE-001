#include <types.h>
#include <SDL2/SDL_keyboard.h>

using namespace ve001;

template<>
void Window::setKeyCallback
(void(*key_callback)(void* user_data_ptr, u8 keystate, bool repeat, SDL_Keysym keysym));

template<>
void Window::setMousePositionCallback
(void(*mouse_position_callback)(void* user_data_ptr, i32 x, i32 y, i32 xrel, i32 yrel, u32 buttonstate));