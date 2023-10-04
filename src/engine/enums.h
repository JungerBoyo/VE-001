#ifndef VE001_ENUMS_H
#define VE001_ENUMS_H

#include <vmath/vmath_types.h>

namespace ve001 {

enum Face : vmath::u32 {
    X_POS, X_NEG, 
    Y_POS, Y_NEG, 
    Z_POS, Z_NEG
};

}


#endif