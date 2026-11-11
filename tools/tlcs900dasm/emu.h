// Minimal stand-in for MAME's emu.h, just enough to compile dasm900.cpp outside MAME.
#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstddef>
#include <string>
#include <ostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <type_traits>

typedef uint8_t u8;   typedef int8_t s8;
typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32;
typedef uint64_t u64; typedef int64_t s64;
typedef u32 offs_t;

template <typename T> constexpr T BIT(T x, int n) { return (x >> n) & T(1); }

namespace util {
namespace detail {
inline const char *fmt_arg(const std::string &s) { return s.c_str(); }
inline const char *fmt_arg(const char *s) { return s; }
inline const char *fmt_arg(char *s) { return s; }
inline double fmt_arg(double v) { return v; }
template <typename T>
typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value, long long>::type
fmt_arg(T v) { return (long long)v; }
// printf with every integer widened to long long: rewrite conversions to use the ll modifier.
inline std::string widen(const char *fmt) {
	std::string out;
	for (const char *p = fmt; *p; ++p) {
		out += *p;
		if (*p != '%') continue;
		if (p[1] == '%') { out += *++p; continue; }
		++p;
		while (*p && std::strchr("-+ #0123456789*.", *p)) out += *p++;
		while (*p && std::strchr("hlLqjzt", *p)) ++p;           // drop existing length modifiers
		if (*p && std::strchr("diouxX", *p)) out += "ll";
		if (*p) out += *p; else break;
	}
	return out;
}
}
template <typename... Args>
std::string string_format(const char *fmt, Args &&... args) {
	std::string f = detail::widen(fmt);
	char buf[512];
	std::snprintf(buf, sizeof buf, f.c_str(), detail::fmt_arg(args)...);
	return buf;
}
template <typename... Args>
void stream_format(std::ostream &os, const char *fmt, Args &&... args) { os << string_format(fmt, std::forward<Args>(args)...); }

class disasm_interface {
public:
	enum : u32 {
		LENGTHMASK = 0x0000ffff, STEP_COND = 0x10000000, STEP_OVER = 0x20000000,
		STEP_OUT = 0x40000000, SUPPORTED = 0x80000000
	};
	class data_buffer {
	public:
		data_buffer(const std::vector<u8> &d, offs_t base) : m_d(d), m_base(base) {}
		u8 r8(offs_t a) const { a -= m_base; return a < m_d.size() ? m_d[a] : 0xff; }
	private:
		const std::vector<u8> &m_d; offs_t m_base;
	};
	virtual ~disasm_interface() = default;
	virtual u32 opcode_alignment() const = 0;
	virtual offs_t disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params) = 0;
};
}
using util::string_format;
