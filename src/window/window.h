#ifndef VE001_WINDOW_H
#define VE001_WINDOW_H

#include <tuple>
#include <string_view>
#include <vmath/vmath_types.h>

namespace ve001 {

struct Window {
	struct WinNativeData;

private:
	WinNativeData* _win_native_data{ nullptr };

public:
	static bool init(std::string_view title, vmath::i32 w, vmath::i32 h, void (*win_error_callback)(vmath::i32, const char *));

	Window() = default;

	[[nodiscard]] void* native();
	[[nodiscard]] std::pair<vmath::i32, vmath::i32> size() const;
	[[nodiscard]] vmath::f32 time() const;

	void setViewport(vmath::i32 w, vmath::i32 h) const;
	void setWinUserDataPointer(void* ptr);

	template<typename R, typename ...Args>
	void setKeyCallback(R(*key_callback)(Args...));

	template<typename R, typename ...Args>
	void setMousePositionCallback(R(*mouse_position_callback)(Args...));

	void swapBuffers() const;
	bool shouldClose() const;
	void pollEvents() const;

	void deinit();
};

extern Window window;

}

#endif