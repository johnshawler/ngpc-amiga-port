/* SPDX-License-Identifier: GPL-2.0-or-later. Private resource equality runner. */
#include "aim_unpack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t *read_file(const char *name,size_t *n)
{
    FILE *f=fopen(name,"rb");long length;uint8_t *p;
    if(!f || fseek(f,0,SEEK_END) || (length=ftell(f))<0 || length>2000000 || fseek(f,0,SEEK_SET))exit(2);
    *n=(size_t)length;p=malloc(*n+1);if(!p || fread(p,1,*n,f)!=*n)exit(2);fclose(f);return p;
}
int main(int argc,char **argv)
{
    uint8_t *source,*expected,*out;size_t n,m;
    if(argc!=3)return 2;
    source=read_file(argv[1],&n);expected=read_file(argv[2],&m);out=malloc(m+2);if(!out)return 2;
    memset(out,0xa5,m+2);
    if(aim_unpack(source,n,out+1,m) || memcmp(out+1,expected,m) || out[0]!=0xa5 || out[m+1]!=0xa5)return 1;
    printf("exact unpacked equality: %lu -> %lu bytes PASS\n",(unsigned long)n,(unsigned long)m);
    free(source);free(expected);free(out);return 0;
}
