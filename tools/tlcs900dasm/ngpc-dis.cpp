// ngpc-dis: recursive-descent TLCS-900/H disassembler for Neo Geo Pocket (Color) cartridges.
//
// Uses MAME's tlcs900_disassembler (vendor/) for instruction decoding and adds:
//   * flow tracing from the header entry point and extra --seed addresses,
//   * interrupt vectors installed via `ld (0x6fb8..0x6ffc),XRR` after `ld XRR,imm32`,
//   * jump tables reached via `ld XRR,table` / `ld XRR,(XRR+r)` / `jp|call T,XRR`,
//   * pointers loaded from ROM via `ld XRR,(rom_addr)` / `jp|call T,XRR`,
//   * a listing with labels, code/data separation and ASCII annotations,
//   * a symbol file (address, kind, label) for later tooling.
//
//   * runtime coverage (--cov file: one executed hex address per line, e.g. from ngpc-trace)
//     used as unlabelled seeds, so code reached only via RAM function pointers is decoded.
//
// Usage: ngpc-dis [--seed hex]... [--cov file]... [--sym out.sym] rom.ngc > out.asm

#include "emu.h"
#include "dasm900.h"
#include "tmp95c061_syms.inc"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <deque>

namespace {

constexpr offs_t ROM_BASE = 0x200000;

struct Insn { u8 len; u32 flags; std::string text; };

enum Kind : u8 { K_NONE, K_LOC, K_SUB, K_VEC, K_JT, K_SEED, K_ENTRY };
const char *kind_prefix[] = { "", "loc", "sub", "vec", "jt", "seed", "entry" };
const char *kind_name[]   = { "", "jump", "call", "vector", "jumptable", "seed", "entry" };

struct Ctx {
	std::vector<u8> rom;
	offs_t used = 0;                      // bytes before trailing 0xFF padding
	tlcs900_disassembler dis{tmp95c061_syms, tmp95c061_cr_syms};
	std::map<offs_t, Insn> code;          // decoded instruction starts
	std::vector<u8> owner;                // 0 = data, 1 = instruction start, 2 = instruction tail
	std::map<offs_t, Kind> labels;
	std::map<offs_t, std::set<offs_t>> xrefs;
	std::map<offs_t, offs_t> jt_ranges;   // table start -> table end (exclusive)
	std::deque<offs_t> work;
	std::vector<std::string> notes;       // conflicts / unresolved flow
	unsigned indirect_unresolved = 0;

	bool in_rom(offs_t a) const { return a >= ROM_BASE && a < ROM_BASE + used; }
	u32 rd32(offs_t a) const {
		a -= ROM_BASE;
		return rom[a] | rom[a + 1] << 8 | rom[a + 2] << 16 | u32(rom[a + 3]) << 24;
	}
	void add_label(offs_t a, Kind k, offs_t from = 0) {
		auto &cur = labels[a];
		if (k > cur) cur = k;              // strongest kind wins (entry > seed > jt > vec > sub > loc)
		if (from) xrefs[a].insert(from);
	}
	void push(offs_t a, Kind k, offs_t from = 0) {
		if (!in_rom(a)) return;
		if (k != K_NONE) add_label(a, k, from);
		work.push_back(a);
	}
	Insn decode(offs_t pc) {
		util::disasm_interface::data_buffer b(rom, ROM_BASE);
		std::ostringstream o;
		offs_t r = dis.disassemble(o, pc, b, b);
		return { u8(r & 0xffff), r & 0xf0000000, o.str() };
	}
};

std::vector<std::string> operands(const std::string &text, std::string &mnem) {
	std::vector<std::string> ops;
	auto sp = text.find(' ');
	mnem = text.substr(0, sp);
	if (sp == std::string::npos) return ops;
	std::string rest = text.substr(sp + 1), cur;
	int depth = 0;
	for (char c : rest) {
		if (c == '(') depth++;
		if (c == ')') depth--;
		if (c == ',' && !depth) { ops.push_back(cur); cur.clear(); } else cur += c;
	}
	ops.push_back(cur);
	return ops;
}

bool parse_hex(const std::string &s, offs_t &v) {
	if (s.rfind("0x", 0) != 0) return false;
	char *end; v = strtoul(s.c_str() + 2, &end, 16);
	return *end == 0;
}

bool is_reg32(const std::string &s) {
	static const std::set<std::string> r = {"XWA","XBC","XDE","XHL","XIX","XIY","XIZ","XSP"};
	return r.count(s) != 0;
}

// Cheap sanity check for a candidate code address: linear decode must reach a return or an
// unconditional jump within 24 instructions without hitting an undefined opcode or leaving ROM.
bool plausible_code(Ctx &c, offs_t pc) {
	for (int n = 0; n < 24; n++) {
		if (!c.in_rom(pc)) return false;
		if (c.owner[pc - ROM_BASE] == 1) return true;        // joins already-decoded code
		if (c.owner[pc - ROM_BASE] == 2) return false;       // lands mid-instruction
		Insn in = c.decode(pc);
		std::string mnem; auto ops = operands(in.text, mnem);
		if (mnem == "db" || in.len == 0) return false;
		bool cond = in.flags & util::disasm_interface::STEP_COND;
		if ((in.flags & util::disasm_interface::STEP_OUT) && !cond) return true;
		if ((mnem == "jp" || mnem == "jr" || mnem == "jrl") && !cond) return true;
		pc += in.len;
	}
	return true;    // long straight-line run without errors: accept
}

void trace(Ctx &c) {
	while (!c.work.empty()) {
		offs_t pc = c.work.front(); c.work.pop_front();
		std::map<std::string, offs_t> imm;     // reg -> immediate loaded in this run
		std::map<std::string, offs_t> table;   // reg -> jump table base it was loaded from
		std::map<std::string, offs_t> romptr;  // reg -> ROM address it was loaded from
		while (c.in_rom(pc)) {
			offs_t off = pc - ROM_BASE;
			if (c.owner[off] == 1) break;      // already decoded
			if (c.owner[off] == 2) { c.notes.push_back(string_format("overlap: flow enters middle of insn at 0x%06x", pc)); break; }
			Insn in = c.decode(pc);
			std::string mnem; auto ops = operands(in.text, mnem);
			if (mnem == "db" || in.len == 0 || !c.in_rom(pc + in.len - 1)) {
				c.notes.push_back(string_format("undefined opcode at 0x%06x", pc));
				break;
			}
			bool clash = false;
			for (unsigned i = 1; i < in.len; i++) if (c.owner[off + i]) clash = true;
			if (clash) { c.notes.push_back(string_format("overlap: insn at 0x%06x runs into decoded code", pc)); break; }
			c.owner[off] = 1;
			for (unsigned i = 1; i < in.len; i++) c.owner[off + i] = 2;
			c.code[pc] = in;
			offs_t next = pc + in.len;

			// --- register tracking for vectors / jump tables / ROM pointers
			if (mnem == "ld" && ops.size() == 2 && is_reg32(ops[0])) {
				// evaluate the source before forgetting the destination: `ld XHL,(XHL+W)` reads XHL
				offs_t v; std::string src = ops[1];
				int kind = 0; offs_t val = 0;   // 1 = immediate, 2 = ROM pointer, 3 = jump table base
				if (parse_hex(src, v)) { kind = 1; val = v; }
				else if (src.size() > 2 && src.front() == '(' && src.back() == ')') {
					std::string inner = src.substr(1, src.size() - 2);
					auto plus = inner.find('+');
					std::string base = inner.substr(0, plus);
					if (parse_hex(inner, v) && c.in_rom(v)) { kind = 2; val = v; }
					else if (plus != std::string::npos && imm.count(base) && c.in_rom(imm[base])) { kind = 3; val = imm[base]; }
				}
				imm.erase(ops[0]); table.erase(ops[0]); romptr.erase(ops[0]);
				if (kind == 1) imm[ops[0]] = val;
				if (kind == 2) romptr[ops[0]] = val;
				if (kind == 3) table[ops[0]] = val;
			}
			if (mnem == "ld" && ops.size() == 2 && is_reg32(ops[1])) {
				offs_t a;
				std::string dst = ops[0];
				if (dst.size() > 2 && dst.front() == '(' && parse_hex(dst.substr(1, dst.size() - 2), a)
				    && a >= 0x6fb8 && a <= 0x6ffc && imm.count(ops[1]))
					c.push(imm[ops[1]], K_VEC, pc);
			}

			bool is_jump = mnem == "jp" || mnem == "jr" || mnem == "jrl";
			bool is_call = mnem == "call" || mnem == "calr";
			bool is_cond = in.flags & util::disasm_interface::STEP_COND;
			bool never = !ops.empty() && ops[0] == "F" && ops.size() == 2;
			if (is_jump || is_call || mnem == "djnz") {
				std::string tgt = ops.empty() ? "" : ops.back();
				offs_t t;
				if (never) { /* jp F,x: never taken */ }
				else if (parse_hex(tgt, t)) c.push(t, is_call ? K_SUB : K_LOC, pc);
				else if (is_reg32(tgt) && table.count(tgt)) {
					// Tables are often followed by other pointer tables (e.g. text), so stop at the
					// first entry that points into the table itself or does not decode as code.
					offs_t base = table[tgt], e = base;
					while (c.in_rom(e + 3) && e - base < 4 * 256) {
						offs_t t = c.rd32(e);
						if (!c.in_rom(t) || (t >= base && t < e + 4) || !plausible_code(c, t)) break;
						c.push(t, K_JT, pc);
						e += 4;
					}
					c.jt_ranges[base] = e;
					c.add_label(base, K_JT, pc);
				}
				else if (is_reg32(tgt) && romptr.count(tgt)) {
					offs_t p = romptr[tgt];
					if (c.in_rom(c.rd32(p))) { c.push(c.rd32(p), K_JT, pc); c.jt_ranges[p] = p + 4; c.add_label(p, K_JT, pc); }
				}
				else {
					c.indirect_unresolved++;
					c.notes.push_back(string_format("unresolved indirect %s at 0x%06x: %s", mnem.c_str(), pc, in.text.c_str()));
				}
				if (is_jump && !is_cond && !never) break;    // unconditional jump ends the run
			}
			if ((in.flags & util::disasm_interface::STEP_OUT) && !is_cond) break;  // ret/reti/retd
			if (is_call) imm.clear(), table.clear(), romptr.clear();               // callee may clobber
			pc = next;
		}
	}
}

std::string label_name(const Ctx &c, offs_t a) {
	auto it = c.labels.find(a);
	if (it == c.labels.end()) return "";
	return string_format("%s_%06x", kind_prefix[it->second], a);
}

// Replace 0x2xxxxx literals that have labels with label names.
std::string symbolize(const Ctx &c, const std::string &text) {
	std::string out;
	for (size_t i = 0; i < text.size();) {
		if (text.compare(i, 2, "0x") == 0) {
			size_t j = i + 2;
			while (j < text.size() && isxdigit((unsigned char)text[j])) j++;
			offs_t v = strtoul(text.substr(i + 2, j - i - 2).c_str(), nullptr, 16);
			std::string l = (j - i >= 8) ? label_name(c, v) : "";
			out += l.empty() ? text.substr(i, j - i) : l;
			i = j;
		} else out += text[i++];
	}
	return out;
}

void emit(const Ctx &c, std::ostream &os) {
	static const char *hdr_cmt[] = { "licence string (28 bytes)", "entry point", "software id", "version / system code", "title (12 bytes)" };
	(void)hdr_cmt;
	size_t code_bytes = 0;
	for (auto &kv : c.code) code_bytes += kv.second.len;
	os << "; Neo Geo Pocket Color disassembly (ngpc-dis, MAME tlcs900 decoder)\n";
	os << string_format("; ROM %u bytes, %u used; code %u bytes (%.1f%% of used) in %u instructions\n",
	                    (unsigned)c.rom.size(), c.used, (unsigned)code_bytes, 100.0 * code_bytes / c.used, (unsigned)c.code.size());
	os << string_format("; labels %u, unresolved indirect jumps/calls %u\n\n", (unsigned)c.labels.size(), c.indirect_unresolved);

	offs_t a = ROM_BASE, end = ROM_BASE + c.used;
	while (a < end) {
		offs_t off = a - ROM_BASE;
		std::string l = label_name(c, a);
		if (!l.empty()) {
			os << "\n" << l << ":";
			auto x = c.xrefs.find(a);
			if (x != c.xrefs.end()) {
				os << "\t\t; xref";
				unsigned n = 0;
				for (offs_t f : x->second) { if (n++ == 6) { os << " ..."; break; } os << string_format(" %06x", f); }
			}
			os << "\n";
		}
		auto ci = c.code.find(a);
		if (ci != c.code.end()) {
			std::string bytes;
			for (unsigned i = 0; i < ci->second.len; i++) bytes += string_format("%02x", c.rom[off + i]);
			os << string_format("  %06x  %-14s  %s\n", a, bytes.c_str(), symbolize(c, ci->second.text).c_str());
			a += ci->second.len;
			continue;
		}
		// data run: until next code / label / 16 bytes; jump tables as dd
		auto jt = c.jt_ranges.find(a);
		if (jt != c.jt_ranges.end()) {
			for (offs_t p = a; p < jt->second; p += 4)
				os << string_format("  %06x  dd %s\n", p, symbolize(c, string_format("0x%06x", c.rd32(p))).c_str());
			a = jt->second;
			continue;
		}
		offs_t b = a;
		std::string hex, asc;
		do {
			u8 v = c.rom[b - ROM_BASE];
			hex += string_format("%s0x%02x", hex.empty() ? "" : ",", v);
			asc += (v >= 0x20 && v < 0x7f) ? char(v) : '.';
			b++;
		} while (b < end && b - a < 16 && c.owner[b - ROM_BASE] == 0 && !c.labels.count(b) && !c.jt_ranges.count(b));
		os << string_format("  %06x  db %-79s ; %s\n", a, hex.c_str(), asc.c_str());
		a = b;
	}
	os << "\n; ---- notes ----\n";
	for (auto &n : c.notes) os << "; " << n << "\n";
}

} // namespace

int main(int argc, char **argv) {
	Ctx c;
	std::vector<offs_t> seeds;
	std::string rom_path, sym_path;
	std::vector<std::string> cov_paths;
	for (int i = 1; i < argc; i++) {
		std::string a = argv[i];
		if (a == "--seed" && i + 1 < argc) seeds.push_back(strtoul(argv[++i], nullptr, 16));
		else if (a == "--sym" && i + 1 < argc) sym_path = argv[++i];
		else if (a == "--cov" && i + 1 < argc) cov_paths.push_back(argv[++i]);
		else rom_path = a;
	}
	if (rom_path.empty()) { std::cerr << "usage: ngpc-dis [--seed hex]... [--sym out.sym] rom.ngc > out.asm\n"; return 2; }
	std::ifstream f(rom_path, std::ios::binary);
	if (!f) { std::cerr << "cannot open " << rom_path << "\n"; return 1; }
	c.rom.assign(std::istreambuf_iterator<char>(f), {});
	if (c.rom.size() < 0x40) { std::cerr << "too small for an NGP cartridge\n"; return 1; }
	c.used = c.rom.size();
	while (c.used > 0x40 && c.rom[c.used - 1] == 0xff) c.used--;
	c.owner.assign(c.rom.size(), 0);

	offs_t entry = c.rd32(ROM_BASE + 0x1c);
	c.push(entry, K_ENTRY);
	for (offs_t s : seeds) c.push(s, K_SEED);
	trace(c);
	// Runtime coverage: trace from every executed address the static pass missed. Each is a
	// known instruction start, so it is pushed without a label; flow from it adds labels.
	unsigned cov_seeds = 0;
	for (auto &p : cov_paths) {
		std::ifstream cf(p);
		if (!cf) { std::cerr << "cannot open " << p << "\n"; return 1; }
		std::string line;
		while (std::getline(cf, line)) {
			offs_t a = strtoul(line.c_str(), nullptr, 16);
			if (c.in_rom(a) && !c.owner[a - ROM_BASE]) { c.push(a, K_NONE); cov_seeds++; }
		}
	}
	trace(c);
	if (!cov_paths.empty()) std::cerr << string_format("coverage: %u extra seeds\n", cov_seeds);
	emit(c, std::cout);

	if (!sym_path.empty()) {
		std::ofstream s(sym_path);
		for (auto &kv : c.labels)
			s << string_format("%06x %-9s %s\n", kv.first, kind_name[kv.second], label_name(c, kv.first).c_str());
	}
	size_t code_bytes = 0;
	for (auto &kv : c.code) code_bytes += kv.second.len;
	std::cerr << string_format("entry 0x%06x: %u instructions, %u code bytes (%.1f%% of %u used), %u labels, %u unresolved indirect, %u notes\n",
	                           entry, (unsigned)c.code.size(), (unsigned)code_bytes, 100.0 * code_bytes / c.used, c.used,
	                           (unsigned)c.labels.size(), c.indirect_unresolved, (unsigned)c.notes.size());
	return 0;
}
