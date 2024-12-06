#if defined(_VC_NODEFAULTLIB)

#include <intrin.h>
#include <string.h>

#pragma function(memset)

extern "C" void* __cdecl memset(void* dst, int ch, size_t count)
{
	auto d = (unsigned char*)dst;
#if defined(_M_IX86) || defined(_M_AMD64)
	__stosb(d, ch, count);
#else
	for (auto n = count; n; n--) { *d++ = ch; }
#endif
	return dst;
}

#pragma function(memcpy)

extern "C" void* __cdecl memcpy(void* dst, const void* src, size_t count)
{
	auto d = (unsigned char*)dst;
	auto s = (const unsigned char*)src;
#if defined(_M_IX86) || defined(_M_AMD64)
	__movsb(d, s, count);
#else
	for (auto n = count; n; n--) { *d++ = *s++; }
#endif
	return dst;
}

#endif
