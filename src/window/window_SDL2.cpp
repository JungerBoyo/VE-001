#include "window.h"

#include <stdexcept>

#include <SDL2/SDL.h>

using namespace ve001;

struct Window::WinNativeData {
	SDL_Window *win_handle{ nullptr };
	SDL_GLContext context{ nullptr };
	void (*win_error_callback)(i32, const char*) = nullptr;
	bool quit{ false };

	void(*mouse_position_callback)(void*, i32, i32, i32, i32, u32) = nullptr;
	void(*key_callback)(void*, u8, bool, SDL_Keysym) = nullptr;

	void* user_data_ptr{ nullptr };
};

Window::Window(std::string_view title, i32 w, i32 h, void (*win_error_callback)(i32, const char *)) {
	/// GLFW INIT
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		throw std::runtime_error("[sdl] init failed");
	}

	if (SDL_GL_LoadLibrary(nullptr) != 0) {
		throw std::runtime_error("[sdl] gl lib load failed");
	}

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);;

	SDL_ShowCursor(SDL_DISABLE);

	_win_native_data = new WinNativeData{};

	_win_native_data->win_handle = SDL_CreateWindow(title.data(), 0, 0, w, h, SDL_WINDOW_OPENGL);
	if (_win_native_data->win_handle == nullptr) {
		throw std::runtime_error("[sdl] window creation failed");
	}

	_win_native_data->context = SDL_GL_CreateContext(_win_native_data->win_handle);
	if (_win_native_data->context == nullptr) {
		throw std::runtime_error("[sdl] GL context creation failed");
	}

	SDL_GL_SetSwapInterval(1);

	_win_native_data->win_error_callback = win_error_callback;
}

void* Window::native() {
	return static_cast<void*>(_win_native_data->win_handle);
}
f32 Window::time() const {
	return static_cast<f32>(SDL_GetTicks());
}
std::pair<int, int> Window::size() const {
	i32 w{0};
	i32 h{0};
	SDL_GetWindowSize(_win_native_data->win_handle, &w, &h);
	return {w, h};
}
void Window::setWinUserDataPointer(void* ptr) {
	_win_native_data->user_data_ptr = ptr;
}

template<>
void Window::setMousePositionCallback<void, void*, i32, i32, i32, i32, u32>
(void(*mouse_position_callback)(void*, i32, i32, i32, i32, u32)) {
	_win_native_data->mouse_position_callback = mouse_position_callback;
}

template<>
void Window::setKeyCallback<void, void*, u8, bool, SDL_Keysym>
(void(*key_callback)(void*, u8, bool, SDL_Keysym)) {
	_win_native_data->key_callback = key_callback;
}

void Window::swapBuffers() const {
	SDL_GL_SwapWindow(_win_native_data->win_handle);
}
bool Window::shouldClose() const {
	return _win_native_data->quit;
}
void Window::pollEvents() const {
	static SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT: _win_native_data->quit = true; break;
		case SDL_MOUSEMOTION: {
			if (_win_native_data->mouse_position_callback != nullptr) {
				_win_native_data->mouse_position_callback(
					_win_native_data->user_data_ptr,
					event.motion.x,
					event.motion.y,
					event.motion.xrel,
					event.motion.yrel,
					event.motion.state
				);
			}
			break;
		}
		case SDL_KEYUP: case SDL_KEYDOWN: {
			if (_win_native_data->key_callback != nullptr) {
				_win_native_data->key_callback(
					_win_native_data->user_data_ptr,
					event.key.state,
					event.key.repeat > 0,
					event.key.keysym		
				);
			}
		}
		}
    }
}

void Window::deinit() {
	SDL_DestroyWindow(_win_native_data->win_handle);
	SDL_GL_DeleteContext(_win_native_data->context);

	delete _win_native_data;	

	SDL_Quit();
}