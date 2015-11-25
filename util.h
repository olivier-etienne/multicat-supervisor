/*****************************************************************************
 * util.h: Utils for the multicat suite
 *****************************************************************************
 * Copyright (C) 2009, 2011, 2014 VideoLAN
 * $Id: util.h 59 2014-08-09 14:09:10Z massiot $
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include <netinet/udp.h>
#include <netinet/ip.h>
#include "eit.h"

#ifdef __APPLE__
#define POLLRDHUP 0
#else
#define HAVE_CLOCK_NANOSLEEP
#endif

#define DEFAULT_PORT 1234
#define DEFAULT_PAYLOAD_SIZE 1316
#define DEFAULT_ROTATE_SIZE UINT64_C(97200000000)
#define TS_SIZE 188
#define RTP_HEADER_SIZE 12

#define VERB_DBG  3
#define VERB_INFO 2
#define VERB_WARN 1

#define EMPTY_STRING			""

/*****************************************************************************
 * sockaddr_t: wrapper to avoid strict-aliasing issues
 *****************************************************************************/
typedef union
{
    struct sockaddr_storage ss;
    struct sockaddr so;
    struct sockaddr_in sin;
    struct sockaddr_in6 sin6;
} sockaddr_t;

/*****************************************************************************
 * Raw udp packet structure with flexible-array payload
 *****************************************************************************/
struct udprawpkt {
#ifndef __APPLE__
    struct  iphdr iph;
    struct  udphdr udph;
    uint8_t payload[];
#endif
} __attribute__((packed));


/*****************************************************************************
 * OpenSocket options
 *****************************************************************************/
 struct opensocket_opt {
    struct udprawpkt *p_raw_pktheader;
 };


/*****************************************************************************
 * Prototypes
 *****************************************************************************/
void msg_Info( void *_unused, const char *psz_format, ... );
void msg_Err( void *_unused, const char *psz_format, ... );
void msg_Warn( void *_unused, const char *psz_format, ... );
void msg_Dbg( void *_unused, const char *psz_format, ... );
void msg_Raw( void *_unused, const char *psz_format, ... );
uint64_t wall_Date( void );
void wall_Sleep( uint64_t i_delay );
uint64_t real_Date( void );
void real_Sleep( uint64_t i_delay );
int OpenSocket( const char *_psz_arg, int i_ttl, uint16_t i_bind_port,
                uint16_t i_connect_port, unsigned int *pi_weight, bool *pb_tcp,
                struct opensocket_opt *p_opt);
mode_t StatFile(const char *psz_arg);
int OpenFile( const char *psz_arg, bool b_read, bool b_append );
char *GetAuxFile( const char *psz_arg, size_t i_payload_size );
FILE *OpenAuxFile( const char *psz_arg, bool b_read, bool b_append );
off_t LookupAuxFile( const char *psz_arg, int64_t i_wanted, bool b_absolute );
void CheckFileSizes( const char *psz_file, const char *psz_aux_file,
                     size_t i_payload_size );
uint64_t GetDirFile( uint64_t i_rotate_size, int64_t i_wanted );
int OpenDirFile( const char *psz_dir_path, uint64_t i_file, bool b_read,
                 size_t i_payload_size, FILE **pp_aux_file );
off_t LookupDirAuxFile( const char *psz_dir_path, uint64_t i_file,
                        int64_t i_wanted, size_t i_payload_size );

void dump_eit_section(struct EitInfoSection * section );						
void dump_eit_info(struct EitInfo * str_eit_info );
void load_eit(struct EitInfo *str_eit_info);
uint16_t convertStrToLong(char * str);
uint8_t convertStrToInt(char * str);
char * getValueFromIni(char * tag,char *prefix,int index);
void utf8_to_latin9(const char *const input);



/*****************************************************************************
 * Aux files helpers
 *****************************************************************************/
static inline uint64_t FromSTC( const uint8_t *p_aux )
{
    return ((uint64_t)p_aux[0] << 56)
         | ((uint64_t)p_aux[1] << 48)
         | ((uint64_t)p_aux[2] << 40)
         | ((uint64_t)p_aux[3] << 32)
         | ((uint64_t)p_aux[4] << 24)
         | ((uint64_t)p_aux[5] << 16)
         | ((uint64_t)p_aux[6] << 8)
         | ((uint64_t)p_aux[7] << 0);
}

static inline void ToSTC( uint8_t *p_aux, uint64_t i_stc )
{
    p_aux[0] = i_stc >> 56;
    p_aux[1] = (i_stc >> 48) & 0xff;
    p_aux[2] = (i_stc >> 40) & 0xff;
    p_aux[3] = (i_stc >> 32) & 0xff;
    p_aux[4] = (i_stc >> 24) & 0xff;
    p_aux[5] = (i_stc >> 16) & 0xff;
    p_aux[6] = (i_stc >> 8) & 0xff;
    p_aux[7] = (i_stc >> 0) & 0xff;
}

/*****************************************************************************
 * Retx helpers - biTStream style
 *****************************************************************************/
#define RETX_HEADER_SIZE 8

static inline void retx_init(uint8_t *p_retx)
{
    p_retx[0] = 'R';
    p_retx[1] = 'E';
    p_retx[2] = 'T';
    p_retx[3] = 'X';
}

static inline bool retx_check(const uint8_t *p_retx)
{
    return p_retx[0] == 'R' && p_retx[1] == 'E' && p_retx[2] == 'T' &&
           p_retx[3] == 'X';
}

static inline void retx_set_seqnum(uint8_t *p_retx, uint16_t i_seqnum)
{
    p_retx[4] = i_seqnum >> 8;
    p_retx[5] = i_seqnum & 0xff;
}

static inline uint16_t retx_get_seqnum(const uint8_t *p_retx)
{
    return ((uint16_t)p_retx[4] << 8) | p_retx[5];
}

static inline void retx_set_num(uint8_t *p_retx, uint16_t i_num)
{
    p_retx[6] = i_num >> 8;
    p_retx[7] = i_num & 0xff;
}

static inline uint16_t retx_get_num(const uint8_t *p_retx)
{
    return ((uint16_t)p_retx[6] << 8) | p_retx[7];
}


static inline unsigned int to_latin9(const unsigned int code)
{
    /* Code points 0 to U+00FF are the same in both. */
    if (code < 256U)
        return code;
    switch (code) {
    case 0x0152U: return 188U; /* U+0152 = 0xBC: OE ligature */
    case 0x0153U: return 189U; /* U+0153 = 0xBD: oe ligature */
    case 0x0160U: return 166U; /* U+0160 = 0xA6: S with caron */
    case 0x0161U: return 168U; /* U+0161 = 0xA8: s with caron */
    case 0x0178U: return 190U; /* U+0178 = 0xBE: Y with diaresis */
    case 0x017DU: return 180U; /* U+017D = 0xB4: Z with caron */
    case 0x017EU: return 184U; /* U+017E = 0xB8: z with caron */
    case 0x20ACU: return 164U; /* U+20AC = 0xA4: Euro */
    default:      return 256U;
    }
}

