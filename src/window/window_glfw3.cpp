#include "window.h"

#include <stdexcept>

#include <GLFW/glfw3.h>

using namespace ve001;

struct Window::WinNativeData {
	GLFWwindow *win_handle{nullptr};
};

Window::Window(std::string_view title, i32 w, i32 h, void (*win_error_callback)(i32, const char *)) {
	/// GLFW INIT
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("glfw init failed");
	}

	// TODO: wykrywanie wersji
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	_win_native_data = new WinNativeData{};

	_win_native_data->win_handle = glfwCreateWindow(w, h, title.data(), nullptr, nullptr);
	if (_win_native_data->win_handle == nullptr) {
		throw std::runtime_error("window creation failed");
	}
	glfwMakeContextCurrent(_win_native_data->win_handle);

	if (win_error_callback != nullptr) {
		glfwSetErrorCallback(win_error_callback);
	}

	glfwSetInputMode(_win_native_data->win_handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void* Window::native() {
	return static_cast<void*>(_win_native_data->win_handle);
}
f32 Window::time() const {
	return static_cast<f32>(glfwGetTime());
}
std::pair<int, int> Window::size() const {
	i32 w{0};
	i32 h{0};
	glfwGetWindowSize(_win_native_data->win_handle, &w, &h);
	return {w, h};
}
void Window::setWinUserDataPointer(void* ptr){
	glfwSetWindowUserPointer(_win_native_data->win_handle, ptr);
}
template<>
void Window::setKeyCallback<void, GLFWwindow*, int, int, int, int>
(void(*key_callback)(GLFWwindow*, int, int, int, int)) {
	glfwSetKeyCallback(_win_native_data->win_handle, key_callback);
}
template<>
void Window::setMousePositionCallback<void, GLFWwindow*, f64, f64>
(void(*mouse_position_callback)(GLFWwindow*, f64, f64)) {
	glfwSetCursorPosCallback(_win_native_data->win_handle, mouse_position_callback);
}

void Window::swapBuffers() const {
	glfwSwapBuffers(_win_native_data->win_handle);
}
bool Window::shouldClose() const {
	return glfwWindowShouldClose(_win_native_data->win_handle) != 0;
}
void Window::pollEvents() const {
	glfwPollEvents();
}

void Window::deinit() {
	glfwDestroyWindow(_win_native_data->win_handle);
	delete _win_native_data;	
	glfwTerminate();
}