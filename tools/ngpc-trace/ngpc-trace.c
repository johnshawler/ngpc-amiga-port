/*
 * ngpc-trace: headless Neo Geo Pocket Color runner for reverse engineering.
 *
 * Statically links the Mednafen NGP core (third_party/ngpc/beetle-ngp-libretro) and drives it
 * from a small input script. While running it records every executed TLCS-900H instruction
 * address (code coverage for the disassembler) and can dump video RAM, work RAM and frames.
 *
 * Usage: ngpc-trace rom.ngc script.txt outdir
 *
 * Script commands (one per line, '#' comments):
 *   wait N                 run N frames with no buttons
 *   press BUTTONS N        hold BUTTONS for N frames, then release for 2 frames
 *                          BUTTONS: any of u d l r a b o joined by '+', e.g. a, u+a, o
 *   mash N SEED            N frames of pseudo-random input (coverage exploration)
 *   dump NAME              write NAME.vram (0x8000-0xbfff), NAME.ram (0x4000-0x7fff), NAME.ppm
 *   cov NAME               write coverage so far to NAME.cov (one hex address per line)
 *
 * Environment: NGPC_LANG=english|japanese selects the console language (default english).
 *
 * Outputs coverage.cov (all executed ROM addresses) at the end.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "libretro.h"

/* core internals */
uint8_t loadB(uint32_t address);

/* ---- coverage hook, called from the patched TLCS900h_interpret() ---- */
#define ROM_BASE 0x200000u
#define ROM_SPAN 0x400000u              /* 0x200000-0x5fffff covers both cart chips */
static uint8_t cov[ROM_SPAN / 8];
static uint64_t insn_count;

void ngpc_trace_pc(uint32_t pc)
{
	uint32_t o = (pc & 0xffffff) - ROM_BASE;
	insn_count++;
	if (o < ROM_SPAN) cov[o >> 3] |= 1u << (o & 7);
}

/* ---- libretro frontend ---- */
static uint16_t frame[256 * 256];
static unsigned fw, fh;
static unsigned pix_fmt = RETRO_PIXEL_FORMAT_0RGB1555;
static uint16_t buttons;                /* libretro joypad bitmask */
static const char *sysdir = ".";
static const char *lang = "english";

static bool env_cb(unsigned cmd, void *data)
{
	switch (cmd) {
	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: pix_fmt = *(const enum retro_pixel_format *)data; return true;
	case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
	case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: *(const char **)data = sysdir; return true;
	case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: return true;
	case RETRO_ENVIRONMENT_GET_VARIABLE: {
		struct retro_variable *v = data;
		if (v->key && !strcmp(v->key, "ngp_language")) { v->value = lang; return true; }
		return false;
	}
	default: return false;
	}
}
static void video_cb(const void *data, unsigned w, unsigned h, size_t pitch)
{
	if (!data) return;
	fw = w; fh = h;
	for (unsigned y = 0; y < h && y < 256; y++)
		memcpy(frame + y * 256, (const uint8_t *)data + y * pitch, w * 2);
}
static void audio_cb(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t audio_batch_cb(const int16_t *d, size_t n) { (void)d; return n; }
static void input_poll_cb(void) {}
static int16_t input_state_cb(unsigned port, unsigned dev, unsigned idx, unsigned id)
{
	(void)idx;
	if (port || dev != RETRO_DEVICE_JOYPAD) return 0;
	if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return (int16_t)buttons;
	return (buttons >> id) & 1;
}

static uint16_t parse_buttons(const char *s)
{
	uint16_t b = 0;
	for (; *s; s++) switch (*s) {
		case 'u': b |= 1 << RETRO_DEVICE_ID_JOYPAD_UP; break;
		case 'd': b |= 1 << RETRO_DEVICE_ID_JOYPAD_DOWN; break;
		case 'l': b |= 1 << RETRO_DEVICE_ID_JOYPAD_LEFT; break;
		case 'r': b |= 1 << RETRO_DEVICE_ID_JOYPAD_RIGHT; break;
		case 'a': b |= 1 << RETRO_DEVICE_ID_JOYPAD_B; break;      /* NGP A */
		case 'b': b |= 1 << RETRO_DEVICE_ID_JOYPAD_A; break;      /* NGP B */
		case 'o': b |= 1 << RETRO_DEVICE_ID_JOYPAD_START; break;  /* Option */
	}
	return b;
}

static void run(unsigned n) { while (n--) retro_run(); }

static void write_bytes(const char *path, uint32_t from, uint32_t to)
{
	FILE *f = fopen(path, "wb");
	if (!f) { perror(path); exit(1); }
	for (uint32_t a = from; a < to; a++) fputc(loadB(a), f);
	fclose(f);
}

static void write_ppm(const char *path)
{
	FILE *f = fopen(path, "wb");
	if (!f) { perror(path); exit(1); }
	fprintf(f, "P6\n%u %u\n255\n", fw, fh);
	for (unsigned y = 0; y < fh; y++)
		for (unsigned x = 0; x < fw; x++) {
			uint16_t p = frame[y * 256 + x];
			unsigned r, g, b;
			if (pix_fmt == RETRO_PIXEL_FORMAT_RGB565) { r = p >> 11; g = (p >> 5) & 63; b = p & 31; r = r * 255 / 31; g = g * 255 / 63; b = b * 255 / 31; }
			else { r = (p >> 10) & 31; g = (p >> 5) & 31; b = p & 31; r = r * 255 / 31; g = g * 255 / 31; b = b * 255 / 31; }
			fputc(r, f); fputc(g, f); fputc(b, f);
		}
	fclose(f);
}

static void write_cov(const char *path)
{
	FILE *f = fopen(path, "w");
	if (!f) { perror(path); exit(1); }
	unsigned n = 0;
	for (uint32_t o = 0; o < ROM_SPAN; o++)
		if (cov[o >> 3] & (1u << (o & 7))) { fprintf(f, "%06x\n", ROM_BASE + o); n++; }
	fclose(f);
	fprintf(stderr, "%s: %u distinct instruction addresses (%llu executed)\n", path, n, (unsigned long long)insn_count);
}

int main(int argc, char **argv)
{
	if (argc != 4) { fprintf(stderr, "usage: ngpc-trace rom.ngc script.txt outdir\n"); return 2; }
	const char *outdir = argv[3];
	sysdir = outdir;
	if (getenv("NGPC_LANG")) lang = getenv("NGPC_LANG");

	FILE *rf = fopen(argv[1], "rb");
	if (!rf) { perror(argv[1]); return 1; }
	fseek(rf, 0, SEEK_END); long sz = ftell(rf); fseek(rf, 0, SEEK_SET);
	void *rom = malloc(sz);
	if (fread(rom, 1, sz, rf) != (size_t)sz) { perror("read"); return 1; }
	fclose(rf);

	retro_set_environment(env_cb);
	retro_set_video_refresh(video_cb);
	retro_set_audio_sample(audio_cb);
	retro_set_audio_sample_batch(audio_batch_cb);
	retro_set_input_poll(input_poll_cb);
	retro_set_input_state(input_state_cb);
	retro_init();
	struct retro_game_info gi = { argv[1], rom, (size_t)sz, NULL };
	if (!retro_load_game(&gi)) { fprintf(stderr, "core refused the ROM\n"); return 1; }

	FILE *sf = fopen(argv[2], "r");
	if (!sf) { perror(argv[2]); return 1; }
	char line[256], path[1024];
	unsigned frames = 0;
	while (fgets(line, sizeof line, sf)) {
		char cmd[32] = "", a1[64] = "", a2[64] = "";
		char *hash = strchr(line, '#'); if (hash) *hash = 0;
		if (sscanf(line, "%31s %63s %63s", cmd, a1, a2) < 1) continue;
		if (!strcmp(cmd, "wait")) { unsigned n = atoi(a1); buttons = 0; run(n); frames += n; }
		else if (!strcmp(cmd, "press")) { unsigned n = atoi(a2); buttons = parse_buttons(a1); run(n); buttons = 0; run(2); frames += n + 2; }
		else if (!strcmp(cmd, "mash")) {
			unsigned n = atoi(a1); uint32_t s = strtoul(a2, NULL, 0) | 1;
			for (unsigned i = 0; i < n; i++) {
				if (i % 6 == 0) {           /* new random d-pad/A/B combo every 6 frames; Option rarely */
					s ^= s << 13; s ^= s >> 17; s ^= s << 5;
					buttons = 0;
					static const unsigned ids[] = { RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN,
						RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_A };
					for (unsigned k = 0; k < 6; k++) if (s >> (k * 3) & 1) buttons |= 1 << ids[k];
					if ((s >> 24) % 64 == 0) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_START;
				}
				retro_run();
			}
			buttons = 0; frames += n;
		}
		else if (!strcmp(cmd, "dump")) {
			snprintf(path, sizeof path, "%s/%s.vram", outdir, a1); write_bytes(path, 0x8000, 0xc000);
			snprintf(path, sizeof path, "%s/%s.ram", outdir, a1);  write_bytes(path, 0x4000, 0x8000);
			snprintf(path, sizeof path, "%s/%s.ppm", outdir, a1);  write_ppm(path);
			fprintf(stderr, "frame %u: dumped %s\n", frames, a1);
		}
		else if (!strcmp(cmd, "cov")) { snprintf(path, sizeof path, "%s/%s.cov", outdir, a1); write_cov(path); }
		else { fprintf(stderr, "unknown script command: %s\n", cmd); return 1; }
	}
	fclose(sf);
	snprintf(path, sizeof path, "%s/coverage.cov", outdir);
	write_cov(path);
	retro_unload_game();
	retro_deinit();
	return 0;
}
