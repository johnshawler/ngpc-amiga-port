/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "fire_stream.h"
typedef struct {const fire_stream *io;uint8_t bytes[512];size_t at,count;uint32_t left,hash;} reader;
static uint32_t be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static int exact(const fire_stream *io,uint8_t *p,size_t n)
{
    while(n){long got=io->read(io->context,p,n);if(got<0)return FIRE_PACK_IO;
        if(!got || (size_t)got>n)return FIRE_PACK_FORMAT;
        p+=(size_t)got;n-=(size_t)got;}
    return 0;
}
static int byte(reader *r)
{
    unsigned v;
    if(r->at==r->count){size_t n=r->left;long got;if(!n)return FIRE_PACK_FORMAT;
        if(n>sizeof r->bytes)n=sizeof r->bytes;
        got=r->io->read(r->io->context,r->bytes,n);
        if(got<0)return FIRE_PACK_IO;
        if(!got || (size_t)got>n)return FIRE_PACK_FORMAT;
        r->at=0;r->count=(size_t)got;r->left-=(uint32_t)got;}
    v=r->bytes[r->at++];r->hash=(r->hash^v)*16777619u;return (int)v;
}
static int pass(const fire_stream *io,const fire_pack_spec *spec,uint8_t *target,int write)
{
    reader r;uint8_t header[20],extra;size_t out=0;int code,hi,lo,result;long trailing;
    if(io->rewind(io->context)<0)return FIRE_PACK_IO;
    result=exact(io,header,sizeof header);if(result)return result;
    {static const uint8_t magic[8]={'N','T','M','F','P','K','1',0};unsigned i;
        for(i=0;i<8;i++)if(header[i]!=magic[i])return FIRE_PACK_FORMAT;}
    if(be32(header+8)!=spec->raw_bytes || be32(header+12)!=spec->packed_bytes ||
       be32(header+16)!=spec->checksum)return FIRE_PACK_FORMAT;
    r.io=io;r.at=r.count=0;r.left=spec->packed_bytes;r.hash=2166136261u;
    while(r.left || r.at<r.count){size_t n,d,i;
        code=byte(&r);if(code<0)return code;
        n=(size_t)(code&127)+(code&128?3:1);
        if(n>spec->raw_bytes-out)return FIRE_PACK_FORMAT;
        if(code&128){
            hi=byte(&r);if(hi<0)return hi;lo=byte(&r);if(lo<0)return lo;
            d=((size_t)hi<<8)|(unsigned)lo;
            if(!d || d>out)return FIRE_PACK_FORMAT;
            if(write)for(i=0;i<n;i++)target[out+i]=target[out+i-d];
        }else{
            for(i=0;i<n;i++){code=byte(&r);if(code<0)return code;if(write)target[out+i]=(uint8_t)code;}
        }
        out+=n;
    }
    if(out!=spec->raw_bytes)return FIRE_PACK_FORMAT;
    trailing=io->read(io->context,&extra,1);
    if(trailing<0)return FIRE_PACK_IO;
    if(trailing)return FIRE_PACK_FORMAT;
    if(r.hash!=spec->checksum)return FIRE_PACK_CHECKSUM;
    return 0;
}
int fire_stream_unpack(const fire_stream *io,const fire_pack_spec *spec,uint8_t *target,size_t capacity)
{
    int r;size_t i;
    if(!io || !spec || !io->read || !io->rewind || !target || !spec->raw_bytes ||
       !spec->packed_bytes || capacity!=spec->raw_bytes)return FIRE_PACK_ARGUMENT;
    r=pass(io,spec,target,0);if(r)return r;
    r=pass(io,spec,target,1);
    if(r)for(i=0;i<capacity;i++)target[i]=0;
    return r;
}
