#pragma once

#ifdef _DEBUG
#define IS_DEBUG 1
#else
#define IS_DEBUG 0
#endif

#define noxnd noexcept(!IS_DEBUG)
