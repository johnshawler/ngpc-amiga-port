/* Read-only, frame-boundary oracle adapter for pinned Beetle NeoPop.
 * GPL-2.0-or-later, matching the core it extends. Not an Amiga runtime.
 * Wire fields are big-endian and never native C structure serialization. */
static void ref_put32(uint8_t *p, uint32_t v)
{
   p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}
__attribute__((visibility("default")))
size_t retro_reference_snapshot(void *output, size_t capacity)
{
   const size_t size = 16 + 23 * 4 + 16384 + 16384;
   uint8_t *out = (uint8_t *)output;
   unsigned n = 0, bank, reg, address;
   if (!out) return size;
   if (capacity < size || !NGPGfx) return 0;
   memcpy(out, "NGR1", 4);
   ref_put32(out + 4, 1); ref_put32(out + 8, (uint32_t)size);
   ref_put32(out + 12, 23);
   out += 16;
   ref_put32(out + 4 * n++, pc);
   ref_put32(out + 4 * n++, sr);
   ref_put32(out + 4 * n++, f_dash);
   for (bank = 0; bank < 4; bank++)
      for (reg = 0; reg < 4; reg++) ref_put32(out + 4 * n++, gprBank[bank][reg]);
   for (reg = 0; reg < 4; reg++) ref_put32(out + 4 * n++, gpr[reg]);
   memcpy(out + 92, CPUExRAM, 16384);
   /* ngpgfx_read8 is read-only in the pinned source. Includes readable MMIO,
    * palette, sprite and scroll/character RAM; unmapped reads return zero.
    * This is not a save state or full latched renderer/peripheral state. */
   for (address = 0x8000; address < 0xc000; address++)
      out[92 + 16384 + address - 0x8000] = ngpgfx_read8(NGPGfx, address);
   return size;
}
