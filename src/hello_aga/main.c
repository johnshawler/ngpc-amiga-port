/* AGA smoke test: open a 320x256 8-bitplane screen, draw 256 colour bars
   using a 12-bit (NGPC-style) RGB ramp, wait ~5 s, exit. */
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <intuition/screens.h>
#include <graphics/modeid.h>

struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

int main(void)
{
    struct Screen *scr;
    ULONG err = 0;
    int i;

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    if (!GfxBase || !IntuitionBase) {
        PutStr("Needs Kickstart 3.0+ (graphics/intuition v39)\n");
        return 20;
    }
    /* Without SetPatch (no Workbench), KS3.x leaves graphics.library in ECS mode.
       SetChipRev(SETCHIPREV_BEST) enables AGA, as SetPatch would. */
    if (!(GfxBase->ChipRevBits0 & GFXF_AA_ALICE)) SetChipRev(SETCHIPREV_BEST);
    Printf((STRPTR)"graphics v%ld ChipRevBits0=$%02lx\n", (LONG)GfxBase->LibNode.lib_Version, (ULONG)GfxBase->ChipRevBits0);
    if (!(GfxBase->ChipRevBits0 & GFXF_AA_ALICE)) PutStr("Warning: no AGA chipset\n");

    scr = OpenScreenTags(NULL,
        SA_Width, 320, SA_Height, 256, SA_Depth, 8,
        SA_DisplayID, PAL_MONITOR_ID | LORES_KEY,
        SA_ErrorCode, (ULONG)&err,
        SA_Quiet, TRUE, SA_ShowTitle, FALSE, SA_Type, CUSTOMSCREEN,
        TAG_DONE);
    if (!scr) { Printf((STRPTR)"OpenScreen failed, error %ld\n", (LONG)err); return 20; }

    for (i = 0; i < 256; i++) {
        /* 12-bit colour like the NGPC: 4 bits per gun, scaled to 32 bits */
        ULONG r = (i >> 4) & 0xF, g = i & 0xF, b = 15 - ((i >> 4) & 0xF);
        SetRGB32(&scr->ViewPort, i, r * 0x11111111UL, g * 0x11111111UL, b * 0x11111111UL);
        SetAPen(&scr->RastPort, i);
        RectFill(&scr->RastPort, (i % 32) * 10, (i / 32) * 32, (i % 32) * 10 + 9, (i / 32) * 32 + 31);
    }
    PutStr("hello_aga: 320x256x8 AGA screen open\n");
    Delay(50 * 5);
    CloseScreen(scr);
    CloseLibrary((struct Library *)IntuitionBase);
    CloseLibrary((struct Library *)GfxBase);
    return 0;
}
