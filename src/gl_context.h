#ifndef VE001_GL_CONTEXT_H
#define VE001_GL_CONTEXT_H

namespace ve001 {

void glInit();

#ifndef DEBUG
    void setGLDebugCallback(
        void (*gl_error_callback)(
            unsigned int, 
            unsigned int, 
            unsigned int, 
            unsigned int, 
            int, 
            const char *, 
            const void *)
        );
#endif

}

#endif