#!/usr/bin/env python3
"""Find original game input-state readers in a fresh private core export."""
from pathlib import Path
import sys
root=Path(sys.argv[1]);p=root/'mednafen/ngp/mem.c';s=p.read_text()
probe='''
#include <stdio.h>
extern uint32_t consumer_pc,consumer_tick;
static uint32_t consumer_sites[512];
static unsigned consumer_hits[512],consumer_used;
static void consumer_read(unsigned address,unsigned width)
{
    unsigned i;
    if(address<0x40bf || address>0x40c2 || (consumer_pc>=0x2002c2 && consumer_pc<=0x20042d))return;
    for(i=0;i<consumer_used;i++)if(consumer_sites[i]==consumer_pc)break;
    if(i==consumer_used){if(i==512)return;consumer_sites[consumer_used++]=consumer_pc;}
    if(consumer_hits[i]==0 || (CPUExRAM[0xc0] && consumer_hits[i]<4)) {
        fprintf(stderr,"CONSUMER tick=%u pc=%06x address=%04x width=%u current=%02x pressed=%02x periodic=%02x compared=%02x\\n",
            consumer_tick,consumer_pc,address,width,CPUExRAM[0xbf],CPUExRAM[0xc0],CPUExRAM[0xc1],CPUExRAM[0xc2]);
        consumer_hits[i]++;
    }
}
'''
a='uint8_t loadB(uint32 address)\n{';assert s.count(a)==1;s=s.replace(a,probe+'\n'+a+'\n   consumer_read(address,1);')
for a,width in [('uint16_t loadW(uint32 address)\n{',2),('uint32 loadL(uint32 address)\n{',4)]:
 assert s.count(a)==1;s=s.replace(a,a+f'\n   consumer_read(address,{width});')
p.write_text(s)
p=root/'mednafen/ngp/TLCS-900h/TLCS900h_interpret.c';s=p.read_text();a='int32 TLCS900h_interpret(void)\n{';assert s.count(a)==1
s=s.replace(a,'#include <stdio.h>\nextern uint8_t CPUExRAM[16384];\nuint32_t consumer_pc,consumer_tick;\nstatic unsigned selection_logged;\n'+a+'''
    consumer_pc=pc;if(pc==0x2002c2)consumer_tick++;
    if(pc==0x209536 && (selection_logged==0 || (CPUExRAM[0xc0] && selection_logged<4))) {
        unsigned i;selection_logged++;
        fprintf(stderr,"SELECTION_ENTRY tick=%u sr=%04x bank=%u",consumer_tick,sr,statusRFP);
        for(i=0;i<4;i++)fprintf(stderr," r%u=%08x",i,gprBank[statusRFP][i]);
        for(i=0;i<4;i++)fprintf(stderr," x%u=%08x",i,gpr[i]);
        fprintf(stderr," state=");for(i=0xed;i<0x11d;i++)fprintf(stderr,"%02x",CPUExRAM[i]);
        fprintf(stderr,"\\n");
    }
''');p.write_text(s)
