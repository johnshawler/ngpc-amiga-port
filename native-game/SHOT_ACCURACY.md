# Native accuracy child

`shot_accuracy` converts original `20bc19`, callback `20bc5b`, oscillator
`20bcec`, marker `20bd3e`/`20a362`, lie eligibility `20dbd1`, and the genuine
lifecycle completion path. It needs no new graphics or lookup resources.

Entry starts at position 0x1300, takes the original velocity word from hole
bytes 0x79/0x7a (zero for club 13), and sets lower=hole[0x7b] and
upper=uint8(38-lower). It installs the native wait adapter without advancing.
Task scratch is +0x16 position word, +0x18 velocity word, +0x1a upper byte,
and +0x1b lower byte. Native phase is separate from cartridge scratch.

Every lifecycle except 4 advances. Signed high-byte candidate comparisons
bounce inclusively at 37 and 1. Unlike the power meter, replacing the high byte
preserves the candidate's fractional low byte. Wrapped word reversal and the
final add are reproduced exactly. Object 7's Y becomes the zero-extended byte
151-position_high plus screen Y, wrapped to 16 bits. No art is queued.

Lifecycle 4 commits position_high to hole[0x78]. Signed accuracy greater than
the upper limit sets aim.reset_d2=1; below lower sets it to 2. Other cases do
not clear existing flags. Club 13 skips further classification. For signed
label>9, eligible lies copy held-input&3 to hole[0x7c]. Eligibility accepts lies
0 and 4 universally; nonzero player modifier also accepts 1 and 2; with zero
modifier, character 1 accepts neither, other characters accept 1, and character
4 also accepts 2. The held byte is addressed by original active player+0x31,
not the pressed-edge byte.

Accuracy 18..20, signed power >= uint8(power_limit-4), and signed label<=9 set
aim.reset_d3=1. Completion then runs the real task helper with unchanged defer,
unlink/recycle, callback and scheduler-cursor behavior. This child does not
start the parent swing transition or simulate ball flight.

`tools/check-shot-accuracy.sh` runs host, ASan/UBSan, freestanding m68020 compile,
and m68020/QEMU authored tests, then the unchanged-original comparator. The
comparator checks all nonstack work RAM, scratch and task links, hit flags,
marker position, and hardware/video memory. Only measured original stack writes
are excluded; no original call is intercepted and the original core is unchanged.
The reusable ctypes layout is `reference/shot_accuracy_case_common.py`.
