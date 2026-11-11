# Native first-shot HUD

`shot_hud.c` implements original `20a38a`, its initial label/wind/number
helpers, and the subsequent `20a473` field clear. It is a service for the
normal first-tee scene setup, not a playable shot or an independent application.

The caller supplies borrowed font, header, label and seventeen wind icon spans.
The original first-tee label selector is 4; it is separate from the club index.
The resource record carries its supported label identity, so an unmatched label
is rejected rather than drawing substitute art. The caller owns the existing
screen queue, maps, tiles, text base, number scratch and four-byte saved HUD.

Order is preserved:

1. Zero the text base and copy the indexed font to tile byte offset 256.
2. Draw the header immediately at map offset 0, tile base 64.
3. Queue the label, draw wind digits, then queue the wind icon. With zero wind
   the dedicated seventeenth icon is used. A full queue silently drops requests.
4. Convert distance with the original signed fixed-point scaling and draw its
   three decimal digits. The original short/full-product rounding is preserved.
5. Save the first two words of the map's second row into the four-byte HUD state.

The separate clear repeats the word at map offset 68 into seven following
words. It does not zero the region or restore saved words. Immediate writes and
queued jobs are intentionally not merged into one drawing operation.

The API rejects invalid pointers, queue alignment/size, direction and label
contracts before mutation. Asset validation follows the existing rendering
services; a later malformed resource can fail after earlier operations. Callers
must use validated resource sets and stop on failure. Assets must remain alive
until queued stamps are drained.

## Verification

`bash tools/check-shot-hud.sh` runs authored fixtures on the host, ASan/UBSan
and m68020/QEMU, then compares both ordinary and private tile-copy backends
against the unchanged original routine. It requires the existing private ROM
and reference core only for the original comparison.

- 8,448 public cases cover all normal wind values/directions and every queue
  occupancy, signed distance arithmetic, digits, saved words, clear and invalid
  input rejection.
- 768 original cases per backend cover every normal wind value/direction with
  an empty queue, one free slot and a full queue. Each checks all nonstack RAM,
  all 4,096 map bytes and 8,192 tile bytes, then checks the original field clear.

`python3 -m unittest tests.test_shot_hud` needs no cartridge. The native helper
uses only 32-bit arithmetic; the independent synthetic reference uses 64-bit
arithmetic only in the test. No target timing or visual presentation result is
claimed by this isolated service test.
