#!/usr/bin/env python3
"""Instrument a fresh private reference export; diagnostic trace stays local."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
p=root/'mednafen/ngp/mem.c';s='#include <stdio.h>\n'+p.read_text()
a='uint8_t loadB(uint32 address)\n{'
assert s.count(a)==1
s=s.replace(a,'extern uint32_t control_pc;\nextern unsigned control_window,control_events;\n'+a+'''\n   if((address&0xffffff)==0x6f82 && control_events<4) {
      fprintf(stderr,"CONTROL_READ pc=%06x value=%02x\\n",control_pc,CPUExRAM[0x2f82]);
      control_window=64;control_events++;
   }
''');p.write_text(s)
p=root/'mednafen/ngp/TLCS-900h/TLCS900h_interpret.c';s='#include <stdio.h>\n'+p.read_text()
a='int32 TLCS900h_interpret(void)\n{';assert s.count(a)==1
s=s.replace(a,'uint32_t control_pc;\nunsigned control_window,control_events;\n'+a+'\n    control_pc=pc;')
a='\treturn cycles + cycles_extra;';assert s.count(a)==1
s=s.replace(a,'''    if(control_window) {
        unsigned i;control_window--;
        fprintf(stderr,"CONTROL_STEP %06x next=%06x sr=%04x bank=%u",control_pc,pc,sr,statusRFP);
        for(i=0;i<4;i++)fprintf(stderr," r%u=%08x",i,gprBank[statusRFP][i]);
        for(i=0;i<4;i++)fprintf(stderr," x%u=%08x",i,gpr[i]);
        fprintf(stderr,"\\n");
    }
'''+a);p.write_text(s)
