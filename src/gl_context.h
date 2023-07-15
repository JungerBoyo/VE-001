#ifndef VE001_GL_CONTEXT_H
#define VE001_GL_CONTEXT_H

#include "types.h"

namespace ve001 {

void glInit();
void setGLDebugCallback(void (*gl_error_callback)(u32, u32, u32, u32, i32, const char *, const void *));

}

#endif