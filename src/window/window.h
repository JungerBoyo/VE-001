#ifndef VE001_WINDOW_H
#define VE001_WINDOW_H

#include <tuple>
#include <string_view>
#include "types.h"

namespace ve001 {

struct Window {
	struct WinNativeData;

private:
	WinNativeData* _win_native_data;

public:
	Window(std::string_view title, i32 w, i32 h, void (*win_error_callback)(i32, const char *));

	[[nodiscard]] void* native();
	[[nodiscard]] std::pair<i32, i32> size() const;
	[[nodiscard]] f32 time() const;

	void setViewport(i32 w, i32 h) const;
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

}

#endif