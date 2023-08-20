#include "cubemap.h"
#include "enums.h"

#include <stb_image.h>

#include <cstring>

using namespace ve001;
using namespace vmath;

/*
Assumes cube map layout ::
 ┌────────┬───────┬─────────────────┐
 │        │       │                 │
 │        │   Y+  │                 │
 │        │       │                 │
 │        │       │                 │
 ├────────┼───────┼────────┬────────┤
 │        │       │        │        │
 │   x-   │   Z+  │   x+   │   Z-   │ 
 │        │       │        │        │
 │        │       │        │        │
 ├────────┼───────┼────────┴────────┤
 │        │       │                 │
 │        │   Y-  │                 │
 │        │       │                 │
 │        │       │                 │
 └────────┴───────┴─────────────────┘
*/
Error CubeMap::load(CubeMap& self, const std::filesystem::path& img_path) {
    static constexpr i32 desired_channels_num{ 4 };

    i32 w, h, ch_num;
    void* ptr = stbi_load(
        img_path.generic_string().c_str(), // on windows c_str() returns wchar_t
        &w, &h, &ch_num, desired_channels_num
    );

    if (ptr == nullptr) {
        return Error::FAILED_TO_LOAD_IMAGE;
    }

    const i32 stride{ w * ch_num };

    const i32 face_stride{ stride/4 };
    const i32 face_width{ face_stride/ch_num };
    const i32 face_height{ h/3 };

    std::array<u8*, 6> face_ptrs{{
        static_cast<u8*>(ptr) + face_stride,                            // pos y
        static_cast<u8*>(ptr) + face_height * stride,                   // neg x
        static_cast<u8*>(ptr) + face_height * stride + face_stride,     // pos z
        static_cast<u8*>(ptr) + face_height * stride + 2*face_stride,   // pos x
        static_cast<u8*>(ptr) + face_height * stride + 3*face_stride,   // neg z
        static_cast<u8*>(ptr) + 2 * face_height * stride + face_stride, // neg y
    }};

    std::array<std::size_t, 6> translate_index{{
        Face::Y_POS, Face::X_NEG, Face::Z_POS, Face::X_POS, Face::Z_NEG, Face::Y_NEG
    }};

    std::size_t i{ 0U };
    for (auto* face_ptr : face_ptrs) {
        auto& face = self.faces[translate_index[i]];

        face.resize(face_stride * face_height);

        // reverse the image
        auto* tmp_face_ptr = face_ptr + (face_height - 1) * stride;
        auto* tmp_new_face_ptr = face.data();
        for (i32 row{ 0 }; row < face_height; ++row) {
            std::memcpy(
                static_cast<void*>(tmp_new_face_ptr), 
                static_cast<void*>(tmp_face_ptr), 
                face_stride
            );
            tmp_face_ptr -= stride;
            tmp_new_face_ptr += face_stride;
        }
        ++i;
    }

    self.height = face_height;
    self.width = face_width;
    self.channels_num = desired_channels_num;

    stbi_image_free(ptr);


    return Error::NO_ERROR;
}
