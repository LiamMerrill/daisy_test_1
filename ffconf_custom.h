#ifndef FFCONF_CUSTOM_H
#define FFCONF_CUSTOM_H

#define FF_USE_LFN 2
#define FF_MAX_LFN 255

// Stub for ff_convert if not using code page conversion
#define ff_convert(src, dir) (src)
#define ff_wtoupper(chr) (chr)

#endif