#pragma once

// Compatibility alias for devkitARM's generated header name for dweller.v.pica.
#include "dweller_shbin.h"

#define dweller_v_shbin const_cast<unsigned char*>(dweller_shbin)
#define dweller_v_shbin_size dweller_shbin_size
