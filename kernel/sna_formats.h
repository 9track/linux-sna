/* sna_formats.h: Linux-SNA data headers and declarations.
 * - Kept in one place to ease maintance headaches
 *
 * Copyright (c) 1999-2002 by Jay Schulist <jschlst@linux-sna.org>
 *
 * This program can be redistributed or modified under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * This program is distributed without any warranty or implied warranty
 * of merchantability or fitness for a particular purpose.
 *
 * See the GNU General Public License for more details.
 */

#ifndef __NET_SNA_FORMATS_H
#define __NET_SNA_FORMATS_H

#ifdef __KERNEL__

/* MU.
 */

#ifdef NOT
struct sna_sender {
        __u8 id;
        __u8 type;
};

struct sna_bind_rq_send {
        __u8 lu_id;
        struct sna_sender *sender;
        __u8 hs_id;
        __u8 trans_prior;
        struct sna_lfsid *lfsid;
        __u8 path_id;
        __u8 parallel;
        __u8 adaptive_pace;
};

struct sna_bind_rsp_send {
        struct sna_sender *sender;
        struct sna_lfsid *lfsid;
        __u8 hs_id;
        __u8 trans_prior;
        __u8 path_id;
        __u8 free_lfsid;
        __u8 parallel;
        __u8 adaptive_pace;
};

struct sna_unbind_rq {
        struct sna_sender *sender;
        struct sna_lfsid *lfsid;
        __u8 hs_id;
        __u8 trans_prior;
        __u8 free_lfsid;
        __u8 path_id;
        __u8 parallel;
        __u8 adaptaive_pace;
};

struct sna_unbind_rsp_send {
        __u8 lu_id;
        struct sna_sender *sender;
        __u8 hs_id;
        __u8 trans_prior;
        __u8 free_lfsid;
        __u8 path_id;
        __u8 parallel;
        __u8 adaptive_pace;
};

struct sna_pc_character {
        __u16 max_send_btu_size;
        __u16 max_rcv_btu_size;
        __u8 adjacent_node_bind_reasm;
        __u8 limit_resource;
};

struct sna_bind_rq_rcv {
        struct sna_sender *sender;
        struct sna_lfsid *lfsid;
        __u8 hs_id;
        __u8 trans_prior;
        __u8 free_lfsid;
        __u8 path_id;
        struct sna_pc_character *pc_characteristics;
        __u8 parallel;
        __u8 adaptive_pace;
};

struct sna_bind_rsp_rcv {
        struct sna_sender *sender;
        struct sna_lfsid *lfsid;
        __u8 hs_id;
        __u8 tx_priority;
        __u8 free_lfsid;
        __u8 path_id;
        __u8 parallel;
        __u8 adaptive_pace;
};
#endif

/* COMPRESSION.
 */

#ifdef NOT

/* Compression algorithms */
#define RU_COMPRESS_RLE 000
#define RU_COMPRESS_LZ  001

/* Compression header */
struct sna_ru_compress
{
        unsigned char   compress:4,     /* Compression algorithm */
                        length:4;       /* Uncmprssd data type/cmprssn hdr sz */        __u16           size;
};

/* SCB data type */
#define RU_SCB_RAW      00
#define RU_SCB_RESRV    01
#define RU_SCB_MSTR_CHR 10
#define RU_SCB_DUP_CHR  11

struct sna_ru_compress_scb
{
        __u8    type:2,         /* SCB type */
                count:6;        /* Uncompressed size of following data */
};

/* LZ command indicators */
#define LZ_RESET        0x001
#define LZ_FREEZE       0x002
#define LZ_UNFREEZE     0x003

/* LZ control sequence header */
struct sna_lz_9bit
{
        __u16   cntl:9,         /* 9 bit LZ compression */
                cmd:7;          /* LZ command */
};

struct sna_lz_10bit
{
        __u16   cntl:10,        /* 10 bit LZ compression */
                cmd:6;          /* LZ command */
};

struct sna_lz_12bit
{
        __u16   cntl:12,        /* 12 but LZ compression  */
                cmd:4;          /* LZ command */
};

struct sna_ru_compress_lz
{
        union {
                struct sna_lz_9bit      9bit;
                struct sna_lz_10bit     10bit;
                struct sna_lz_12bit     12bit;
        } size;
};
#endif

/* EBCDIC.
 */

static unsigned char const ebcdic_to_rotated[256] = {
          0,   1,   2,   3,   4,   5,   6,   7,         /* 0 -     7 */
          8,   9,  10,  11,  12,  13,  14,  15,         /* 8 -    15 */
         16,  17,  18,  19,  20,  21,  22,  23,         /* 16 -   23 */
         24,  25,  26,  27,  28,  29,  30,  31,         /* 24 -   31 */
         32,  33,  34,  35,  36,  37,  38,  39,         /* 32 -   39 */
         40,  41,  42,  43,  44,  45,  46,  47,         /* 40 -   47 */
         48,  49,  50,  51,  52,  53,  54,  55,         /* 48 -   55 */
         56,  57,  58,  59,  60,  61,  62,  63,         /* 56 -   63 */
         1,  65,  66,  67,  68,  69,  70,  71,          /* 64 -   71 */
         72,  73,  74,  75,  76,  77,  78,  79,         /* 72 -   79 */
         80,  81,  82,  83,  84,  85,  86,  87,         /* 80 -   87 */
         88,  89,  90, 109,  92,  93,  94,  95,         /* 88 -   95 */
         96,  97,  98,  99, 100, 101, 102, 103,         /* 96 -  103 */
        104, 105, 106, 107, 108, 109, 110, 111,         /* 104 - 111 */
        112, 113, 114, 115, 116, 117, 118, 119,         /* 112 - 119 */
        120, 121, 122, 237, 241, 125, 126, 127,         /* 120 - 127 */
        128, 129, 130, 131, 132, 133, 134, 135,         /* 128 - 135 */
        136, 137, 138, 139, 140, 141, 142, 143,         /* 136 - 143 */
        144, 145, 146, 147, 148, 149, 150, 151,         /* 144 - 151 */
        152, 153, 154, 155, 156, 157, 158, 159,         /* 152 - 159 */
        160, 161, 162, 163, 164, 165, 166, 167,         /* 160 - 167 */
        168, 169, 170, 171, 172, 173, 174, 175,         /* 168 - 175 */
        176, 177, 178, 179, 180, 181, 182, 183,         /* 176 - 183 */
        184, 185, 186, 187, 188, 189, 190, 191,         /* 184 - 191 */
        192,   7,  11,  14,  19,  23,  27,  31,         /* 192 - 199 */
         35,  39, 202, 203, 204, 205, 206, 207,         /* 200 - 207 */
        208,  71,  75,  79,  83,  87,  91,  95,         /* 208 - 215 */
         99, 103, 218, 219, 220, 221, 222, 223,         /* 216 - 223 */
        224, 225, 139, 143, 147, 151, 155, 159,         /* 224 - 231 */
        163, 167, 234, 235, 236, 237, 238, 239,         /* 232 - 239 */
        195, 199, 203, 207, 211, 215, 219, 223,         /* 240 - 247 */
        227, 231, 250, 251, 252, 253, 254, 255          /* 248 - 255 */
};

static unsigned char const ebcdic_to_ascii_sna[256] =
{
          0,   1,   2,   3,   4,   5,   6,   7,         /* 0 -     7 */
          8,   9,  10,  11,  12,  13,  14,  15,         /* 8 -    15 */
         16,  17,  18,  19,  20,  21,  22,  23,         /* 16 -   23 */
         24,  25,  26,  27,  28,  29,  30,  31,
         32,  33,  34,  35,  36,  37,  38,  39,
         40,  41,  42,  43,  44,  45,  46,  47,
         48,  49,  50,  51,  52,  53,  54,  55,
         56,  57,  58,  59,  60,  61,  62,  63,
         32,  65,  66,  67,  68,  69,  70,  71,
         72,  73,  74,  75,  76,  77,  78,  79,
         80,  81,  82,  83,  84,  85,  86,  87,
         88,  89,  90,  91,  92,  93,  94,  95,
         96,  97,  98,  99, 100, 101, 102, 103,
        104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119,
        120, 121, 122, 123, 124, 125, 126, 127,
        128, 129, 130, 131, 132, 133, 134, 135,
        136, 137, 138, 139, 140, 141, 142, 143,
        144, 145, 146, 147, 148, 149, 150, 151,
        152, 153, 154, 155, 156, 157, 158, 159,
        160, 161, 162, 163, 164, 165, 166, 167,
        168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183,
        184, 185, 186, 187, 188, 189, 190, 191,
        192,  65,  66,  67,  68,  69,  70,  71,
         72,  73, 202, 203, 204, 205, 206, 207,
        208,  74,  75,  76,  77,  78,  79,  80,
         81,  82, 218, 219, 220, 221, 222, 223,
        224, 225,  83,  84,  85,  86,  87,  88,
         89,  90, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 247,
        248, 249, 250, 251, 252, 253, 254, 255
};

static unsigned char const ascii_to_ebcdic_sna[256] =
{
          0,   1,   2,   3,  55,  45,  46,  47,         /* 0  -    7 */
         22,   5,  37,  11,  12,  13,  14,  15,         /* 8  -   15 */
         16,   0,   0,   0,   0,  61,  50,  38,         /* 16 -   23 */
         24,  25,  63,  39,   0,   0,   0,   0,         /* 24 -   31 */
         64,   0, 127, 123,  80, 108,  91, 121,         /* 32 -   39 */
         77,  93,  53,  78, 107,  96,  75,  97,         /* 40 -   47 */
        240, 241, 242, 243, 244, 245, 246, 247,         /* 48 -   55 */
        248, 249, 122,  94,  76, 126, 110, 111,         /* 56 -   63 */
        124, 193, 194, 195, 196, 197, 198, 199,         /* 64 -   71 */
        200, 201, 209, 210, 211, 212, 213, 214,         /* 72 -   79 */
        215, 216, 217, 226, 227, 228, 229, 230,         /* 80 -   87 */
        231, 232, 233,  74, 224,   0,  95, 109,         /* 88 -   95 */
        125, 129, 130, 131, 132, 133, 134, 135,         /* 96 -  103 */
        136, 137, 145, 146, 147, 148, 149, 150,         /* 104 - 111 */
        151, 152, 153, 162, 163, 164, 165, 166,         /* 112 - 119 */
        167, 168, 169, 192, 106, 208, 161,   7,         /* 120 - 127 */
          0,   0,   0,   0,   0,   0,   0,   0,		/* 128 - 135 */
          0,   0,   0,   0,   0,   0,   0,   0,		/* 136 - 143 */
          0,   0,   0,   0,   0,   0,   0		/* 144 - 150 */
};

/* FM.
 */

/* FMH type indicators */
#define SNA_FMH_TYPE_1			1
#define SNA_FMH_TYPE_2                  2
#define SNA_FMH_TYPE_3			3
#define SNA_FMH_TYPE_4                  4        
#define SNA_FMH_TYPE_5                  5       /* attach. */
#define SNA_FMH_TYPE_6                  6        
#define SNA_FMH_TYPE_7                  7      	/* error. */
#define SNA_FMH_TYPE_10                 10      
#define SNA_FMH_TYPE_12                 12	/* security. */
#define SNA_FMH_TYPE_18			18
#define SNA_FMH_TYPE_19			19

/* simply a structure to make getting the len and type easy. */
#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
        u_int8_t        len                     __attribute__ ((packed));
        u_int8_t        type:7,
                        rsv1:1                  __attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
        u_int8_t        len                     __attribute__ ((packed));
        u_int8_t        rsv1:1,
                        type:7                  __attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_fmh;
#pragma pack()

/* FMH command codes. */
#define SNA_FMH_CMD_ATTACH		0x02FF

/* FMH resource type. */
#define SNA_FMH_RSRC_TYPE_HD_BASIC	0xD0
#define SNA_FMH_RSRC_TYPE_HD_MAPPED	0xD1
#define SNA_FMH_RSRC_TYPE_FD_BASIC	0xD2
#define SNA_FMH_RSRC_TYPE_FD_MAPPED	0xD3

/* FMH concatenation indicators */
#define FMH_CONCAT_TRUE         1
#define FMH_CONCAT_FALSE        0

/* FMH media indicators */
#define FMH_CONSOLE             0000    /* Console */
#define FMH_XCHANGE             0001    /* Exchange */
#define FMH_CARD                0010    /* Card */
#define FMH_DOCUMENT            0011    /* Document */
#define FMH_NXCHANGE_DISK       0100    /* Non-exchange disk */
#define FMH_XDOCUMENT           0101    /* Extended document */
#define FMH_XCARD               0110    /* Extended card */
#define FMH_DATA_SELECT         0111    /* Data set name select destination */
#define FMH_WP_1                1000    /* Word processing media 1 */
#define FMH_WP_2                1001    /* Word processing media 2 */
#define FMH_WP_3                1010    /* Word processing media 3 */
#define FMH_RESRV_1             1011    /* Reserved */
#define FMH_WP_4                1100    /* Word processing media 4 */
#define FMH_RESRV_2             1101    /* Reserved */
#define FMH_RESRV_3             1110    /* Reserved */
#define FMH_RESRV_4             1111    /* Reserved */

/* FMH data stream profile indicators */
#define DSP_DEFAULT             0000    /* Default */
#define DSP_BASE                0001    /* Base */
#define DSP_GENERAL             0010    /* General */
#define DSP_JOB                 0011    /* Job */
#define DSP_WP_RAW              0100    /* WP raw-form text */
#define DSP_WP_XCHANGE_DISK     0101    /* WP exchange diskette */
#define DSP_RESRV_1             0110    /* Reserved */
#define DSP_OII_2               0111    /* Office Info. Interchange level 2 */
#define DSP_RESRV_2             1000    /* Reserved */
#define DSP_RESRV_3             1001    /* Reserved */
#define DSP_DOC_INTERCHANGE     1010    /* Document interchange */
#define DSP_STRUCT              1011    /* Structured field */
#define DSP_RESRV_4             1100    /* Reserved */
#define DSP_RESRV_5             1101    /* Reserved */
#define DSP_RESRV_6             1110    /* Reserved */
#define DSP_RESRV_7             1111    /* Reserved */

/* FMH destination selection indicators */
#define DSSEL_RESUME            000     /* Resume */
#define DSSEL_END               001     /* End */
#define DSSEL_BEGIN             010     /* Begin */
#define DSSEL_BEGIN_END         011     /* Begin/end */
#define DSSEL_SUSPEND           100     /* Suspend */
#define DSSEL_END_ABORT         101     /* End-abort */
#define DSSEL_CONTINUE          110     /* Continue */
#define DSSEL_RESRV_1           111     /* Reserved */

/* FMH compaction indicators */
#define FMH_COMPACT_TRUE        1       /* Compaction */
#define FMH_COMPACT_FLASE       0       /* No compaction */

/* SNA Function Managment 1 header */
struct sna_fmh1
{
        __u8    medium:4,       /* Desired medium for data */
                lsubaddr:4;     /* Logical subaddress */
        __u8    sri:1,          /* Stack Reference indicator */
                dmsel:1,        /* Demand select */
                rsv_1:2,        /* Reserved */
                dsp:4;          /* Data stream profiles */
        __u8    dssel:3,        /* Destination selection */
                dst:1,          /* Data set transmission */
                rsv_2:1,        /* Reserved */
                cmi:1,          /* FMH-1 SCB compression indicator */
                cpi:1,          /* Compaction indicator */
                rsv_3:1;        /* Reserved */
        __u8    ecrl;           /* Exchange record length */
        __u16   rsv_4;          /* Reserved */
        __u8    dslen;          /* Length of destination name */

        unsigned char *dsname;  /* Destination name, unlimited length */
};

/* FMH-2 function indicators */
#define FMH2_PDIR               0x01    /* Peripheral data information record */#define FMH2_COMPACT_TABLE      0x02    /* Compaction table */
#define FMH2_SCB_COMP_CHAR      0x04    /* Prime SCB compression character */
#define FMH2_XCUTE_PG_OFFLN     0x07    /* Execute program offline */
#define FMH2_CREATE_DATA_SET    0x20    /* Create data set */
#define FMH2_SCRATCH_DATA_SET   0x21    /* Scratch data set */
#define FMH2_ERASE_DATA_SET     0x22    /* Erase data set */
#define FMH2_PASSWORD           0x23    /* Password */
#define FMH2_ADD                0x24    /* Add */
#define FMH2_REPLACE            0x25    /* Replace */
#define FMH2_ADD_REPLICATE      0x26    /* Add replicate */
#define FMH2_REPLACE_REPLIACTE  0x27    /* Replace replicate */
#define FMH2_QUERY_DATA_SET     0x28    /* Query for data set */
#define FMH2_NOTE               0x29    /* Note */
#define FMH2_REC_ID             0x2B    /* Record ID */
#define FMH2_ERASE_REC          0x2C    /* Erase record */
#define FMH2_SCRATCH_ALL_SETS   0x2D    /* Scratch all data sets */
#define FMH2_VOL_ID             0x2E    /* Volume ID */
#define FMH2_NOTE_REPLY         0xAA    /* Note reply */

/* FMH-3 function indicators */
#define FMH3_COMPACT_TABLE      0x02    /* Compaction table */
#define FMH3_QUERY_COMPACT_TBLE 0x03    /* Query for compaction table */
#define FMH3_SCB_COMP_CHAR      0x04    /* Prime SCB compression character */
#define FMH3_STATUS             0x05    /* Status */
#define FMH3_SERIES_ID          0x06    /* Series ID */

/* SNA Function Managment 2 and 3 header */
struct sna_fmh2
{
        __u8    sri:1,          /* Stack reference indicator */
                fmh2cmd:7;      /* FMH-2 function to be performed */

        unsigned char *fmh2opt; /* Parameter fields, unlimited length */
};

/* FMH-4 block transmission type indicators */
#define FMH4_INHERIT            0x00    /* Inherit code */
/* FMH4 0x01 - 0x3F */                  /* Reserved */
#define FMH4_FFR_FNI            0x40    /* FFR-FNI record */
#define FMH4_FFR_FS             0x41    /* FFR-FS record */
#define FMH4_FFR_FS2            0x42    /* FFR-FS2 record */
/* FMH4 0x43 - 0x4F */                  /* Reserved */
/* FMH4 0x50 - 0xFF */                  /* Reserved */

/* FMH-4 command indicators */
#define FMH4_CRT_NU_BLK         0x00    /* CRT-NU-BLK */
#define FMH4_CRT_SU_BLK         0x02    /* CRT-SU-BLK */
#define FMH4_CRT_SN_BLK         0x03    /* CRT-SN-BLK */
#define FMH4_CONT_NU_BLK        0x10    /* CONT-NU-BLK */
#define FMH4_CONT_SU_BLK        0x12    /* CONT-SU-BLK */
#define FMH4_CONT_SN_BLK        0x13    /* CONT-SN-BLK */
#define FMH4_CONT_NU_BLK        0x10    /* CONT-NU-BLK */
#define FMH4_CONT_SU_BLK        0x12    /* CONT-SU-BLK */
#define FMH4_CONT_SN_BLK        0x13    /* CONT-SN-BLK */
#define FMH4_DEL_SN_BLK         0x23    /* DEL-SN-BLK */
#define FMH4_UPD_SU_BLK         0x32    /* UPD-SU-BLK */
#define FMH4_UPD_SN_BLK         0x33    /* UPD-SN-BLK */
#define FMH4_RPL_SU_BLK         0x42    /* RPL-SU-BLK */
#define FMH4_RPL_SN_BLK         0x43    /* RPL-SN-BLK */

/* SNA Function Managment 4 header */
struct sna_fmh4
{
        __u8    fmh4fxct;       /* Length of fixed length parameters */
        __u8    fmh4tt1;        /* Block transmission type */
        __u8    fm4htt2;        /* Block transmission type qualifier */
        __u8    fmh4cmd;        /* Command */
        __u8    rsv1:2,         /* Reserved */
                f4rdescr:2,     /* Record descriptor flag */
                rsv2:2,         /* Reserved */
                fmh4bdtf:1,     /* Block data transform flag */
                fmh4rdtf:1;     /* Reserved */
        __u8    fmh4lbn;        /* Length of FMH4BN */

        __u8    *fmh4bn;        /* Name of block */

        __u8    fmh4lbdt;       /* Length of FMH4BDT */

        unsigned char fmh4bdt;  /* Block data transform */

        __u8    fmh4lvid;       /* Length of FMH4VID */

        unsigned char fmh4vid;  /* Version identifier */
};

struct fmh5_sec_access
{
        __u8    length;         /* Length */
        __u8    type;           /* Subfield type */
        unsigned char data;     /* Security data about receiver */
};

/* PIP header */
struct piph
{
        __u16   length;
        __u16   gds;            /* GDS indicator */

        /* PIP sub-fields of structure piph, unlimited length */
        unsigned char sub_pip;
};

/* SNA Function Managment 5 header ATTACH (LU 6.2) */
#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	len			__attribute__ ((packed));
	u_int8_t	type:7,
			rsv1:1			__attribute__ ((packed));
	u_int16_t	cmd			__attribute__ ((packed));
	u_int8_t	rsv2:2,
			xaid:1,
			pipi:1,
			spwdi:1,
			pvid:2,
			vid:1			__attribute__ ((packed));
	u_int8_t	fix_len			__attribute__ ((packed));
	u_int8_t	rsrc_type		__attribute__ ((packed));
	u_int8_t	rsv3			__attribute__ ((packed));
	u_int8_t	rsv4:6,
			sync_level:2		__attribute__ ((packed));	
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	len			__attribute__ ((packed));
	u_int8_t	rsv1:1,
			type:7			__attribute__ ((packed));
	u_int16_t	cmd			__attribute__ ((packed));
	u_int8_t	vid:1,
			pvid:2,
			spwdi:1,
			pipi:1,
			xaid:1,
			rsv2:2			__attribute__ ((packed));
	u_int8_t	fix_len			__attribute__ ((packed));
	u_int8_t	rsrc_type		__attribute__ ((packed));
	u_int8_t	rsv3			__attribute__ ((packed));
	u_int8_t	sync_level:2,
			rsv4:6			__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_fmh5;
#pragma pack()

#ifdef NOT_LU62

/* SNA Function Managment 5 header (Not LU 6.2) */
struct sna_fmh5
{
        __u8    length;         /* Length of FMH-5 + Length byte */
        __u8    frag:1,         /* FMH concatenation */
                type:7;         /* FMH type - 0000101 */
        __u8    fmh5cmd;        /* Command code */
        __u8    fmh5mod;        /* Modifier */
        __u8    fmh5fxct;       /* Fixed length parameters */
        __u8    attdsp;
        __u8    attdba;

        unsigned char *names;   /* Resource names, unlimited length */
};

#endif  /* CONFIG_SNA_LU62 */

/* SNA Function Managment 6 header */
struct sna_fmh6
{
        __u8    length;         /* Length of FMH-6 + Length byte */
        __u8    frag:1,         /* FMH concatenation */
                type:7;         /* FMH type - 0000110 */
        __u8    fmh6cmd;        /* Command code */
        __u8    fmh6lnsz:1,     /* Length of parameter length fields */
                rsv1:7;         /* Reserved */

        /* Many variable lenth params */
};

/* SNA Function Managment 7 header ERROR (LU 6.2) */
#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	len		__attribute__ ((packed));
	u_int8_t	type:7,
			rsv1:1		__attribute__ ((packed));
	u_int32_t	sense		__attribute__ ((packed));
	u_int8_t	rsv2:7,
			logi:1		__attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	len		__attribute__ ((packed));
	u_int8_t	rsv1:1,
			type:7		__attribute__ ((packed));
	u_int32_t	sense		__attribute__ ((packed));
	u_int8_t	logi:1,
			rsv2:7		__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_fmh7;
#pragma pack()


/* SNA Function Managment 8 header (LU 6.1) */
struct sna_fmh8
{
        __u8    *raw;
};

/* SNA Function Managment 10 header */
struct sna_fmh10
{
        __u8    length;         /* Length of FMH-10 + Length byte */
        __u8    frag:1,         /* FMH concatenation */
                type:7;         /* FMH type - 0001010 */
        __u16   spccmd;         /* Sync point command */
        __u16   spcmod;         /* Sync point modifier */
};

/* SNA Function Managment 12 header SECURITY (LU 6.2) */
struct sna_fmh12
{
        __u8    length;         /* Length of FMH-12 + Length byte */
        __u8    rsv1:1,         /* Reserved */
                type:7;         /* FMH type - 0001100 */

        unsigned char fmh12mac[7];      /* DES Message Authentication Code */
};

/* GDS.
 */

#define SNA_GDS_NULL_DATA	0x12F1
#define SNA_GDS_APP_DATA	0x12FF

/*
 * SNA GDS (Global Data Stream) definitions
 *
 * Identifer registery
 * 0000 - 01FF          3270
 * 03xx                 3270
 * 06xx                 3270
 * 09xx                 3270
 * 0B00 - 0EFF          3270
 * 0Fxx                 3270
 * 101x                 3270
 * 1030 - 1034          Print Job Restart
 * 1058 - 105B          Workstation Platform/2
 * 1100 - 1104          SNA Character String
 * 12xx                 LU 6.2 and APPN
 * 13xx                 SNA/Management Services
 * 140x                 3820 Page Priner
 * 1500                 Dependent LU Requester/Server
 * 1501                 Subarea Routing Services
 * 1520                 DLSw Capabilities eXchange
 * 1521                 DLSw Capabilities eXchange Positive Response
 * 1522                 DLSw Capabilities eXchange Negative Response
 * 1530 - 1531          SNA File Services
 * 1532                 SNA Condition Report
 * 1533 - 154F          SNA File Services
 * 1550 - 155F          SNA File Services
 * 1570 - 158F          SNA/Distribution Services
 * 4000 - 41FF          3270
 * 4A00 - 4CFF          3270
 * 71xx                 3250
 * 8000 - 81FF          3270
 * C00x                 Document Interchange Architecture
 * C100 - C104          Document Interchange Architecture
 * C105                 SNA/Distribution Services
 * C10A - C122          Document Interchange Architecture
 * C123 - C124          SNA/Distribution Services
 * C219                 Document Interchange Architecture
 * C300 - C345          Document Interchange Architecture
 * C350 - C361          SNA/Distribution Services
 * C366 - C46F          Document Interchange Architecture
 * C500 - C56F          Document Interchange Architecture
 * C600 - C66F          Document Interchange Architecture
 * C7xx                 Graphical Display Data Manager
 * C800 - C87F          Document Interchange Architecture
 * C900 - CB0F          Document Interchange Architecture
 * CC00 - CC3F          Document Interchange Architecture
 * CF0x                 Document Interchange Architecture
 * D0xx                 Distributed Data Management
 * D3xx                 Document Content Architecture
 * D6xx                 Intelligent Printer Data Stream
 * D780 - D7BF          Facsimile Architecture
 * D820 - D821          AS/400 (5250)
 * D822 - D826          AS/400 (5394)
 * D930 - D95F          AS/400 (5250)
 * E10x                 Level-3 Document Content Architecture
 * E20x                 Level-3 Document Content Architecture
 * E30x                 Level-3 Document Content Architecture
 * E40x                 Level-3 Document Content Architecture
 * E50x                 Level-3 Document Content Architecture
 * E60x                 Level-3 Document Content Architecture
 * E70x                 Level-3 Document Content Architecture
 * E80x                 Level-3 Document Content Architecture
 * E90x                 Level-3 Document Content Architecture
 * EA0x                 Level-3 Document Content Architecture
 * EFFF                 IBM TokenRing Network PC Adapter
 * F000 - FEFF          Non-IBM Reserve Block
 * FFxx                 Context-Dependent Block
 */

/*
 * SNA Service Transaction Programs (STPs)
 */
#define SNA_GDS_CHG_NUM_SESS            0x1210
typedef struct {
        __u8    rsv1:4,
                service_flag:4;
        __u8    reply_mod;
        __u8    action;
        __u8    rsv2:3,
                src_lu_drain:1,
                rsv3:3,
                dst_lu_drain:1;
        __u8    rsv4:7,
                sess_deact_resp:1;
        __u16   rsv5:1,
                max_sess_cnt:15;
        __u16   rsv6:1,
                src_g_min_cont_win:15;
        __u16   rsv7:1,
                dst_g_min_cont_win:15;
        __u8    rsv8:7,
                affect_mode_names:1;
        __u8    mode_name_len;
        __u8    mode_name;                      /* Start of Mode Name */
} gds_chg_num_sess;

struct sna_gds {
        __u16   cvlen:16;
        __u16   cvid:16;

        union {

        } cv;
};

/* HPR.
 */

/* Switching mode defines */
#define HPR_FUNCT_RT    101
#define HPR_ANR         110

/* NLP Time-Sensitivity */
#define NLP_NOT_TIME_SENSE      0
#define NLP_TIME_SENSE          1

/* Congestion defines */
#define NLP_SLOW1_TRUE          1
#define NLP_SLOW1_FLASE         0
#define NLP_SLOW2_TRUE          1
#define NLP_SLOW2_FLASE         0

/* HPR Network Layer Header (NHDR) */
struct sna_nhdr
{
        __u8    sm:3,           /* Switching mode */
                __pad1:2,
                tpf:2,          /* Transmission priority field */
                __pad2:1;
        __u8    ftype:4,        /* Function type */
                tsdata:1,       /* Time-sensitive packet indicator */
                slow1:1,        /* Slowdown (minor) indicator */
                slow2:1,        /* Slowdown (significant) indicator */
                __pad3:1;

        /* Variable length data fields */
};

/* PS.
 */

/* Flag structures for PS10 header */
struct psh10_prepare
{
        __u8    luname:1,       /* LU names indicator */
                wait:1,         /* Wait for outcome indicator */
                __pad1:3,
                locks:1,        /* LOCKS parameter indicator */
                __pad2:2;
};

struct psh10_rqcommit
{
        __u8    luwid:1,        /* Support of the New LUWID PS header */
                wait:1,         /* Wait for outcome indicator */
                reliable:1,     /* Resource reliability indicator */
                sprq:1,         /* OK to leave out sync-point request */
                rreport:1,      /* Initiator read-only reporting */
                locks:1,        /* LOCKS parameter indicator */
                __pad1:2;
};

struct psh10_committed
{
        __u8    __pad1:1,
                resync:1,       /* Resync processing status */
                sluwid:1,       /* Source of next LUWID */
                sprq:1,         /* OK to leave out sync-point request */
                ifexcpt:1,      /* Implied Forget expectation indicator */
                __pad2:3;
};

struct psh10_forget
{
        __u8    luwid:1,        /* Support of the New LUWID PS header */
                resync:1,       /* Resync processing status */
                __pad1:6;
};

struct psh10_hm
{
        __u8    __pad1:2,
                sluwid:1,       /* Source of next LUWID */
                __pad2:5;
};

struct psh10_nluwid
{
        __u8    __pad1;
};

/* PS10 header */
struct sna_psh10
{
        __u8    length;         /* Length */

        __u8    rsv:1,
                type:7;

        union {
                struct psh10_prepare    prepare;
                struct psh10_rqcommit   rqcommit;
                struct psh10_committed  committed;
                struct psh10_forget     forget;
                struct psh10_hm         hm;
                struct psh10_nluwid     nluwid;
        } flags;

        __u8    spptype;        /* Sync-point command type */

        /* Command specific data here */

        __u16   flowmod;        /* Modifier specifying next flow */

        unsigned char sndlen[7];        /* Number of bytes sent */

        /* All kinds of variable length structures */
};

/* TH.
 */

#define SNA_TH_FID0     	0x0
#define SNA_TH_FID1     	0x1
#define SNA_TH_FID2     	0x2
#define SNA_TH_FID3     	0x3
#define SNA_TH_FID4     	0x4
#define SNA_TH_FID5     	0x5
#define SNA_TH_FIDF     	0xF

#define SNA_TH_MPF_BBIU         0x2     /* 10 */
#define SNA_TH_MPF_EBIU         0x1     /* 01 */
#define SNA_TH_MPF_WHOLE_BIU    0x3     /* 11 */
#define SNA_TH_MPF_NO_BIU       0x0     /* 00 */
#define SNA_TH_MPF_MID_MIU      SNA_TH_MPF_NO_BIU

#define SNA_TH_EFI_NORM         0x0
#define SNA_TH_EFI_EXP          0x1

#define GET_SNF_REQUEST(snf)    (((u_int16_t)snf) & 0x7FFF)
#define GET_SNF_SENDER(snf)     (((u_int16_t)snf) >> 15)

#define SET_SNF_REQUEST(snf)    (((u_int16_t)snf))
#define SET_SNF_SENDER(snf)     (((u_int16_t)snf) << 15)

/* FID0 and FID1 formats are used between adjacent subarea nodes when
 * either or both nodes do not support ER and VR protocols.
 */
#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	efi:1,
			rsv1:1,
			mpf:2,
			format:4	__attribute__ ((packed));
	u_int8_t        rsv2            __attribute__ ((packed));
        u_int16_t       daf             __attribute__ ((packed));
        u_int16_t       oaf             __attribute__ ((packed));
       	u_int16_t       snf             __attribute__ ((packed));      
        u_int16_t       dcf             __attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	format:4,
        		mpf:2,
                	rsv1:1,
                 	efi:1		__attribute__ ((packed));
        u_int8_t    	rsv2		__attribute__ ((packed));
        u_int16_t   	daf		__attribute__ ((packed));
        u_int16_t	oaf		__attribute__ ((packed));
        u_int16_t	snf		__attribute__ ((packed));
        u_int16_t   	dcf		__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_fid0;
#pragma pack()

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	efi:1,
			odai:1,
			mpf:2,
			format:4	__attribute__ ((packed));
        u_int8_t        rsv1            __attribute__ ((packed));
        u_int8_t        daf             __attribute__ ((packed));
        u_int8_t        oaf             __attribute__ ((packed));
        u_int16_t       snf             __attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	format:4,
			mpf:2,
			odai:1,
			efi:1		__attribute__ ((packed));
        u_int8_t	rsv1    	__attribute__ ((packed));
        u_int8_t	daf     	__attribute__ ((packed));
        u_int8_t	oaf     	__attribute__ ((packed));
	u_int16_t	snf		__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_fid2;
#pragma pack()

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	efi:1,
			rsv1:1,
			mpf:2,
			format:4	__attribute__ ((packed));
	u_int8_t	lsid		__attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	format:4,
			mpf:2,
			rsv1:1,
			efi:1		__attribute__ ((packed));
	u_int8_t	lsid		__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_fid3;
#pragma pack()

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	ntwk_prty:1,
			vr_pac_cnt_ind:1,
			er_vr_supp_ind:1,
			tg_sweep:1,
			format:4	__attribute__ ((packed));
	u_int8_t	piubf:2,
			hft:2,
			rsv1:2,
			tgsf:2		__attribute__ ((packed));
	u_int8_t	ern:4,
			nlp_cnt:3,
			nlpoi:1		__attribute__ ((packed));
	u_int8_t	tpf:2,
			rsv2:2,
			vrn:4		__attribute__ ((packed));
	u_int16_t	tg_snf:12,
			vr_sqti:2,
			tg_nonfifo_ind:1,
			vr_cwi:1	__attribute__ ((packed));
	u_int16_t	vr_snf_send:12,
			vr_rwi:1,
			vr_cwri:1,
			vrprs:1,
			vrprp:1		__attribute__ ((packed));
        u_int32_t       dsaf            __attribute__ ((packed));
        u_int32_t       osaf            __attribute__ ((packed));
	u_int8_t	efi:1,
			rsv4:1,
			mpf:2,
			snai:1,
			rsv3:3		__attribute__ ((packed));
        u_int8_t        rsv5            __attribute__ ((packed));
        u_int16_t       def             __attribute__ ((packed));
        u_int16_t       oef             __attribute__ ((packed));
        u_int16_t       snf             __attribute__ ((packed));
        u_int16_t       dcf             __attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	format:4,
			tg_sweep:1,
			er_vr_supp_ind:1,
                	vr_pac_cnt_ind:1,
                	ntwk_prty:1	__attribute__ ((packed));
        u_int8_t	tgsf:2,
                	rsv1:2,
                	hft:2,
                	piubf:2		__attribute__ ((packed));
        u_int8_t	nlpoi:1,
                	nlp_cnt:3,
                	ern:4		__attribute__ ((packed));
        u_int8_t	vrn:4,
                	rsv2:2,
                	tpf:2		__attribute__ ((packed));
        u_int16_t	vr_cwi:1,
                	tg_nonfifo_ind:1,
                	vr_sqti:2,
                	tg_snf:12	__attribute__ ((packed));
        u_int16_t	vrprq:1,
                	vrprs:1,
                	vr_cwri:1,
                	vr_rwi:1,
                	vr_snf_send:12	__attribute__ ((packed));
        u_int32_t	dsaf		__attribute__ ((packed));
        u_int32_t	osaf		__attribute__ ((packed));
        u_int8_t    	rsv3:3,
                	snai:1,
                	mpf:2,
                	rsv4:1,
                	efi:1		__attribute__ ((packed));
        u_int8_t	rsv5		__attribute__ ((packed));
        u_int16_t	def		__attribute__ ((packed));
        u_int16_t	oef		__attribute__ ((packed));
        u_int16_t	snf		__attribute__ ((packed));
        u_int16_t	dcf		__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_fid4;
#pragma pack()

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	efi:1,
			rsv1:1,
			mpf:2,
			format:4	__attribute__ ((packed));
	u_int8_t        rsv2            __attribute__ ((packed));
        u_int16_t       snf             __attribute__ ((packed));
        u_int8_t        sa[8]           __attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	format:4,
			mpf:2,
			rsv1:1,
			efi:1		__attribute__ ((packed));
	u_int8_t	rsv2		__attribute__ ((packed));
	u_int16_t	snf		__attribute__ ((packed));
	u_int8_t	sa[8]		__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_fid5;
#pragma pack()

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	rsv1:4,
			format:4	__attribute__ ((packed));
        u_int8_t        rsv2             __attribute__ ((packed));
        u_int8_t        c_format         __attribute__ ((packed));
        u_int8_t        c_type           __attribute__ ((packed));
        u_int16_t       c_snf            __attribute__ ((packed));
        u_int8_t        rsv3[18]         __attribute__ ((packed));
        u_int16_t       dcf              __attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	format:4,
			rsv1:4		 __attribute__ ((packed));
	u_int8_t	rsv2		 __attribute__ ((packed));
	u_int8_t	c_format	 __attribute__ ((packed));
	u_int8_t	c_type		 __attribute__ ((packed));
	u_int16_t	c_snf		 __attribute__ ((packed));
        u_int8_t	rsv3[18]	 __attribute__ ((packed));
        u_int16_t	dcf		 __attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_fidf;
#pragma pack()

/* RH.
 */

#define SNA_RH_RRI_REQ          0
#define SNA_RH_RRI_RSP          1

#define SNA_RH_RU_FMD           0 
#define SNA_RH_RU_NC            1 
#define SNA_RH_RU_DFC           2 
#define SNA_RH_RU_SC            3 

#define SNA_RH_FI_NO_FMH        0
#define SNA_RH_FI_FMH           1

#define SNA_RH_SDI_NO_SD        0
#define SNA_RH_SDI_SD           1

#define SNA_RH_BCI_NO_BC        0
#define SNA_RH_BCI_BC           1

#define SNA_RH_ECI_NO_EC        0
#define SNA_RH_ECI_EC           1

#define SNA_RH_DR1I_NO_DR1      0
#define SNA_RH_DR1I_DR1         1

#define SNA_RH_LLCI_NO_LLC      0
#define SNA_RH_LLCI_LLC         1

#define SNA_RH_DR2I_NO_DR2      0
#define SNA_RH_DR2I_DR2         1

#define SNA_RH_ERI_OFF		0
#define SNA_RH_ERI_ON		1

#define SNA_RH_RTI_POS          0
#define SNA_RH_RTI_NEG          1

#define SNA_RH_RLWI_NO_RLW      0
#define SNA_RH_RLWI_RLW         1

#define SNA_RH_QRI_NO_QR        0
#define SNA_RH_QRI_QR           1

#define SNA_RH_PI_NO_PAC        0
#define SNA_RH_PI_PAC           1

#define SNA_RH_BBI_NO_BB        0
#define SNA_RH_BBI_BB           1

#define SNA_RH_EBI_NO_EB        0
#define SNA_RH_EBI_EB           1

#define SNA_RH_CDI_NO_CD        0
#define SNA_RH_CDI_CD           1

#define SNA_RH_CSI_CODE0        0
#define SNA_RH_CSI_CODE1        1

#define SNA_RH_EDI_NO_ED        0
#define SNA_RH_EDI_ED           1

#define SNA_RH_PDI_NO_PD        0
#define SNA_RH_PDI_PD           1

#define SNA_RH_CEBI_NO_CEB      0
#define SNA_RH_CEBI_CEB         1

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	eci:1,
			bci:1,
			sdi:1,
			fi:1,
			rsv1:1,
			ru:2,
			rri:1		__attribute__ ((packed));
	u_int8_t	pi:1,
			qri:1,
			rlwi:1,
			rsv2:1,
			rti:1,
			dr2i:1,
			llci:1,
			dr1i:1		__attribute__ ((packed));
	u_int8_t	cebi:1,
			pdi:1,
			edi:1,
			csi:1,
			rsv3:1,
			cdi:1,
			ebi:1,
			bbi:1		__attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	rri:1,
			ru:2,
			rsv1:1,
			fi:1,
			sdi:1,
			bci:1,
			eci:1		__attribute__ ((packed));
	u_int8_t	dr1i:1,
			llci:1,
			dr2i:1,
			rti:1,
			rsv2:1,
			rlwi:1,
			qri:1,
			pi:1		__attribute__ ((packed));
	u_int8_t	bbi:1,
       			ebi:1,
			cdi:1,
			rsv3:1,
			csi:1,
			edi:1,
			pdi:1,
			cebi:1		__attribute__ ((packed));   
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_rh;
#pragma pack()

#define SNA_IPM_TYPE_SOLICIT	0
#define SNA_IPM_TYPE_UNSOLICIT	1
#define SNA_IPM_TYPE_RESET	2

#define SNA_IPM_RWI_OFF		0
#define SNA_IPM_RWI_ON		1

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	rsv0:5,
			rwi:1,
			type:2		__attribute__ ((packed));
	u_int16_t       nws             __attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	type:2,
			rwi:1,
    			rsv0:5		__attribute__ ((packed));
	u_int16_t	nws		__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_ipm;
#pragma pack()

/* RU.
 */

/* ABCONN (Abandon Connection) */
struct sna_abconn {
        __u16 ena;
};

/* ABCONNOUT (Abandon Connect Out) */
struct sna_abconnout {
        __u16 ena;
};

/* Type of activation indicators */
#define COLD    0x1
#define ERP     0x2

struct sna_sscp_id {
        __u8    format:4,
                pu_type:4;
        char    id[4];
};

/* ACTDRM (Activate Cross-Domain Resource Manager) */
struct sna_actcdrm {
        __u8    request;        /* 0x14 */
        __u8    format:4,
                type_activation:4;
        __u8    fm_profile;
        __u8    ts_profile;
        char    contents_id[7];
        struct  sna_sscp_id *sscp_id;
        __u8    __pad1:2,
                primary_hs_rcv_win_size:6;
};

/* ACTCONNIN (Activate Conect In) */
struct sna_actconnin {
        /* 0-2 NS header */
        __u16   ena;
        __u8    incoming_call:1,
                info_rq:1,
                __pad1:6;
};

/* ACTLINK (Activate Link) */
struct sna_actlink {
        /* 0-2 NS header */
        __u16   ena;
        __u8    switched_subarea:1,
                sub_llink:1,
                __pad1:6;
};

/* ACTLU (Activate Logical Unit) */
struct sna_actlu {
        __u8    request_code;
        __u8    eami:1,         /* Enhanced Addr. Management Indicator */
                sdai:1,         /* Static/dynamic addr. indicator */
                __pad1:4,
                type_activation:2;
        __u8    fm_profile:4,
                ts_profile:4;
};

/* ACTPU (Activate Physical Unit) */
struct sna_actpu {
        __u8    request_code;
        __u8    format:4,
                type_activation:4;
        __u8    fm_profile:4,
                ts_profile:4;
        struct sna_sscp_id *sscp_id;
};

/* ACTTRACE (Activate Trace) */
struct sna_trace_bit {
        __u8    tx_group:1,
                piu:1,
                __pad1:2,
                scanner_internal:1,
                __pad2:1,
                all_frames:1,
                link:1;
};

struct sna_acttrace {
        /* 0-2 NS header */
        __u16   ena;
        struct sna_trace_bit *trace;

        /* Some trace specific trailers */
};

/* ADDLINK (Add Link) */
struct sna_addlink {
        /* 0-2 NS header */
        __u16   ena;
        __u16   __pad1;
        __u8    size_llink_id;
        /* 8-n llink ID */
};

/* ADDLINKSTA (Add Link Station) */
struct sna_fid_bit {
        __u8    fid0:1,
                fid1:1,
                fid2:1,
                fid3:1,
                fid4:1,
                __pad1:3;
};

struct sna_addlinksta {
        /* 0-2 NS header */
        __u16   ena;
        struct  sna_fid_bit *fid;
        __u8    __pad1;
        __u8    size_linksta_id;
        /* 8-n Link station ID */
};

/* ANA (Assign Network Addresses) */
struct sna_ana {
        /* Retired */
};

/* BFCINIT (BF Control Initiate) */
struct sna_bfcinit {
        /* 0-2 NS header */
        __u16   ena;
        __u8    format:4,
                __pad1:4;
        __u8    __pad2:1,
                sub_source:1,
                save_rscv:1,
                copy_rscv:1,
                sub_names:2,
                ext_bind:1,
                __pad3:1;
        char    __pad4[4];
        __u16   size_bind_image;
        /* Bind Image */
        /* Other variable length goodies */
};

/* BFCLEANUP (BF Cleanup) */
struct sna_bfcleanup {
        /* 0-2 NS header, 0x812629 */
        __u16   ena;
        __u8    format:4,
                __pad1:4;
        __u16   __pad2;

        /* Session keys */
        /* Control Vectors */
};

/* BFINIT (BF Initiate) */
struct sna_bfinit {
        /* 0-2 NS header, 0x812681 */
        __u16   ena;
        __u8    format:4,
                __pad1:4;
        __u16   size_bind_image;

        /* Session Keys */
        /* Control Vectors */
};

/* BFSESSEND (BF Session Ended) */
struct sna_bfsessend {
        /* 0-2 NS header, 0x812688 */
        __u16   ena;
        __u8    format:4,
                lu_role:1,
                __pad1:4;
        __u8    cause;
        __u8    __pad2;

        /* Session Keys */
        /* Control Vectors */
};

/* BFSESSINFO (BF Session Information) */
struct sna_bfsessinfo {
        /* 0-2 NS header, 0x81268C */
        __u16   ena;
        __u8    format:4,
                __pad1:4;
        __u8    als_takeover:1,
                lu_takeover:1,
                auth_lu:1,
                sdai:1,
                static_lu_addr_status:1,
                __pad2:3;
        __u16   __pad3;
        __u8    size_lu_name;

        /* Network Qualified LU name */
        /* Control Vectors */
};

/* BFSESSST (BF Session Started) */
struct sna_bfsessst {
        /* 0-2 NS header, 0x812686 */
        __u16   ena;
        __u8    format:4,
                __pad4;

        /* Session Keys */
        /* Control Vectors */
};

/* BFTERM (BF Terminate) */
struct sna_bfterm {
        /* 0-2 NS header, 0x812683 */
        __u16   ena;
        __u8    format:4,
                __pad1:4;
        __u8    cause;
        __u8    __pad2;

        /* Session Keys */
        /* Control Vectors */
};

/* BID (BID) Not for LU6.2 */
struct sna_nonlu62_bid {
        __u8    request_code;
};

#define SNA_USER_DATA_MODE		0x02
#define SNA_USER_DATA_SESSION_INSTANCE	0x03
#define SNA_USER_DATA_PLU_NETWORK_NAME	0x04
#define SNA_USER_DATA_SLU_NETWORK_NAME	0x05
#define SNA_USER_RANDOM_DATA		0x11
#define SNA_USER_SECURITY_REPLY		0x12
#define SNA_USER_DATA_NONCE		0x13
#define SNA_USER_SECURITY_MECH		0x14

/* BIND (BIND Session) */
#define SNA_BIND_TYPE_NEG		0
#define SNA_BIND_TYPE_NONNEG		1

#define SNA_FM_PROFILE_2		2
#define SNA_FM_PROFILE_3		3
#define SNA_FM_PROFILE_4		4
#define SNA_FM_PROFILE_7		7
#define SNA_FM_PROFILE_18		18
#define SNA_FM_PROFILE_19		19

#define SNA_TS_PROFILE_2		2
#define SNA_TS_PROFILE_3		3
#define SNA_TS_PROFILE_4		4
#define SNA_TS_PROFILE_7		7

#define SNA_CHAIN_USE_SINGLE		0
#define SNA_CHAIN_USE_MULTI		1

#define SNA_REQ_MODE_IMMEDIATE		0
#define SNA_REQ_MODE_DELAY		1

#define SNA_CHAIN_RSP_NONE		0
#define SNA_CHAIN_RSP_EXCEPTION		1
#define SNA_CHAIN_RSP_DEFINATE		2
#define SNA_CHAIN_RSP_BOTH		3

#define SNA_TX_END_BRACKET_NO		0
#define SNA_TX_END_BRACKET_YES		1

#define SNA_WHOLE_BIU_SEG		0
#define SNA_WHOLE_BIU_NO_SEG		1

#define SNA_FM_HEADER_NOT_ALLOWED	0
#define SNA_FM_HEADER_ALLOWED		1

#define SNA_FLOW_MODE_FULL_DUPLEX	0
#define SNA_FLOW_MODE_HALF_DUPLEX_FF	2

#define CV_COS_TPF		0x2C
#define CV_FQ_PCID		0x60
#define CV_ROUTE_SEL		0x2B
#define CV_TG_DESC		0x46
#define CV_TG_ID		0x80

/* BIND (BIND Session). BIND is sent from a primary LU to a seconday LU to
 * activate a session between the LUs. The secondary LU uses the BIND
 * parameters to help determine whether it will response positively or
 * negativly to BIND.
 */
#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	rc			__attribute__ ((packed));
	u_int8_t	type:4,
			format:4		__attribute__ ((packed));
	u_int8_t	fm_profile		__attribute__ ((packed));
	u_int8_t	ts_profile		__attribute__ ((packed));
	u_int8_t	p_tx_end_bracket:1,
			p_compression:1,
			p_rsv1:2,
			p_chain_rsp:2,
			p_req_mode:1,
			p_chain_use:1		__attribute__ ((packed));
	u_int8_t	s_tx_end_bracket:1,
			s_compression:1,
			s_rsv1:2,
			s_chain_rsp:2,
			s_req_mode:1,
			s_chain_use:1		__attribute__ ((packed));
	u_int8_t	bind_queue:1,
			rsv2:2,
			alt_code_set:1,
			bracket_term:1,
			brackets:1,
			fm_header:1,
			whole_biu:1		__attribute__ ((packed));
	u_int8_t	hdx_ff_reset:1,
			ctrl_vectors:1,
			alt_code_proc_id:2,
			contention:1,
			recovery:1,
			flow_mode:2		__attribute__ ((packed));
	u_int8_t	sec_tx_win_size:6,
			rsv3:1,
			sec_stagi:1		__attribute__ ((packed));
	u_int8_t	sec_rx_win_size:6,
			rsv4:1,
			adaptive_pacing:1	__attribute__ ((packed));
	u_int8_t	sec_max_ru_size		__attribute__ ((packed));
	u_int8_t	pri_max_ru_size		__attribute__ ((packed));
	u_int8_t	pri_tx_win_size:6,
			rsv5:1,
			pri_stagi:1		__attribute__ ((packed));
	u_int8_t	pri_rx_win_size:6,
			rsv6:2			__attribute__ ((packed));
	u_int8_t	lu_type:7,
			ps_usage:1		__attribute__ ((packed));
	u_int8_t	lu6_level		__attribute__ ((packed));
	u_int8_t	rsv7[6]			__attribute__ ((packed));
	u_int8_t	rsv8:6,
			xt_security_sense:1,
			xt_security:1		__attribute__ ((packed));
	u_int8_t	persist_verification:1,
			already_verified:1,
			password_sub:1,
			lulu_verification:1,
			access_security:1,
			rsv9:3			__attribute__ ((packed));
	u_int8_t	chg_num_sessions_gds:1,
			parallel_session:1,
			session_reinit:2,
			rsv11:1,
			sync_level:2,
			rsv10:1			__attribute__ ((packed));
	u_int8_t	lcc:2,
			rsv13:4,
			limited_resource:1,
			rsv12:1			__attribute__ ((packed));
	u_int8_t	crypto_len:4,
			crypto_supp:2,
			rsv14:2			__attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	rc			__attribute__ ((packed));
	u_int8_t	format:4,
			type:4			__attribute__ ((packed));
	u_int8_t	fm_profile		__attribute__ ((packed));
	u_int8_t	ts_profile		__attribute__ ((packed));
	u_int8_t	p_chain_use:1,
			p_req_mode:1,
			p_chain_rsp:2,
			p_rsv1:2,
			p_compression:1,
			p_tx_end_bracket:1	__attribute__ ((packed));
	u_int8_t	s_chain_use:1,
                        s_req_mode:1,
                        s_chain_rsp:2,
                        s_rsv1:2,
                        s_compression:1,
                        s_tx_end_bracket:1      __attribute__ ((packed));
	u_int8_t	whole_biu:1,
			fm_header:1,
			brackets:1,
			bracket_term:1,
			alt_code_set:1,
			rsv2:2,
			bind_queue:1		__attribute__ ((packed));
	u_int8_t	flow_mode:2,
			recovery:1,
			contention:1,
			alt_code_proc_id:2,
			ctrl_vectors:1,
			hdx_ff_reset:1		__attribute__ ((packed));
	u_int8_t	sec_stagi:1,
			rsv3:1,
			sec_tx_win_size:6	__attribute__ ((packed));
	u_int8_t	adaptive_pacing:1,
			rsv4:1,
			sec_rx_win_size:6	__attribute__ ((packed));
	u_int8_t	sec_max_ru_size		__attribute__ ((packed));
	u_int8_t	pri_max_ru_size		__attribute__ ((packed));
	u_int8_t	pri_stagi:1,
			rsv5:1,
			pri_tx_win_size:6	__attribute__ ((packed));
	u_int8_t	rsv6:2,
			pri_rx_win_size:6	__attribute__ ((packed));
	u_int8_t	ps_usage:1,
			lu_type:7		__attribute__ ((packed));
	u_int8_t	lu6_level		__attribute__ ((packed));
	u_int8_t	rsv7[6]			__attribute__ ((packed));
	u_int8_t	xt_security:1,
			xt_security_sense:1,
			rsv8:6			__attribute__ ((packed));
	u_int8_t	rsv9:3,
			access_security:1,
			lulu_verification:1,
			password_sub:1,
			already_verified:1,
			persist_verification:1	__attribute__ ((packed));
	u_int8_t	rsv10:1,
			sync_level:2,
			rsv11:1,
			session_reinit:2,
			parallel_session:1,
			chg_num_sessions_gds:1	__attribute__ ((packed));
	u_int8_t	rsv12:1,
			limited_resource:1,
			rsv13:4,
			lcc:2			__attribute__ ((packed));
	u_int8_t	rsv14:2,
			crypto_supp:2,
			crypto_len:4		__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_ru_bind;
#pragma pack()

typedef struct {
        __u8    chain_sel:1,
                request_mode:1,
                chain_rsp_proto:2,
                phase2_sync_point:1,
                __pad1:1,
                scbi:1,
                send_end_bracket:1;
} sna_fm_usage_bit;

struct sna_reason_bit {
        __u8    plu:1,
                bind:1,
                setup_rej_plu:1,
                setup_rej_slu:1,
                __pad1:4;
};

/* CANCEL (Cancel) */
struct sna_cancel {
        __u8    request_code;
};

/* CDCINIT (Cross-Domain Control Initiate) */
struct sna_cdcinit {
        /* 0-2 NS header, 0x81864B */
        __u8    format:4,
                __pad1:4;
        __u8    __pad2:7,
                xrf_bind:1;
        char    pcid[7];

        /* Session pair identifiers */
};

/* CDINIT (Cross-Domain Initiate) */
struct sna_cdinit {
        /* 0-2 NS header, 0x818641 */
        __u8    format:4,
                __pad1:4;

        /* A mess of stuff after here */
};

/* CDSESSEND (Cross-Domain Session Ended) */
struct sna_cdsessend {
        /* 0-2 NS header, 0x818648 */
        char    pcid[7];
        __u8    format:4,
                __pad1:4;

        /* Session Keys */
        /* Control Vectors */
};

/* CDSESSSF (Cross-Domain Session Setup Failure) */
struct sna_cdsesssf {
        /* Retired */
};

/* CDSESSST (Cross-Domain Session Started) */
struct sna_cdsessst {
        /* 0-2 NS header, 0x818646 */
        char    pcid[7];
        __u8    __pad1;

        /* Session Keys */
        /* Control Vectors */
};

/* CDSESSTF (Cross-Domain Session Takedown Failure) */
struct sna_cdsesstf {
        /* Retired */
};

struct sna_cdtaked {
        /* 0-2 NS header, 0x818649 */
        char    pcid[7];
        __u8    type:2,
                takedown:2,
                sscp_sscp_term:1,
                __pad1:3;
        struct  sna_reason_bit *reason;
};

/* CDTAKEDC (Cross-Domain Takedown Complete) */
struct sna_cdtakedc {
        /* 0-2 NS header, 0x81864A */
        char    pcid[7];
        __u8    type;
        __u8    status;
};

/* CDTERM (Cross-Domain Terminate) */
struct sna_cdterm {
        /* 0-2 NS header, 0x818643 */
        __u8    format:4,
                __pad1:4;
        __u8    type:2,
                termination_type:1,
                send_dactlu_to_dlu:1,
                __pad2:1,
                __pad3:2,
                __termination:1;
        char    pcid[7];
        __u8    network:1,
                normal:1,
                reason_code:1,
                sess_setup:1,
                cinit_cterm_err:1,
                bind_unbind_err:1,
                setup_taked_rej:1,
                setup_rej:1;
        __u16   __pad4;

        /* Session Keys */
        /* Control Vectors */
};

/* CHASE (Chase) */
struct sna_chase {
        __u8    request_code;
};

/* CINIT (Control Initiate) */
struct sna_cinit {
        /* 0-2 NS header, 0x810601 */
        __u8    format:4,
                __pad1:4;
        __u8    init_lu_olu:1,
                sub_source:1,
                lu_olu:1,
                __pad2:1,
                name_sub:2,
                ext_bind:1,
                bind_xrf:1;
        char    sess_key[4];
        __u16   size_bind_image;

        /* Lots of variable length crap */
};

/* RU packet type */
#define RU_RQ	0	/* Request Unit */
#define RU_RSP	1	/* Response Unit */

/* RU category */
#define RU_FM_DATA_UNIT	00
#define RU_NC_UNIT	01
#define RU_DFC_UNIT	10
#define RU_SC_UNIT	11

/* RU format indicator */
#define RU_FMH_TRUE	1
#define RU_FMH_FALSE	0

/* RU sense data indicator */
#define RU_SDI_TRUE	1
#define RU_SDI_FALSE	0

/* Begin/End chain indicator */
#define RU_BC_FIRST_TRUE	1
#define RU_BC_FIRST_FALSE	0 
#define RU_EC_LAST_TRUE		1
#define RU_EC_LAST_FALSE	0

/* Definite response 1 indicator */
#define RU_DR1_TRUE	1
#define RU_DR1_FALSE	0

/* Length checked compression indicator */
#define RU_LCC_TRUE	1
#define RU_LCC_FALSE	0

/* Definate response 2 indicator */
#define RU_DR2_TRUE	1
#define RU_DR2_FLASE	0

/* Response type indicators */
#define RU_RT_POSITIVE	0
#define RU_RT_NEGATIVE	1

/* Request larger window indicator */
#define RU_RLW_TRUE	1
#define RU_RLW_FLASE	0

/* Queue response indicators */
#define RU_QR_PASS_TC_QUEUE	0
#define RU_QR_TC_QUEUE		1

/* Pacing indicators */
#define RU_PACE_TRUE	1
#define RU_PACE_FLASE	0

/* Begin bracket indicators */
#define RU_BB_TRUE	1
#define RU_BB_FLASE	0

/* End bracket indicators */
#define RU_EB_TRUE	1
#define RU_EB_FLASE	0

/* Change direction indicators */
#define RU_CD_TRUE	1
#define RU_CD_FALSE	0

/* Code selection indicators */
#define RU_CS_TRUE	1
#define RU_CS_FASE	0

/* Enciphered data indicators */
#define RU_ED_TRUE	1
#define RU_ED_FALSE	0

/* Padded data indicators */
#define RU_PD_TRUE	1
#define RU_PD_FALSE	0

/* Conditional end bracket indicator */
#define RU_CEB_TRUE	1
#define RU_CEB_FALSE	0

/* Session-level crypto indicators. */
#define SNA_RU_CRYPTO_NO	0x0
#define SNA_RU_CRYPTO_SELECT	0x1
#define SNA_RU_CRYPTO_MANDATORY	0x3

struct sna_send_parm {
	__u8	allocate;
	__u8	fmh;
	__u8	type;
	__u8	*data;
};

struct sna_security_reply_2 {
	struct sna_send_parm send_parm;
};

/* ABCONN (Abandon Connection). Request the PU to deactive the link connection
 * for the specified link.
 */
struct sna_ru_abconn {
	__u8	ns_hdr[3];
	__u16	element_addr;
};

/* ACONNOUT (Abandon Connection Out). Requests the PU to terminate a
 * connect-out procedure on the designated link.
 */
struct sna_ru_abconnout {
	__u8	ns_hdr[3];
	__u16	element_addr;
};

/* ACTCDRM (Activate Cross-Domain Resource Manager). ACTCDRM is sent
 * from one SSCP to another SSCP to activate a session between them and
 * to exchange information about the SSCPs.
 */
struct sna_ru_actcdrm {
	__u8	req_code;	/* 0x14 request code. */
	__u8	format:4,	/* Format: 0x0. */
		act_type:4;	/* Type of activation requested:
				 * 0x1 - cold.
				 * 0x2 - ERP.
				 */
	__u8	fm_profile;
	__u8	ts_profile;
	__u8	contents_id[8];
	__u8	sscp_id[6];

	__u8	rsv1:2,
		pri_hs_rcv_window_size:6;

	__u8	*ctrl_vectors;
};

/* ACTCONNIN (Activate Connection In). ACTCONNIN requests the PU to enable
 * the specified link to accept incomming calls. It can also be used to
 * solicit information about an existing connection on the link.
 */
struct sna_ru_actconnin {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	incomming_call:1,
		info_reqi:1,
		rsv1:6;
	__u8	*ctrl_vectors;
};

/* ACTLINK (Activate Link). ACTLINK initiates a procedure at the PU to
 * activate the protocol boundary between a link station in the node (as
 * specified by the link network address parameter in the request) and
 * the link connection attached to it.
 */
struct sna_ru_actlink {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	swsai:1,	/* Switched subarea support indicator. */
		slli:1,		/* Subordinate logical link indicator. */
		rsv1:6;
};

/* ACTLU (Activate Logical Unit). ACTLU is sent from an SSCP to an LU to
 * activate a session betwrrn the SSCP and the LU and to establish common
 * session parameters.
 */
struct sna_ru_actlu {
	__u8	req_code;	/* 0x0D. */
	__u8	eami:1,		/* Enhanced Address Managment indicator. */
		sdai:1,		/* Static/Dynamic Address indicator. */
		rsv1:4,
		act_type:2;	/* Activation type request. */

	__u8	fm_profile:4,
		ts_profile:4;

	__u8	*ctrl_vectors;
};

/* ACTPU (Activate Physical Unit). ACTPU is sent by the SSCP to activate a
 * session with the PU, and to obtain certain information about the PU.
 */
struct sna_ru_actpu {
	__u8	req_code;		/* 0x11. */
	__u8	format:4,
		act_type:4;
	__u8	fm_profile:4,
		ts_profile:4;
	__u8	sscp_id_format:4,	/* 0x0000. */
		sscp_node_type:4;
	__u8	sscp_id_desc[5];
	__u8	*ctrl_vectors;
};

/* ACTTRACE (Activate Trace). ACTTRACE requests the PU to activate a
 * specified type of trace for a specified resource or hierachy of
 * resources for a generalized PIU trace.
 */
struct sna_ru_acttrace {
	__u8	ns_hdr[3];
	__u16	element_addr;

	/* Trace type. */
	__u8	tg_trace:1,	/* Transmission group trace. */
		piu_trace:1,	/* Generalized PIU trace (GPT). */
		rsv1:2,
		scanner_int_trace:1,
		rsv2:1,
		trace_all_frames:1,
		link_trace:1;

	/* Trace specific data. */
};

/* ADDLINK (Add Link). ADDLINK is sent from the SSCP to the PU to obtain a
 * link network address that will be mapped to the locally-used link
 * identifier specified in the request.
 */
struct sna_ru_addlink {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u16	rsv1;
	__u8	llid_len;	/* Local Link ID length. */
	__u8	*llid;		/* Local Link ID. */
};

/* ADDLINKSTA (Add Link Station). ADDLINKSTA is sent from the SSCP to the PU
 * to obtain an adjacent link station network address to be associated with
 * the locally-used link station identifier specified in the request.
 */
struct sna_ru_addlinksta {
	__u8	ns_hdr[3];
	__u16	element_addr;

	/* FID type supported. */
	__u8	fid0:1,
		fid1:1,
		fid2:1,
		fid3:1,
		fid4:1,
		rsv1:3;
	__u8 	rsv2;
	__u8	lsid_len;		/* Link Station ID length. */
	__u8	*lsid;			/* Link Station ID. */
};

/* BFCINIT (BF Control Initiate). BCFINIT requests the BF(LU) to attempt to
 * activate, via a BIND request, a session with the specified SLU.
 */
struct sna_ru_bfcinit {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	format:4,
		rsv1:4;

	__u8	rsv2:1,
		sub_src:1,	/* Substitution source. */
		save_rscv:1,
		copy_rscv:1,
		names_sub:2,
		no_ext_bind:1,
		ext_bind:1;

	__u8	rsv3[5];

	__u16	bind_image_len;

	__u8	*bind_image;
};

/* BFCLEANUP (BF Cleanup). BFCLEANUP is sent with definite response
 * requested to request that the BF(PLU) ot BF(SLU) attempt to deactivate
 * the identified session.
 */
struct sna_ru_bfcleanup {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	format:4,
		rsv1:4;
	__u16	rsv2;
	__u8	*session_key;
	__u8	*ctrl_vectors;
};

/* BFINIT (BF Initiate). BFINIT from the BF(LU) requests the initiation of
 * a session between the two LUs named in the BIND image.
 */
struct sna_ru_bfinit {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	format:4,
		rsv1:4;
	__u16	bind_image_len;
	__u8	*bind_image;
};

/* BFSESSEND (BF Session Ended). BFSESSEND notifies the SSCP that the LU-LU
 * session identified has been deactivated.
 */
struct sna_ru_bfsessend {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	format:4,
		rsv1:4;
	__u8	cause;
	__u8	rsv2;
	__u8	*session_key;
	__u8	*ctrl_vectors;
};

/* BFSESSINFO (BF Session Information). BFSESSINFO provides the SSCP with
 * information about sessions with SSCP-independent LUs in a peripheral
 * node taken over by the receiving SSCP.
 */
struct sna_ru_bfsessinfo {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	format:4,
		rsv1:4;
	__u8	als_takeover:1,
		lu_takeover:1,
		auth_lui:1,		/* Authorized LU indicator. */
		sdai:1,			/* Static/Dynamic address indicator. */
		slu_addr:1,		/* Static LU address. */
		rsv2:2;
	__u16	rsv3;

	__u8	nq_lu_name_len;
	__u8	*nq_lu_name;
	__u8	*ctrl_vectors;
};

/* BFSESST (BF Session Started). BFSESSST informs the SSCP that a new LU-LU
 * session has been activated and provides information about the active
 * session.
 */
struct sna_ru_bfsessst {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	format:4,
		lu_role:1,
		rsv1:3;

	__u8	*session_key;
	__u8	*ctrl_vectors;
};

/* BFTERM (BF Terminate). BFTERM from the BF(LU) requests that the SSCP
 * assist in the termination of the identified LU-LU session.
 */
struct sna_ru_bfterm {
	__u8	ns_hdr[3];
	__u16	element_type;
	__u8	format:4,
		rsv1:4;
	__u8	cause;
	__u8	rsv2;
	__u8	*session_key;
	__u8	*ctrl_vectors;
};

/* BID (BID). BID is used by the bidder to request permission to initiate
 * a bracket, and is used only when using brackets. This RU is not used for
 * LU 6.2.
 */
struct sna_ru_bid {
	__u8	req_code;
};

#define SNA_RU_BIND_ADAPT_NO_SUPP	0x0
#define SNA_RU_BIND_ADAPT_SUPP		0x1

/* BINDF (BIND Failure). BINDF is sent, with no-response requested, by the
 * PLU to notify the SSCP that the attempt to activate the session between
 * the specified LUs has failed.
 */
struct sna_ru_bindf {
	__u8	ns_hdr[3];
	__u32	sense;
	__u8	reason;
	__u8	*session_key;
	__u8	*ctrl_vectors;
};

/* BIS (Bracket Initiation Stopped). BIS is sent by the half-session to
 * indicate that it will not attempt to begin any more brackets.
 */
#pragma pack(1)
typedef struct {
	u_int8_t rc	__attribute__ ((packed));
} sna_ru_bis;
#pragma pack()

/* CANCEL (Cancel). CANCEL may be sent by a half-session to terminate a
 * partially sent chain of FMD requests. CANCEL may be sent only when
 * a chain is in process. The sending half-session may send CANCEL to end
 * a partially sent chain if a negative response is received for a request
 * in the chain, or for some other reason. This RU is not used by LU 6.2.
 */
struct sna_ru_cancel {
	__u8	req_code;
};

/* CDCINIT (Cross-Domain Control Initiate). CDCINIT passes information about
 * the SLU from the SSCP(SLU) to the SSCP(PLU) and requests that the SSCP(PLU)
 * send CINIT to the PLU.
 */
struct sna_ru_cdcinit_format0 {
	__u16	plu_addr;
	__u16	slu_addr;
};

struct sna_ru_cdcinit_format1 {
	__u8	*session_key;
	__u16	bind_image_len;
	__u8	*bind_image;

	__u16	char_field;
	__u8	char_format;
	__u8	*desc;

	__u8	sess_crypto_key_len;
	__u8	*pri_sess_crypto_key;

	__u8	*ctrl_vectors;
};

struct sna_ru_cdcinit {
	__u8	ns_hdr[3];
	__u8	format:4,
		rsv1:4;
	__u8	rsv2:7,
		xrf_slu_supp:1;
	__u8	*sess_pair_id;

	union {
		struct sna_ru_cdcinit_format0 format0;
		struct sna_ru_cdcinit_format1 format1;
	} format1;
};

/* CDINIT (Cross-Domain Initiate). CDINIT requests another SSCP to assist in
 * initiating an LU-LU session for the specified (OLU,DLU) pair.
 */
struct sna_ru_cdinit {
	__u8	*raw;
};

/* CDSESSEND (Cross-Domain Session Ended). CDSESSEND notifies the SSCP that the
 * LU-LU session identified by the session key has been successfully
 * deactivated, or that knowledge of session deactivation has been lost due to
 * session outage with one or more of the participating LUs, gatewat nodes, or
 * adjacent SSCPs. In the latter case, the session may still be active, but
 * explicit notification of session deactivation is no longer possible.
 */
struct sna_ru_cdsessend {
	__u8	ns_hdr[3];
	__u8	fcid[8];
	__u8	format:4,
		rsv1:4;
	__u8	*session_key;
	__u8	*ctrl_vectors;
};

/* CDSESSST (Cross-Domain Session Started). CDSESSST notifies the SSCP(SLU)
 * that the LU-LU session identified by the Session Key content field and
 * the specified PCID for the initiation procedure has been sucessfully
 * activated.
 */
struct sna_ru_cdsessst {
	__u8	ns_hdr[3];
	__u8	fcid[8];
	__u8	rsv1;
	__u8	*session_key;
	__u8	*ctrl_vectors;
};

/* CDTAKED (Cross-Domain Take Down). CDTAKED initiates a procedure to cause
 * the takedown of all cross-domain LU-LU sessions (active, pending-active,
 * and queued) involving the domains of both the sending and receiving SSCP.
 * It also prevents the initiation of new LU-LU sessions between these domains.
 */
struct sna_ru_cdtaked {
	__u8	ns_hdr[3];
	__u8	pcid[8];
	__u8	lu_lu_type:2,
		takedown:2,
		sscp_sscp_term:1,
		rsv1:3;

	/* Reason. */
	__u8	user:1,
		state:1,
		rsv2:6;
};

/* CDTAKEDC (Cross-Domain Take-Down Complete). Except when the Cleanup
 * option was specified, the SSCP that received CDTAKED (and responded
 * positively to it) sends CDTAKEDC upon completion of its domain takedown
 * procedure. The other SSCP, after completing its domain takedown procedure
 * and receivinf a CDTAKEDC, also sends a CDTAKEDC.
 */
struct sna_ru_cdtakedc {
	__u8	ns_hdr[3];
	__u8	pcid[8];
	__u8	type;
	__u8	status;
};

/* CDTERM (Cross-Domain Terminate). CDTERM requests that the receiving SSCP
 * assist in the termination of the cross-domain LU-LU session identified
 * by the Session Key and the Type byte of the RU. Each SSCP executes that
 * portion of termination processing that relates to the LU in its domain.
 */
struct sna_ru_cdterm {
	__u8	ns_hdr[3];
	__u8	format:4,
		rsv1:4;
	__u8	type;
	__u8	pcid[8];
	__u8	reason;
	__u16	rsv2;
	__u8	*session_key;
	__u16	rsv3;
	__u8	*ctrl_vectors;
};

/* CHASE (Chase). CHASE is sent by a half-session to request the receiving
 * half-session to return all outstanding normal-flow responses to requests
 * previously received from the issuer of CHASE. The receiver of CHASE sends
 * the response to CHASE after processing (and sending any necessary responses
 * to) all requests received before the CHASE. This RU is not used for LU 6.2.
 */
struct sna_ru_chase {
	__u8	req_code;
};

/* CINIT (Control Initiate). CINIT requests the PLU to attempt to activate,
 * via a BIND request, a session with the specified SLU.
 */
struct sna_ru_cinit {
	__u8	ns_hdr[3];
	__u8	format:4,
		rsv1:4;
	__u8	init_origin:1,
		sub_source:1,
		slu_plu_olu:1,
		rsv2:1,
		name_sub_plu_slu:2,
		ext_bind_slu:1,
		xrf_supp_slu:1;
	__u8	session_key[5];

	__u16	bind_image_len;
	__u8	*bind_image;

	/* There is more. */
};

/* CLEANUP (Clean Up session). CLEANUP is sent by the SSCP to an LU (in a
 * subarea node or BF for peripheral LU) requesting that the LU of BF
 * attempt to deactivate the session for the specified (PLU, SLU) network
 * address pair.
 */
struct sna_ru_cleanup {
	__u8	ns_hdr[3];
	__u8	format:4,
		rsv1:4;
	__u8	rsv2;
	__u8	user:1,
		status:1,
		rsv3:6;
	__u8	*session_key;
	__u8	*ctrl_vectors;
};

/* CLEAR (Clear). CLEAR is sent by primary session control to reset the
 * data traffic. FSMs and subtrees (for example, brackets, pacing, sequence
 * numbers) in the primary and seconday half-session (and boundary function,
 * if any). CLEAR also resets compression and decompression tables in sessions
 * using length-checked compression. This RU is not used for LU 6.2.
 */
struct sna_ru_clear {
	__u8	req_code;
};

/* CONNOUT (Connect Out). CONNOUT requests the PU to initiate a connect-out
 * procedure on the specified link.
 */
struct sna_ru_connout {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	ls_id;		/* Link station ID. */
	__u8	connect_type:1,
		connect_out_feature:2,
		rsv1:2,
		connect_supp:1,
		nci:1,		/* Networking capabilities indicator. */
		sdai:1;		/* Static/Dynamic address indicator. */

	__u8	retry_limit;
	__u8	number;
	__u8	*dial_digits;

	__u8	id_number;
	__u8	*ctrl_vectors;
};

/* CONTACT (Contact). CONTACT requests the initiation of a procedure at the
 * PU to activate DLC-level contact with the adjacent link station specified
 * in the request. The DLC-level contact must be activated before any PIUs
 * can be exchanged with the adjacent node over the link.
 */
struct sna_ru_contact {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	rsv1:1,
		eami:1,		/* Enhanced addr managment indicator. */
		sdai:1,
		limit_resource:1,
		cp_cp_sess_supp:1,
		conn_supp:1,
		nci:1,		/* Networking capabilities indicator. */
		nnn_id_usage:1;	/* Nonnative network id usage indicator. */

	__u8	tg_num;
	__u8	*ctrl_vectors;
};

/* CONTACTED (Contacted). CONTACTED is issued by the PU to indicate to the
 * SSCP the completion of the DLC contact procedure. A status parameter
 * conveyed by this request informs SSCP configuration services whether or
 * not the contact procedure was successful; if not sucessful, the status
 * indicates whether an adjacent node load is required or whether an error
 * occured on the contact procedure.
 */
struct sna_ru_contacted {
	__u8	*raw;
};

/* CRV (Cryptogrpahy Verification). CRV, a valid request only when
 * session-level cryptography was selected in BIND, is sent by the primary
 * LU sesion control to verify cryptography security and thereby enable
 * sending and receiving of FMD requests by both half-sessions.
 */
struct sna_ru_crv {
	__u8	rq_code;
	__u8	crypto_seed[7];
};

/* CTERM (Control Terminate). CTERM requests that the PLU attempt to deactivate
 * a session identified by the specified (PLU,SLU) network address pair.
 */
struct sna_ru_cterm {
	__u8	ns_hdr[3];
	__u8	format:4,
		rsv1:4;
	__u8	rsv2:2,
		type:2,
		rsv3:4;
	__u8	user:1,
		status:1,
		rsv4:6;
	__u16	rsv5;
	__u8	*session_key;
	__u16	rsv6;
};

/* DACTCDRM (Deactivate Cross-Domain Resource Manager). DACTCDRM is sent
 * to deactivate a SSCP-SSCP session.
 */
struct sna_ru_dactcdrm {
	__u8	req_code;
	__u8	format:4,
		deact_type:4;

	/* Type 2 cont. */
	__u32	reason;

	/* Type 3 cont. */
	__u8	cause;
	__u8	rsv1;

	/* Type 4 cont. */
	__u8	rsv2;
};

/* DACTCONNIN (Deactivate Connect In). DACTCONNIN requests the PU to
 * disable the specified link from accepting incoming calls.
 */
struct sna_ru_dactconnin {
	__u8	ns_hdr[3];
	__u16	element_addr;
};

/* DACTLINK (Deactivate Link). DACTLINK initiates a procedure at the PU to
 * deactivate the protocol boundary between a link station in the node
 * (as specified by the link network address parameter in the request) and
 * the link connection attached to it. The normal type is used after all
 * adjacent link stations on the specified link have been discontacted.
 * The unconditional type may be used at any time to reset immediately a
 * link and its attached stations, regardless of their current state. If any
 * other control points were actively sharing control of the link at the time
 * of reset, thy are notified via INOP with a unique code. The give-back type
 * gives back link ownership without disrupting LU-LU sessions.
 */
struct sna_ru_dactlink {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	dactlink_type;
};

/* DACTLU (Deactivate Logical Unit). DACTLU is sent to deactivate the session
 * between the SSCP and the LU.
 */
struct sna_ru_dactlu {
	__u8	req_code;
	__u8	deact_type;
	__u8	cause;
};

/* DACTPU (Deactivate Physical Unit). DACTPU is sent to deactivate the
 * session between the SSCP and the PU.
 */
struct sna_ru_dactpu {
	__u8	req_code;
	__u8	deact_type;
	__u8	cause;
};

/* DACTTRACE (Deactivate Trace). DACTTRACE requests the PU to deactivate
 * a specified type of trace for a specified resource (or hierachy of
 * resources for a generalized PIU trace.
 */
struct sna_ru_dacttrace {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	tg_trace:1,
		piu_trace:1,
		rsv1:2,
		scanner_int_trace:1,
		rsv2:1,
		trace_all_frames:1,
		link_trace:1;

	__u8	*trace_data;
};

/* DELETENR (Delete Network Resource). DELETENR is sent to free a network
 * address addigned to a link or adjacent link station.
 */
struct sna_ru_deletenr {
	__u8	ns_hdr[3];
	__u16	element_addr;
};

/* DISCONTACT (Discontact). DISCONTACT requests the PU to deactivate DLC-level
 * contact with the specified adjacent node. The discontact procedure is
 * DLC-dependent; if applicable, polling is stopped. DISCONTACT may be used
 * to terminate contact, IPL, or dump procedures before their completion. The
 * PU responds negatively to DISCONTACT if any uninterruptable link-level
 * procedure is in progress at the primary link station of the specified link.
 */
struct sna_ru_discontact {
	__u8	ns_hdr[3];
	__u16	element_addr;
};

/* DISPSTOR (Display Storage). DISPSTOR requests the PU to send a RECSTOR RU
 * containing a specified number of bytes of storage beginning at a specified
 * location, or the names and related information for load modules and dump
 * (if present) on the disk attached to the T4 node.
 */
struct sna_ru_dispstor {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	target_addr_space:4,
		display_type:4;
	__u8	rsv1;
	__u16	display_bytes;
	__u32	begin_local_display;
};

/* DSRLST (Direct Search List). DSRLST specifies a list search argument to
 * be used at the receiving SSCP to identify a control list type to be
 * returned on RSP(DSRLST).
 */
struct sna_ru_dsrlst {
	__u8	ns_hdr[3];
	__u8	search_arg;
	__u8	*ctrl_vectors;
};

/* DUMPFINAL (Dump Final). DUMPFINAL performs one of two functions depending
 * on the address used in the request:
 * - When the DUMPFINAL request contains the link station address of an
 *   adjacent T4 node. DUMPFINAL terminates the dump sequence in progress
 *   (whether DUMPTEST is used or not). A positive response by the T4/5 node
 *   to this form of DUMPFINAL indicates that the dump sequence is complete.
 * - Then the DUMPFINAL request contains the network address of the receiving
 *   T4 node (not applicable to a T5 node) and a link station address of
 *   0x0000, the DUMPFINAL request causes an ABEND at the T4 node. The T4
 *   node then dumps to local disk. No response is returned to the requester
 *   for this form of DUMPFINAL.
 */
struct sna_ru_dumpfinal {
	__u8	ns_hdr[3];
	__u16	element_addr;
};

/* DUMPINIT (Dump Initial). DUMPINIT performs one of two functions, depending
 * on the address used in the request:
 * - When the node to be dumped is identified by an adjacent link station
 *   address, DUMPINIT causes the receiving T4/5 node to initiate a DLC-level
 *   dump from the adjacent T4 node (identified in the DUMPINIT) to the
 *   receiving T4/5 node; this dump is sent to the SSCP on subsequent
 *   RSP(DUMPTEXT)s.
 * - When the DUMPINIT request contains the network address of the receiving
 *   T4 node (not applicable to a T5 node), a link station address of 0x0000
 *   and a Dump Control byte equal to 0x80, the DUMPINIT interrogates the
 *   status of the receiving node's system-defined local options (to react
 *   to a subsequent DUMPFINAL), and its capacity to store a dump of its own
 *   contents to local disk storage. A positive response to the request
 *   indicates that a DUMPFINAL request can be accepted ( and the local dump
 *   be performed). A negative response indicates either that the system
 *   defined local options conflict with those of the requester, or
 *   insufficient disk capacity exists to at the DUMPINIT receiver hold the
 *   dump.
 */
struct sna_ru_dumpinit {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	dump_type;
};

/* DUMPTEXT (Dump Text). If further dump data is required, DUMPINIT may be
 * followed by DUMPTEXT. DUMPTEXT causes the dump data specified by the
 * starting-address parameter to be returned to the SSCP on the response.
 * The T4/5 obtains the dump data from the T4 node, using a DLC-level
 * interchange.
 */
struct sna_ru_dumptext {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u32	start_dump_addr;
	__u16	text_len;
};

/* ER_TESTED (Explicit Route Tested). ER_TESTED is sent by a subarea node
 * to one or more SSCPs to provide the status of an ER as determined by
 * explicit route test procedures.
 */
struct sna_ru_er_tested {
	__u8	ns_hdr[3];
	__u8	format;
	__u8	type;
	__u8	er_len;
	__u8	er_max_len;
	__u32	subarea_addr_dst;
	__u8	rsv1;
	__u8	rsv2:4,
		ern_of_er:4;
	__u32	subarea_addr_src;
	__u16	rev_ern_mask;
	__u16	max_piu_len;
	__u16	max_piu_size;
	__u16	ns_er_test_origin_sscp_subarea_num;
	__u16	ns_er_test_origin_sscp_element_num;
	__u8	req_corr[9];

	/* More. */
};

/* ESLOW (Entering Slowdown) ESLOW informs the SSCP that the node of the
 * sending PU has entered a slowdown state. This state is generally
 * associated with buffer depletion, and requires traffic through the node
 * to be selectively reduced or suspended.
 */
struct sna_ru_eslow {
	__u8	ns_hdr[3];
	__u16	element_addr;
};

/* EXECTEST (Execute Test). EXECTEST requests the PU to activate the specified
 * test type related to the specified network address. The test code specifies
 * the test type and defines the contents of the test data field. The test may
 * be for the PU, or for the LUs or links supported by the PU.
 */
struct sna_ru_exectest {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u32	test_code;
	__u8	*test_data;
};

/* EXPD (Expedited Data). EXPD is an expediated-flow request that can be
 * sent between half-sessions, regardless of the status of the normal flows,
 * to carry TP-defined data. This RU is defined for LU 6.2 only.
 */
struct sna_ru_expd {
	__u8	req_code;
	__u8	rsv1;
	__u16	exp_data_len;
	__u8	*exp_data;
};

/* EXSLOW (Exiting Slowdown). EXSLOW informs the SSCP that the node of the
 * sending PU is no longer in the slowdown state and regular traffic can
 * resume.
 */
struct sna_ru_exslow {
	__u8	ns_hdr[3];
	__u16	element_addr;
};

/* FNA (Free Network Addresses). FNA is sent from an SSCP to request the PU
 * T4/5 to free the identified element address(es) associated with the
 * target resource. If ENA is not supported, the entire network address is
 * in each Element Address field throughout this RU.
 */
struct sna_ru_fna {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	num_element_addrs;

	__u8	rsv1:1,
		eami:1,			/* Enhanced Address Managment Indi. */
		sdai:1,			/* Static/Dynamic Addr Indi. */
		rsv2:5;

	__u16	first_element_addr;
	__u16	*other_element_addrs;
};

/* INIT_OTHER (Initiate Other). INIT_OTHER from the ILU requests the initiation
 * of a session between the two LUs named in the RU. The requester may be a
 * third-party LU or one of the two named LUs. This RU is not used by LU 6.2
 * although it can be used by a third-party LU for LU 6.2.
 */
struct sna_ru_init_other {
	__u8	ns_hdr[3];
	__u8	format:4,
		rsv1:4;
	__u8	iqi:2,		/* Initiate/Enqueue Indicator. */
		rsv21:4,
		pluslu_spec:1,
		rsv2:1;
};

/* INITPROC (Initiate Procedure). INITPROC is sent to the subarea PU adjacent
 * to a PU T2 in order to initiate a PU T4/5 PU T2 load operation.
 */
struct sna_ru_initproc {
	__u8	ns_hdr[3];
	__u8	rsv1[3];
	__u16	element_addr;
	__u8	proc_type;
	__u8	*ipl_load_module;
};

/* IPLFINAL (IPL Final). IPLFINAL completes an IPL sequence and supplies the
 * load-module entry point to the T4 node. A positive response to IPLFINAL
 * indicates that the T4 node is successfully loaded, or the load module
 * has been successfully added to or replaced on the T4 node's local disk.
 */
struct sna_ru_iplfinal {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u32	entry_point;
	__u8	ipl_sli:1,	/* IPL save load indicator. */
		ui:1,		/* Usage indicator. */
		daipli:1,	/* Disk automatic IPL indicator. */
		dadi:1,		/* Disk automatic dump indicator. */
		rsv1:4;
};

/* IPLINIT (IPL Initial). IPLINIT either initiates a DLC-level load of an
 * adjacent T4 node from the T4 node receiving the IPLINIT, when the node to
 * be loaded is identified by the adjacent link station address contained in
 * the request, or initiates the adding, replacing, or purging of the load
 * module on the local disk of the T4 node receiving the request when the
 * address in the request is the network address of the PU T4 receiving the
 * request. In the case of purging, no IPLFINAL is sent, a positive response
 * to IPLINIT indicates that the load module has been successfully purged.
 */
struct sna_ru_iplinit {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	dipli:1,	/* Disk IPL indicator. */
		rsv1:7;
	__u8	load_module_name[8];
	__u8	lmi:2,
		rsv2:6;
};

/* IPLTEXT (IPL Text). IPLTEXT transfers load module information to the PU T4,
 * which passes it in a DLC-level load to the T4 adjacent node or adds or
 * replaces the load module on its local disk. Following an IPLINIT, any
 * number of IPLTEXT commands are valid, except that for purging and loading
 * from local disk, IPLTEXT is not sent.
 */
struct sna_ru_ipltext {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	*text;
};

/* LCP (Lost Control Point). LCP notifies the SSCP that a subarea PU's session
 * with another SSCP has failed. The SSCP displays the information for the
 * network operator.
 */
struct sna_ru_lcp {
	__u8	ns_hdr[3];
	__u8	reason_code;
	__u8	rsv1;
	__u8	net_addr[5];
};

/* LUSTAT (Logical Unit Status). LUSTAT is used by one half-session to send
 * up to four bytes of status information to its paired half-session. The
 * RU format allows the sending of either end-user information or LU status
 * information. If the high-order two bytes of the status information are 0,
 * the low-order two bytes carry end-user information and may be set to any
 * value. In general, LUSTAT is used to report about failures and error
 * recovery conditions for a local device of an LU.
 */
#pragma pack(1)
typedef struct {
	u_int8_t	rq_code		__attribute__ ((packed));
	u_int32_t	status		__attribute__ ((packed));
} sna_ru_lustat;
#pragma pack()

/* NC_ACTVR (Activate Virtual-Route). NC_ACTVR initializes the state and
 * attributes of the VR at each of its end nodes.
 */
struct sna_ru_nc_actvr {
	__u8	req_code;
	__u16	rsv1;
	__u8	format;
	__u8	rsv2;
	__u16	rcv_ern_mask;
	__u16	send_ern_mask;
	__u16	rsv3:4,
		init_vr_send_sqn:12;
	__u8	rsv4;
	__u8	max_vr_window;
	__u8	rsv5;
	__u8	min_vr_window;
	__u16	max_piu_size;
	__u16	max_piu_len;
};

/* NC_DACTVR (Deactivate Virtual Route). NC_DACTVR deactivates a virtual
 * route.
 */
struct sna_ru_nc_dactvr {
	__u8	req_code;
	__u16	rsv1;
	__u8	format;
	__u8	type;
};

/* NC_ER_ACT (Explicit Route Activate). NC_ER_ACT is sent by the ER manager
 * in a subarea node in order to activate an explicit route.
 */
struct sna_ru_nc_er_act {
	__u8	req_code;
	__u16	rsv1;
	__u8	format;
	__u8	rsv2;
	__u8	er_len;
	__u8	max_er_len;
	__u32	dst_subarea_addr;
	__u8	route_def_cap:1,
		rsv3:7;
	__u8	rsv4:4,
		ern:4;
	__u32	src_subarea_addr;
	__u16	reverse_ern_mask;
	__u16	max_piu_len;
	__u8	rsv5[8];
	__u8	act_req_sni[8];
};

/* NC_ER_INOP (Explicit Route Inoperative). NC_ER_INOP is initiated when the
 * last remaining link of the transmission group has failed or is disconnected
 * via a link-level procedure.
 */
struct sna_ru_nc_er_inop {
	__u8	req_code;
	__u16	rsv1;
	__u8	format;
	__u8	reason_code;
	__u32	home_subarea_addr;
	__u32	remote_subarea_addr;
	__u8	num_dst_subareas;
	__u8	inop_er_field[6];
	__u32	dst_subarea_addr;
	__u16	inop_er_mask;
	__u8	*data;
};

/* NC_ER_OP (Explicit Route Operative). NC_ER_OP is generated when a link of
 * an inoperative transmission group becomes operative.
 */
struct sna_ru_nc_er_op {
	__u8	req_code;
	__u16	rsv1;
	__u8	format;
	__u8	rsv2;
	__u32	home_subarea_addr;
	__u32	remote_subarea_addr;
	__u8	tg_number;
	__u8	num_subarea_addrs;
	__u8	op_er_field[6];
	__u32	dst_subarea_addr;
	__u16	op_er_mask;
	__u8	*data;
};

/* NC_ER_TEST (Explicit Route Test). NC_ER_TEST is sent by a subarea node that
 * requires testing of an explicit route to a specified destination subarea.
 */
struct sna_ru_nc_er_test {
	__u8	req_code;
	__u16	rsv1;
	__u8	format;
	__u8	rsv2;
	__u8	er_len;
	__u8	max_er_len;
	__u32	dst_subarea_addr;
	__u8	rsv3;
	__u8	rsv4:4,
		ern:4;
	__u32	orig_subarea_addr;
	__u16	reverse_er_mask;
	__u16	max_piu_size;
	__u16	rsv5;
	__u8	orig_sscp_net_addr[6];
	__u8	rq_cv[10];		/* Request Correlation Value. */
};

/* NC_ER_TEST_REPLY (Explicit Route Test Reply). NC_ER_TEST_REPLY is returned
 * to signal the successful or unsuccessful completion of te NC_ER_TEST.
 */
struct sna_ru_nc_er_test_reply {
	__u8	req_code;
	__u16	rsv1;
	__u8	format;
	__u8	type;
	__u8	er_len;
	__u8	max_er_len;
	__u32	dst_subarea_addr;
	__u8	rsv2;
	__u8	rsv3:4,
		ern:4;
	__u32	orig1_subarea_addr;
	__u16	reverse_ern_mask;
	__u16	max_piu_size_permit;
	__u16	max_piu_size_accum;
	__u8	orig_network_addr[6];
	__u8	rq_cv[10];
	__u32	orig2_subarea_addr;
	__u32	subarea_addr_type;
	__u8	tgn;
	__u8	*ctrl_vectors;
};

/* NC_IPL_ABORT (NC IPL Abort). NC_IPL_ABORT contains sense data indicating
 * the reason for a failure during IPL.
 */
struct sna_ru_nc_ipl_abort {
	__u8	req_code;
	__u32	sense;
};

/* NC_IPL_FINAL (NC IPL Final). NC_IPL_FINAL contains the entry point location
 * of the IPL module.
 */
struct sna_ru_nc_ipl_final {
	__u8	req_code;
	__u32	entry_point;
};

/* NC_IPL_INIT (NC IPL Init). NC_IPL_INIT is sent from a PU T4/5 to a PU T2
 * after the PU T4/5 processes an INITPROC (Type = IPL) RU.
 */
struct sna_ru_nc_ipl_init {
	__u8	req_code;
	__u8	rsv1;
	__u8	ipl_load_module[8];
};

/* NC_IPL_TEXT (NC IPL Text). NC_IP_TEXT contains the IPL data. */
struct sna_ru_nc_ipl_text {
	__u8	req_code;
	__u8	*data;
};

/* NMVT (Network Managment Vector Transport). NMVT carries management services
 * (MS) requests and replies between an SSCP and a PU.
 */
struct sna_ru_nmvt {
	__u8	ns_hdr[3];
	__u16	rsv1;
	__u16	rsv2:2,
		rsv3:2,
		prid:12;
	__u8	si:1,		/* Solicitation Indicator. */
		sf:2,		/* Sequence field. */
		sna_addr_list:1,
		rsv4:4;
	__u8	*ms_ctrl_vectors;
};

/* PROCSTAT (Procedure Status). PROCSTAT reports to the SSCP either the 
 * successful completion or the failure of the load operation. If the procedure
 * failed, the request code of the failing RU and sense data are included as
 * parameters in the PROCSTAT RU.
 */
struct sna_ru_procstat {
	__u8	ns_hdr[3];
	__u16	rsv1;
	__u16	element_addr;
	__u8	proc_type;
	__u8	proc_status;
	__u16	rsv2;
	__u8	status_qualifier[5];
	__u8	fail_ru_req_code;
	__u32	sense;
};

/* QC (Quiesce Complete). QC is sent by a halfpsession after receving QEC, to
 * indicate that it has quiesced. This RU is not used for LU 6.2
 */
struct sna_ru_qc {
	__u8	req_code;
};

/* QEC (Quiesce at End of Chain). QEC is sent by a half session to quiesce its
 * partner half-session after it (the partner) finishes sending the current
 * chain (if any). This RU is not used for LU 6.2.
 */
struct sna_ru_qec {
	__u8	req_code;
};

/* RECFMS (Record Formatted Maintenance Statistics). RECFMS permits the passing
 * of maintenance related information from a PU to managment services at the
 * SSCP.
 */
struct sna_ru_recfms {
	__u8	ns_hdr[3];
	__u16	cnm_target;
	__u16	rsv1:2,
		cnm_target_id:2,
		prid:12;
	__u8	si:1,
		ntri:1,
		req_type_code:6;
	__u32	block_num:12,
		id_num:20;
	__u16	rsv2;

	/* There is more. */
};

/* RECSTOR (Record Storage). RECSTOR carries the storage dump as requested by
 * a DISPSTOR RU, or the names and related information for load modules and
 * dump (if present) on the disk attached to the T4 node.
 */
struct sna_ru_recstor {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	display_src:4,
		display_type:4;
	__u8	rsv1;
	__u16	size;
	__u32	begin_location;
	__u8	*storage_display;
};

/* RECTD (Record Test Data). RECTD returns the status and results of a test
 * requested by EXECTEST to SSCP maintenance services.
 */
struct sna_ru_rectd {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u32	test_code;
	__u8	*data;
};

/* RELQ (Release Quiesce). RELQ is used to release a half-session from a
 * quiesced state. This RU is not used for LU 6.2.
 */
struct sna_ru_relq {
	__u8	req_code;
};

/* REQACTCDRM (Request Acitivation of Cross-Network Resource Manager).
 * REQACTCDRM prompts the receiving SSCP to issue RNAA and SETCV to setup a
 * cross-network address transform. ACTCDRM will then be sent to activate an
 * SSCP-SSCP session with the other-network SSCP identified in this request.
 */
struct sna_ru_reqactcdrm {
	__u8	ns_hdr[3];
	__u16	rsv1;
	__u8	format;
	__u8	tsr:1,
		vrid:1,
		rsv2:6;
	__u8	*session_key;
	__u8	*bad_actcdrm;
};

/* REQACTLU (Request Activate Logical Unit). REQACTLU is sent from the PU to an
 * SSCP to request that ACTLU be sent to the LU named in the RU.
 */
struct sna_ru_reqactlu {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	type;
	__u8	len;
	__u8	*symbol_name;
};

/* REQACTPU (Request Activate Physical Unit). REQACTPU is sent from the PU to
 * SSCP to request that ACTPU be sent to the PU named in the corresponding
 * FID2 Encapsulation (0x1500) GDS variable.
 */
struct sna_ru_reqactpu {
	__u8	ns_hdr[3];
	__u16	rsv1;
	__u8	format:4,
		rsv2:4;
};

/* REQCONT (Request Contact). REQCONT notifies the SSCP that a connection with
 * an adjacent secondary link station (in a T1/2 node) has be activated via a
 * successful connect-in or connect-out procedure. A DLC-level indentification
 * exchange (XID) is required before issuing REQCONT.
 */
struct sna_ru_reqcontx {
	__u16	vector_hdr;
	__u8	tg_status:1,
		rsv1:7;
};

struct sna_ru_reqcont {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	*xid_ifield_image;
	__u8	*ctrl_vectors;
};

/* REQDACTPU (Request Deactivate Physical Unit). REQDACTPU is sent from the PU
 * to an SSCP to request that DACTPU be sent to the PU.
 */
struct sna_ru_reqdactpu {
	__u8	ns_hdr[3];
	__u16	rsv1;
	__u8	format:4,
		rsv2:4;
	__u8	cause;
};

/* REQDISCONT (Request Discontact). With REQDISCONT, the PU T2 requests the
 * SSCP to start a procedure that will ultimately discontact the secondary
 * station in the T2 node.
 */
struct sna_ru_reqdiscont {
	__u8	ns_hdr[3];
	__u8	type:4,
		contact:4;
};

/* REQFNA (Request Free Network Address). REQFNA is sent from a PU T4/5 to an
 * SSCP to request the SSCP to send FNA to the PU T4/5 in order to free all
 * addresses for the specified LU.
 */
struct sna_ru_reqfna {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	rsv1;
	__u8	req_type;
};

/* REQMS (Request Maintenance Statisitics). REQMS requests the managment
 * services associated with the PU to provide maintenance statistics for the
 * resource indicated by the CNM target ID in the CNM header.
 */
struct sna_ru_reqms {
	__u8	*raw;
};

/* RNAA (Request Network Address Assignment). RNAA requests the PU to
 * assign addresses:
 * - To an adjacent link station, as identified in the RNAA request by a link
 *   element address and secondary link station link-level address.
 * - To a dependent LU, where the LU is identified in the RNAA by a adjacent
 *   link station address and the LU local address.
 * - To a subarea LU that supports parallel sessions; in order to assign an
 *   additional element address, the LU is identified in the RNAA request
 *   by the LU element address used for the SSCP-LU session.
 * - As alias addresses for a cross-network SSCP-SSCP or LU-LU session, where
 *   the name pair and session characteristics are identified in the RNAA
 *   request.
 * - To an LU to be used as a PLU address for an independent LU or to be used
 *   as an indepenedent or depenedent LU, where the LU is identified by an ALS
 *   address, an LU name, and an optional LU address.
 *
 * If ENA is not supported on this SSCP-to-PU T4/5 session, the entire network
 * address is in each element address field throughout this RU.
 */
struct sna_ru_rnaa {
	__u8	*raw;
};

/* ROUTE_INOP (Route Inoperative). ROUTE_INOP notifies the CP when either a
 * virtual route or an explicit route has become inoperative as the result of
 * a transmission group having become inoperative somewhere in the network.
 * This RU is retired for route dynamics.
 */
struct sna_ru_route_inop {
	__u8	*raw;
};

/* ROUTE_SETUP (Route Setup). ROUTE_SETUP is used between adjacent HPR CPs when
 * at leat one of them does not support the Control Flows over RTP (1402) option
 * set; it carries the Route Setup GDS variable and is preceded by a FID2 TH
 * set to FID2, BBIU, EBIU, ODAI = 0, EFI = 1, DAF = 0, OAF - 0, and SNA = 0
 * (ie, TH = 0x2D0000000000) and an RH set to RQ, NC, FI = 1, SDI = 0, BC, EC
 * DR1 = DR2 = ECI = 0 (RQN or no-response requested), with all other bits
 * 0 (ie RH = 0x2B0000).
 */
struct sna_ru_route_setup {
	__u8	req_code;
	__u8	*data;
};

/* ROUTE_TEST (Route Test). ROUTE_TEST requests the PC_ROUTE_MANAGER component
 * of PU.SVC_MGR to return the status (for example, active, operative, not
 * defined), as know in the control blocks in the node, of various explicit
 * and/or virutal routes.
 */
struct sna_ru_route_test {
	__u8	ns_hdr[3];
	__u16	element_addr;
	__u8	format;
	__u8	test_code;
	__u8	test_routes;
	__u8	rsv1:6,
		tx_priority:2;
	__u8	rsv2:1,
		congestion_data:1,
		vrdi:1,
		rsv3:5;
	__u8	max_exp_er_len;
	__u32	dst_subarea_addr;
	__u16	bit_route_test;
	__u8	req_cf[10];
	__u8	network_id[8];
};

/* RPO (Remote Power Off). RPO Causes the receiving PU T4/5 to initiate a
 * DLC-level power-off sequence to the T4 node specified by the adjacent link
 * station address conveyed in the request. The node being powered off does not
 * need to have an active SSCP-PU half-session not be contacted.
 */
struct sna_ru_rpo {
	__u8	ns_hdr[3];
	__u16	element_addr;
};

/* RQR (Request Recovery). RQR is sent by the secondary to request the primary
 * to initiate recovery for the session by sending CLEAR or to deactivate the
 * session. This RU is not used for LU 6.2.
 */
struct sna_ru_rqr {
	__u8	req_code;
};

/* RSHUTD (Request Shutdown). RSHUTD is sent from the secondary to the primary
 * to indicate that the secondary is ready to have the session deactivated.
 * RSHUTD does not request a shutdown; therefor, SHUTD is not a proper reply;
 * RSHUTD requests an UNBIND. This RU is not used for LU 6.2.
 */
struct sna_ru_rshutd {
	__u8	req_code;
};

/* RTR (Ready to Receive). RTR indicates to the bidder that it is now allowed
 * to initiate a bracket. RTR is sent only be the first speaker.
 */
struct sna_ru_rtr {
	__u8	req_code;
};

/* SBI (Stop Bracket Indication). SBI is sent by either half-session to request
 * that the receiving half-session stop initiating brackets by continued sending
 * of BB and the BID request. This RU is not used by LU 6.2.
 */
struct sna_ru_sbi {
	__u8	req_code;
};

/* SDT (Start Data Traffic). SDT is sent by the primary session control to the
 * secondary session control to enable the sending and receiving of FMD and
 * DFC requests and responses by both half-sessions. This RU is not used
 * by LU 6.2.
 */
struct sna_ru_sdt {
	__u8	req_code;
};

/* SESSEND (Session Ended). SESSEND is sent, with no-response requested, by
 * the LU (or boundary function on behalf of the LU in a periheral node) to
 * notify the SSCP that the session between the specified LUs has been
 * successfully deactivated.
 */
struct sna_ru_sessend {
	__u8	ns_hdr[3];
	__u8	format:4,
		rsv1:4;
	__u8	*raw;
};

/* SESSST (Session Started). SESSST is sent, with no response requested, to
 * notify the SSCP that the session between the specified LUs has been
 * successfully activated.
 */
struct sna_ru_sessst {
	__u8	ns_hdr[3];
	__u8	format;
	__u8	*raw;
};

/* SHUTC (Shutdown Complete). SHUTC is sent by a secondary to indicate that it
 * is in the shutdown (quiesced) state. This RU is not used for LU 6.2.
 */
struct sna_ru_shutc {
	__u8	req_code;
};

/* SHUTD (Shutdown). SHUTD is sent by the primary to request that the secondary
 * shut down (quiesce) as soon as convenient. This RU is not used for LU 6.2.
 */
struct sna_ru_shutd {
	__u8	req_code;
};

/* SIG (Signal). SIG is an expediated request that can be sent between
 * half-sessions, regardless of the status of the normal flows. It carries a
 * four-byte value, of which the first two bytes are the signal code and the
 * last two bytes are the signal extension value.
 */
#pragma pack(1)
typedef struct {
	u_int8_t	rq_code		__attribute__ ((packed));
	u_int16_t	signal_code	__attribute__ ((packed));
	u_int16_t	signal_ext	__attribute__ ((packed));
} sna_ru_sig;
#pragma pack()

/* STSN (Set and Test Sequence Numbers). STSN is sent by the primary 
 * half-session sync point manager to resynchronize the values of the 
 * half-session sequence numbers, for one or both of the normal flows at both
 * ends of the session. The RU is not used for LU 6.2.
 */
struct sna_ru_stsn {
	__u8	req_code;
	__u8	sp_act_code:2,
		ps_act_code:2,
		rsv1:4;
	__u16	sp_snf;
	__u16	ps_snf;
};

/* SWITCH (Switch Data Traffic). SWITCH is sent by the PLU to the SLU to change
 * the (XRF) state of their LU-LU session from XRF-backup to XRF-active.
 */
struct sna_ru_switch {
	__u8	req_code;
	__u8	type;
};

/* TERM_OTHER (Terminate Other). TERM_OTHER from the TLU requests that the
 * SSCP assist in terminating one or more sessions between the two LUs named in
 * the RU. The requester may be a third-party LU or onr of the two named LUs.
 * This RU is not used by LU 6.2, although it can be used by a third party
 * LU for LU 6.2.
 */
struct sna_ru_term_other {
	__u8	*raw;
};

/* TERM_SELF FORMAT 0 (Terminate Self). TERM_SELF from the TLU requests that the
 * SSCP assist in the termination of one or more sessions between the sender
 * of the request (TLU = OLU) and the DLU. This RU is not used for LU 6.2;
 * refer to TERM_SELF FORMAT 1.
 */
struct sna_ru_term_self_fmt0 {
	__u8	*raw;
};

/* TERM_SELF FORMAT 1 (Terminate Self). TERM_SELF from the TLU requests that the
 * SSCP assist in the termination of one or more sessions between the sender of
 * the request (TLU = OLU) and the DLU.
 */
struct sna_ru_term_self_fmt1 {
	__u8	*raw;
};

/* TESTMODE (Test Mode). TESTMODE requests the management services associated
 * with the PU to manage a test procedure. The test procedure begins with the
 * TESTMODE request that initiates a test and ends when the test results and
 * status are returned in a RECTR reply request corresponding to the initial
 * TESTMODE request.
 */
struct sna_ru_testmode {
	__u8	ns_hdr[3];
	__u16	cnm_target_id;
	__u16	rsv1:2,
		cnm_target_id_desc:2,
		prid:12;
	__u8	*data;
};

/* UNBIND (Unbind Session). UNBIND is sent to deactivate an active session
 * between the two LUs.
 */
#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t        rc              __attribute__ ((packed));
        u_int8_t        type            __attribute__ ((packed));
        u_int32_t       sense           __attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t	rc		__attribute__ ((packed));
	u_int8_t	type		__attribute__ ((packed));
	u_int32_t	sense		__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_ru_unbind;
#pragma pack()

/* UNBINDF (Unbind Failure). UNBINDF is sent, with no-response requested, by
 * the PLU to notify the SSCP that the attempt to deactivate the session
 * between the specified LUs has failed (for example, because of a path failure)
 */
#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int32_t	rc:24		__attribute__ ((packed));
	u_int32_t	sense		__attribute__ ((packed));
	u_int8_t	rsv2:5,
			takedown:1,
			unbind:1,
			rsv1:1		__attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int32_t       rc:24           __attribute__ ((packed));
        u_int32_t       sense           __attribute__ ((packed));
	u_int8_t	rsv1:1,
			unbind:1,
			takedown:1,
			rsv2:5		__attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_ru_unbindf;
#pragma pack()

/* RU NS Header indicators. */
#define SNA_RU_NS_CONTACT		0x010201
#define SNA_RU_NS_DISCONTACT		0x010202
#define SNA_RU_NS_IPLINIT		0x010203
#define SNA_RU_NS_IPLTEXT		0x010204
#define SNA_RU_NS_IPLFINAL		0x010205
#define SNA_RU_NS_DUMPINIT		0x010206
#define SNA_RU_NS_DUMPTEXT		0x010207
#define SNA_RU_NS_DUMPFINAL		0x010208
#define SNA_RU_NS_RPO			0x010209
#define SNA_RU_NS_ACTLINK		0x01020A
#define SNA_RU_NS_DACTLINK		0x01020B
#define SNA_RU_NS_CONNOUT		0x01020E
#define SNA_RU_NS_ABCONN		0x01020F
#define SNA_RU_NS_SETCV			0x010211
#define SNA_RU_NS_ESLOW			0x010214
#define SNA_RU_NS_EXSLOW		0x010215
#define SNA_RU_NS_ACTCONNIN		0x010216
#define SNA_RU_NS_DACTCONNIN		0x010217
#define SNA_RU_NS_ABCONNOUT		0x010218
#define SNA_RU_NS_ANA			0x010219
#define SNA_RU_NS_FNA			0x01021A
#define SNA_RU_NS_REQDISCONT		0x01021B
#define SNA_RU_NS_CONTACTED		0x010280
#define SNA_RU_NS_INOP			0x010281
#define SNA_RU_NS_REQCONT		0x010284
#define SNA_RU_NS_NS_LSA		0x010285
#define SNA_RU_NS_EXECTEST		0x010301
#define SNA_RU_NS_ACTTRACE		0x010302
#define SNA_RU_NS_DISPSTOR		0x010331
#define SNA_RU_NS_RECSTOR		0x010334
#define SNA_RU_NS_RECMS			0x010381
#define SNA_RU_NS_RECTD			0x010382
#define SNA_RU_NS_RECTRD		0x010383
#define SNA_RU_NS_NSPE			0x010604
#define SNA_RU_NS_INIT_SELF_FMT0	0x010681
#define SNA_RU_NS_TERM_SELF_FMT0	0x010683
#define SNA_RU_NS_RNAA			0x410210
#define SNA_RU_NS_DELETENR		0x41021C
#define SNA_RU_NS_ER_INOP		0x41021D
#define SNA_RU_NS_ADDLINK		0x41021E
#define SNA_RU_NS_ADDLINKSTA		0x410221
#define SNA_RU_NS_VR_INOP		0x410223
#define SNA_RU_NS_INITPROC		0x410235
#define SNA_RU_NS_PROCSTAT		0x410236
#define SNA_RU_NS_LDREQD		0x410237
#define SNA_RU_NS_REQACTPU		0x41023E
#define SNA_RU_NS_REQDACTPU		0x41023F
#define SNA_RU_NS_REQACTLU		0x410240
#define SNA_RU_NS_NS_IPL_INIT		0x410243
#define SNA_RU_NS_NS_IPL_TEXT		0x410244
#define SNA_RU_NS_NS_IPL_FINAL		0x410245
#define SNA_RU_NS_NS_IPL_ABORT		0x410246
#define SNA_RU_NS_REQFNA		0x410286
#define SNA_RU_NS_LCP			0x410287
#define SNA_RU_NS_ROUTE_INOP		0x410289
#define SNA_RU_NS_REQACTCDRM		0x41028A
#define SNA_RU_NS_REQMS			0x410304
#define SNA_RU_NS_TESTMODE		0x410305
#define SNA_RU_NS_ROUTE_TEST		0x410307
#define SNA_RU_NS_RECFMS		0x410384
#define SNA_RU_NS_RECTR			0x410385
#define SNA_RU_NS_ER_TESTED		0x410386
#define SNA_RU_NS_NMVT			0x41038D
#define SNA_RU_NS_REQECHO		0x810387
#define SNA_RU_NS_ECHOTEST		0x810389
#define SNA_RU_NS_CINIT			0x810601
#define SNA_RU_NS_CTERM			0x810602
#define SNA_RU_NS_NOTIFY		0x810620
#define SNA_RU_NS_CLEANUP		0x810629
#define SNA_RU_NS_INIT_OTHER		0x810680
#define SNA_RU_NS_INIT_SELF_FMT1	0x810681
#define SNA_RU_NS_TERM_OTHER		0x810682
#define SNA_RU_NS_TERM_SELF_FMT1	0x810683
#define SNA_RU_NS_BINDF			0x810685
#define SNA_RU_NS_SESSST		0x810686
#define SNA_RU_NS_UNBINDF		0x810687
#define SNA_RU_NS_SESSEND		0x810688
#define SNA_RU_NS_FORWARD		0x810810
#define SNA_RU_NS_DELIVER		0x810812
#define SNA_RU_NS_BFCINIT		0x812601
#define SNA_RU_NS_BFCLEANUP		0x812629
#define SNA_RU_NS_BFINIT		0x812681
#define SNA_RU_NS_BFTERM		0x812683
#define SNA_RU_NS_BFSESSST		0x812686
#define SNA_RU_NS_BFSESSEND		0x812688
#define SNA_RU_NS_BFSESSINFO		0x81268C
#define SNA_RU_NS_DSRLST		0x818627
#define SNA_RU_NS_INIT_OTHER_CD		0x818640
#define SNA_RU_NS_CDINIT		0x818641
#define SNA_RU_NS_CDTERM		0x818643
#define SNA_RU_NS_CDSESSF		0x818645
#define SNA_RU_NS_CDSESST		0x818646
#define SNA_RU_NS_CDSESSTF		0x818647
#define SNA_RU_NS_CDSESSEND		0x818648
#define SNA_RU_NS_CDTAKED		0x818649
#define SNA_RU_NS_CDTAKEDC		0x81864A
#define SNA_RU_NS_CDCINIT		0x81864B

/* RU Request Codes. */
#define SNA_RU_RC_NC_IPL_FINAL		0x02
#define SNA_RU_RC_NC_IPL_INIT		0x03
#define SNA_RU_RC_EXPD			0x03
#define SNA_RU_RC_LUSTAT		0x04
#define SNA_RU_RC_NC_IPL_TEXT		0x04
#define SNA_RU_RC_RTR			0x05
#define SNA_RU_RC_LSA			0x05
#define SNA_RU_RC_NC_ER_INOP		0x06
#define SNA_RU_RC_NC_ER_TEST		0x09
#define SNA_RU_RC_NC_ER_TEST_REPLY	0x0A
#define SNA_RU_RC_NC_ER_ACT		0x0B
#define SNA_RU_RC_NC_ER_ACT_REPLY	0x0C
#define SNA_RU_RC_ACTLU			0x0D
#define SNA_RU_RC_NC_ACTVR		0x0D
#define SNA_RU_RC_DACTLU		0x0E
#define SNA_RU_RC_NC_DACTVR		0x0E
#define SNA_RU_RC_NC_ER_OP		0x0F
#define SNA_RU_RC_ROUTE_SETUP		0x10
#define SNA_RU_RC_ACTPU			0x11
#define SNA_RU_RC_DACTPU		0x12
#define SNA_RU_RC_ACTCDRM		0x14
#define SNA_RU_RC_DACTCDRM		0x15
#define SNA_RU_RC_BIND			0x31
#define SNA_RU_RC_UNBIND		0x32
#define SNA_RU_RC_SWITCH		0x33
#define SNA_RU_RC_NC_IPL_ABORT		0x46
#define SNA_RU_RC_BIS			0x70
#define SNA_RU_RC_SBI			0x71
#define SNA_RU_RC_QEC			0x80
#define SNA_RU_RC_QC			0x81
#define SNA_RU_RC_RELQ			0x82
#define SNA_RU_RC_CANCEL		0x83
#define SNA_RU_RC_CHASE			0x84
#define SNA_RU_RC_SDT			0xA0
#define SNA_RU_RC_CLEAR			0xA1
#define SNA_RU_RC_STSN			0xA2
#define SNA_RU_RC_RQR			0xA3
#define SNA_RU_RC_SHUTD			0xC0
#define SNA_RU_RC_CRV			0xC0
#define SNA_RU_RC_SHUTC			0xC1
#define SNA_RU_RC_RSHUTD		0xC2
#define SNA_RU_RC_BID			0xC8
#define SNA_RU_RC_SIG			0xC9

/* MS.
 */

/* Solicitation indicators */
#define NMVT_UNSOL      0x0
#define NMVT_SOL        0x1

/* NMVT sequence indicators */
#define NMVT_SINGLE     0x00
#define NMVT_LAST       0x01
#define NMVT_FIRST      10
#define NMVT_MIDDLE     11

/* Network Managment Vector Transport (NMVT) header */
struct sna_nmvt
{
//         sna_ru_request  *ru_request;

        unsigned char __pad1[2];

        __u16   __pad2:2,
                __pad3:2,
                prid:12;        /* Procedure related identifier */

        __u8    si:1,           /* Solicitation indicator */
                seqf:2,
                sals:1,         /* SNA Address List subvector indicator */
                __pad4:4;

        unsigned char msvect;   /* One or more MS major vectors */
};

/* Record formatted maintenance statistics headers */

/* CNM indicators */
#define CNM_LSID        00
#define CNM_ELEMENT     01

/* Not last indicators */
#define CNM_LAST_REQUEST_TRUE   0
#define CNM_LAST_REQUEST_FALSE  1

struct recfms_request
{
        __u8    solic:1,        /* Solicitation indicator */
                nlri:1,         /* Not-last request indicator */
                rqcode:6;       /* Request-specific type code */

        unsigned char nid[5];   /* 48 bit Node Identification */

        __u32   blk:12,         /* Block number */
                blkid:20;       /* Block ID number */

        __u16   __pad1;
};

#ifdef NOT

/* Alert indicators */
#define ALERT_EPERM             0x1     /* Permanent error */
#define ALERT_ETEMP             0x2     /* Temporary error */
#define ALERT_EPERFORMANCE      0x3     /* Performance error */
#define ALERT_EINVALD           0x4     /* Unsupported or invalid use */
#define ALERT_EBUSY             0x4     /* Busy */
#define ALERT_EAPP              0x5     /* Application generated */
#define ALERT_EOPRTR            0x6     /* Operator triggered */
#define ALERT_SNA               0x7     /* SNA summary */

/* Alert Major probable cause indicators */
#define ALERT_MJ_HRDW           0x1     /* hardware */
#define ALERT_MJ_SFTW           0x2     /* software */
#define ALERT_MJ_LINK           0x3     /* link connection */
#define ALERT_MJ_PROTO          0x4     /* protocol */
#define ALERT_MJ_ENVIR          0x5     /* environment */
#define ALERT_MJ_RMMEDIA        0x6     /* removable media */
#define ALERT_MJ_HRDW_SFTW      0x7     /* hardware or software */
#define ALERT_MJ_LOGIC          0x8     /* logical */
#define ALERT_MJ_OPRTR_SNDPD    0x9     /* operator of sending product */
#define ALERT_MJ_UNDEFINE       0xF     /* undetermined */

/* Alert Minor probablt casue indicators */
#define ALERT_MN_BPROCESSOR     0x01    /* base processor */
#define ALERT_MN_SPROCESSOR     0x02    /* service processor */
#define ALERT_MN_MICROCODE      0x03    /* microcode */
#define ALERT_MN_MAINSTORAGE    0x04    /* main storage */
#define ALERT_MN_DASD           0x05    /* DASD drive */
#define ALERT_MN_PRINTER        0x06    /* printer */
#define ALERT_MN_CARD           0x07    /* card reader/punch */
#define ALERT_MN_TAPE           0x08    /* tape drive */
#define ALERT_MN_KEYBOARD       0x09    /* keyboard */
#define ALERT_MN_PEN            0x0A    /* selector pen */
#define ALERT_MN_MAGREADER      0x0B    /* magnetic stripe reader */
#define ALERT_MN_DIS_PRINTER    0x0C    /* display or printer */
#define ALERT_MN_DISPLAY        0x0D    /* display unit */
#define ALERT_MN_REMOTE         0x0E    /* remote product */
#define ALERT_MN_POWER_INT      0x0F    /* power internal to this product */
#define ALERT_MN_IOCTRLR        0x10    /* I/O attached controller */
#define ALERT_MN_COMM_SCAN      0x11    /* communications controller scanner */
#define ALERT_MN_COMM_ADPTR     0x12    /* communications link adapter  */
#define ALERT_MN_LINK_ADPTR     0x13    /* link adapter */
#define ALERT_MN_CHNL_ADPTR     0x14    /* channel adapter */
#define ALERT_MN_LOOP_ADPTR     0x15    /* loop adapter */
#define ALERT_MN_DIR_ADPTR      0x16    /* adapter for direct attach devices */
#deifne ALERT_MN_MISC_ADPTR     0x17    /* miscellaneous adapter */
#define ALERT_MN_S390_CHNL      0x18    /* System/390 channel */
#define ALERT_MN_LINK           0x19    /* transmiss medium-ownership unknwn */
#define ALERT_MN_LINK_COMN      0x1A    /* common carrier transmission medium */#define ALERT_MN_LINK_CUST      0x1B    /* customer transmission medium */
#define ALERT_MN_LOOP           0x1C    /* transmiss medium-ownership unknwn */
#define ALERT_MN_LOOP_COMN      0x1D    /* common carrier transmission medium */#define ALERT_MN_LOOP_CUST      0x1E    /* customer transmission medium */
#define ALERT_MN_X21_EXT        0x1F    /* X.21 link conn ext to this prod */
#define ALERT_MN_X25_EXT        0x20    /* X.25 link conn ext to this prod */
#define ALERT_MN_X21_IFACE      0x21    /* local X.21 interface:  DTE-DCE */
#define ALERT_MN_X25_IFACE      0x22    /* local X.25 interface:  DTE-DCE */
#define ALERT_MN_LOCAL_MODEM    0x23    /* local modem */
#define ALERT_MN_REMOTE_MODEM   0x24    /* remote modem */
#define ALERT_MN_MODEM_LIFACE   0x25    /* local modem interface:  DTE-DCE */
#define ALERT_MN_MODEM_RIFACE   0x26    /* remote modem interface:  DTE-DCE */
#define ALERT_MN_LOCAL_PROBE    0x27    /* local probe */
#define ALERT_MN_REMOTE_PROBE   0x28    /* remote probe */
#define ALERT_MN_PROBE_LIFACE   0x29    /* local probe interface */
#define ALERT_MN_PROBE_RIFACE   0x2A    /* remote probe interface */
#define ALERT_MN_NETCONN        0x2B    /* network connection */
#define ALERT_MN_IBM_HOST_PG    0x2C    /* IBM host program */
#define ALERT_MN_IBM_HOST_APP   0x2D    /* IBM host application program */
#define ALERT_MN_IBM_HOST_TELE  0x2E    /* IBM host telecommunication access */
#define ALERT_MN_CUST_APP       0x2F    /* customer host application */
#define ALERT_MN_IBM_COMM_CTRL  0x30    /* IBM comm controller program */
#define ALERT_MN_IBM_CTRL_PG    0x31    /* IBM control program */
#define ALERT_MN_RMT_IFMDM      0x32    /* remote modem iface or remote prod */
#define ALERT_MN_RMT_MDM_TRANS  0x33    /* transmission med. or remote modem */
#define ALERT_MN_SDLC_XFMT      0x34    /* SDLC format exception */
#define ALERT_MN_BSC_XFMT       0x35    /* BSC format exception */
#define ALERT_MN_SS_XFMT        0x36    /* start/stop format exception */
#define ALERT_MN_SNA_XFMT       0x37    /* SNA format exception */
#define ALERT_MN_POWER_EXT      0x38    /* power external to product */
#define ALERT_MN_THERMAL        0x39    /* thermal */
#define ALERT_MN_PAPER          0x3A    /* paper */
#define ALERT_MN_TAPE           0x3B    /* tape */
#define ALERT_MN_DASD           0x3C    /* DASD - removable media */
#define ALERT_MN_CARD           0x3D    /* card */
#define ALERT_MN_MAG_STRIP      0x3E    /* magnetic stripe card */
#define ALERT_MN_NEG_SNA_RESP   0x3F    /* negative SNA response */
#define ALERT_MN_SYS_DEF_ERR    0x40    /* system definition error */
#define ALERT_MN_INSTALL_REST   0x41    /* installation restrictions */
#define ALERT_MN_LS_OFFLINE     0x42    /* adjacent link station offline */
#define ALERT_MN_LS_BUSY        0x43    /* adjacent link station busy */
#define ALERT_MN_DEVICE         0x44    /* controller or device */
#define ALERT_MN_LOCAL_PROBE    0x45    /* local probe or modem */
#define ALERT_MN_DRIVE          0x46    /* tape or drive */
#define ALERT_MN_CARD           0x47    /* card readr/punch or display/printr */#define ALERT_MN_CTRL_APP       0x48    /* controller application program */
#define ALERT_MN_KEY_DISPLAY    0x49    /* keyboard or display */
#define ALERT_MN_SCU            0x4A    /* storage control unit */
#define ALERT_MN_CHNL_SCU       0x4B    /* channel or storage control unit */
#define ALERT_MN_SCU_CTRL       0x4C    /* storage control unit or controller */#define ALERT_MN_CU             0x4D    /* control unit */
#define ALERT_MN_DASD_M_D_D     0x4E    /* DASD data or media or drive */
#define ALERT_MN_DASD_M_D       0x4F    /* DASD data or media */
#define ALERT_MN_DISK           0x50    /* diskette */
#define ALERT_MN_DISK_DRIVE     0x51    /* diskette or drive */
#define ALERT_MN_UNDETERMINE    0xFF    /* undetermined */

#endif

struct recfms_alert
{
        __u8    alert:2,        /* 00 - Alert */
                acode:6;        /* Alert type code - 0000 */

        unsigned char nid[5];   /* 48 bit Node Identification */

        __u32   blk:12,         /* Block number */
                blkid:20;       /* Block ID number */
        __u16   __pad1;

        /* Alert classification */
        __u8    aformat:2,      /* Alert Class Format */
                __pad2:6;
        __u8    atype:4,        /* Alert type, see Alert codes */
                amjcause:4;     /* Alert major cause */
        __u8    amncause;       /* Alert minor cause */
        __u8    __pad3;
        __u8    actcode;        /* User action code */
        __u8    __pad4;

        unsigned char cnmvectors;       /* Appended CNM vectors */
};

struct sna_sdlc_stats
{
        struct recfms_request *request; /* header */
        __u16   wvfcs;
        __u16   vfcs;
};

struct sna_summary_err
{
        struct recfms_request *request; /* header */
        __u16   errcnt:1,       /* 1 if product error counter is valid */
                commcnt:1,      /* 1 if comm adapter error counter is valid */
                negcnt:1,       /* 1 if SNA neg response counter is valid */
                __pad1:5;

        __u8    __pad2;

        __u8    __pad3:7,
                commflag:1;     /* Comm adapter err flg RECFMS types 02 or 03 */
        __u16   prodcnt;        /* Product error counter */
        __u16   comadptr;       /* Communication adapter error counter */
        __u16   snaneg;         /* SNA neg resps originating at this node */
};

/* Comm adapter error set indicators */
#define COMM_ADPTR_SET1         0x01
#define COMM_ADPTR_SET2         0x02
#define COMM_ADPTR_SET3         0x03
#define COMM_ADPTR_SET4         0x04
#define COMM_ADPTR_SET5         0x05
#define COMM_ADPTR_SET6         0x06

/* Data for counters sets 1 and 2 */
struct sna_comm_adptr_set1
{
        /* Communication adapter counter validity mask bytes */

        __u8    nonprod_to:1,   /* Nonproductive time-out */
                idle_to:1,      /* Idle time-out counter */
                wrty:1,         /* Write retry counter */
                ovrn:1,         /* Overrun counter */
                undrn:1,        /* Underrun counter */
                connprb:1,      /* Connection problem counter */
                fcserr:1,       /* FCS error counter */
                psabort:1;      /* Primary station abort counter */

        __u8    cmdrej:1,       /* SDLC Command reject counter */
                dceerr:1,       /* SDLC DCE error counter */
                wto:1,          /* Write time-out counter */
                inval:1,        /* Invalid status counter */
                comm_chk:1,     /* Comm adapter machine check counter */
                __pad1:3;

        __u8    __pad2;

        __u8    nonprod_to_cnt; /* Nonproductive time-out counter */
        __u8    idle_to_cnt;    /* Idle time-out counter */
        __u8    wrty_cnt;       /* Write retry counter */
        __u8    ovrn_cnt;       /* Overrun counter */
        __u8    undrn_cnt;      /* Underrun counter */
        __u8    connprb_cnt;    /* Connection problem counter */
        __u8    fcserr_cnt;     /* FCS error counter */
        __u8    psabort_cnt;    /* Primary station abort counter */
        __u8    cmdrej_cnt;     /* SDLC command reject counter */
        __u8    dceerr_cnt;     /* SDLC DCE error counter */
        __u8    wto_cnt;        /* Write time-out counter */
        __u8    inval_cnt;      /* Invalid status counter */
        __u8    comm_chk_cnt;   /* Communication adapter machine check cntr */
};

/* Data for counters set 3 */
struct sna_comm_adptr_set3
{
        __u8    tx_iframes:1,   /* Total transmitted I-frames counter */
                wrty:1,         /* Write retry counter */
                rx_iframes:1,   /* Total received I-frames counter */
                fcserr:1,       /* FCS error counter */
                cmdrej:1,       /* SDLC command reject counter */
                dceerr:1,       /* DCE error counter */
                noprod_to:1,    /* Nonproductive time-out counter */
                __pad1;

        __u16   __pad2;

        __u16   tx_iframes_cnt; /* Total transmitted I-frames counter */
        __u16   wrty_cnt;       /* Write retry counter */
        __u16   rx_iframes_cnt; /* Total received I-frames counter */
        __u16   fcserr_cnt;     /* FCS error counter */
        __u16   cmdrej_cnt;     /* SDLC command reject counter */
        __u16   dceerr_cnt;     /* DCE error counter */
        __u16   noprod_to_cnt;  /* Nonproductive time-out counter */
};

struct sna_comm_adptr_set4
{
        __u8    cmdrejwni:1,    /* Command-reject-while-not-initialized cnt */
                cmdnrec:1,      /* Command-not-recognized counter */
                sensewni:1,     /* Sense-while-not-initialized counter */
                chnlpcdss:1,    /* Channel-prty-chk-during-selection-seq cnt */
                chnlpcddws:1,   /* Channel-parity-chk-during-data-wrt-seq cnt */                oppcacu:1,      /* Output-parity-check-at-control-unit cnt */
                ippcacu:1,      /* Input-parity-check-at-control-unit counter */
                ipcaac:1;       /* Input-parity-check-at-adapter counter */

        __u8    dataeaa:1,      /* Data-error-at-adapter counter */
                datasseq:1,     /* Data-stop-sequence counter */
                shrtfolchk:1,   /* Short-frame-or-length-check counter */
                connrcvwac:1,   /* Connect-rcvd-when-already-connected cnt */
                disrcvwpuact:1, /* Disconnect-received-while-PU-active cnt */
                longru:1,       /* Long-RU counter */
                connprmold:1,   /* Connect-parameter-error counter */
                rdstrtoldrcv:1; /* Read-Start-Old-received counter */

        __u8    __pad1;

        __u8    cmdrejwni_cnt;  /* Command-reject-while-not-initialized cnt */
        __u8    cmdnrec_cnt;    /* Command-not-recognized counter */
        __u8    sensewni_cnt;   /* Sense-while-not-initialized counter */
        __u8    chnlpcdss_cnt;  /* Channel-prty-chk-during-selection-seq cnt */
        __u8    chnlpcddws_cnt; /* Channel-parity-chk-during-data-wrt-seq cnt */        __u8    oppcacu_cnt;    /* Output-parity-check-at-control-unit cnt */
        __u8    ippcacu_cnt;    /* Input-parity-check-at-control-unit counter */        __u8    ipcaac_cnt;     /* Input-parity-check-at-adapter counter */
        __u8    dataeaa_cnt;    /* Data-error-at-adapter counter */
        __u8    datasseq_cnt;   /* Data-stop-sequence counter */
        __u8    shrtfolchk_cnt; /* Short-frame-or-length-check counter */
        __u8    connrcvwac_cnt; /* Connect-rcvd-when-already-connected cnt */
        __u8    disrcvwpuact_cnt;       /* Disconn-rcvd-while-PU-active cnt */
        __u8    longru_cnt;     /* Long-RU counter */
        __u8    connprmold_cnt; /* Connect-parameter-error counter */
        __u8    rdstrtoldrcv_cnt;       /* Read-Start-Old-received counter */
};

struct sna_comm_adptr_set5
{
        __u8    tx_iframes:1,   /* I-frames transmitted counter */
                rx_iframes:1,   /* I-frames received counter */
                tx_rrframes:1,  /* RR frames transmitted counter */
                rx_rrframes:1,  /* RR frames received counter */
                tx_rnrframes:1, /* RNR frames transmitted counter */
                rx_rnrframes:1, /* RNR frames received counter */
                tx_rejframes:1, /* REJ frames transmitted counter */
                rx_rejframes:1; /* REJ frames received counter */

        __u8    retransmit:1,   /* Number of retransmissions counter */
                fcs_frames:1,   /* Number of frames rcvd with FCS errors cnt */
                rcverr:1,       /* Number of errors on receive side counter */
                ovrn_rxs:1,     /* Number of overruns on receive side counter */                undrn_txs:1,    /* Number of underruns on transmit side cntr */
                __pad1:3;

        __u8    __pad2;

        __u16   tx_iframes_cnt; /* I-frames transmitted counter */
        __u16   rx_iframes_cnt; /* I-frames received counter */
        __u16   tx_rrframes_cnt;        /* RR frames transmitted counter */
        __u16   rx_rrframes_cnt;        /* RR frames received counter */
        __u16   tx_rnrframes_cnt;       /* RNR frames transmitted counter */
        __u16   rx_rnrframes_cnt;       /* RNR frames received counter */
        __u16   tx_rejframes_cnt;       /* REJ frames transmitted counter */
        __u16   rx_rejframes_cnt;       /* REJ frames received counter */
        __u16   retransmit_cnt; /* Number of retransmissions counter */
        __u16   fcs_frames_cnt; /* Number of frames rcvd with FCS errors cnt */
        __u16   rcverr_cnt;     /* Number of errors on receive side counter */
        __u16   ovrn_rxs_cnt;   /* Number of overruns on receive side counter */        __u16   undrn_txs_cnt;  /* Number of underruns on transmit side cnt */
};

struct sna_comm_adptr_set6
{
        __u8    tx_ipackets:1,  /* data packets transmitted counter */
                rx_ipackets:1,  /* data packets received counter */
                tx_rrpackets:1, /* RR packets transmitted counter */
                rx_rrpackets:1, /* RR packets received counter */
                tx_rnrpackets:1,        /* RNR packets transmitted counter */
                rx_rnrpackets:1,        /* RNR packets received counter */
                tx_intpackets:1,        /* interrupt packets transmitted cnt */
                rx_intpackets:1;        /* interrupt packets received counter */
        __u8    connreq:1,      /* Number of connection requests counter */
                conns:1,        /* Number of connections counter */
                rstindic:1,     /* Number of reset indications counter */
                clrindic:1,     /* Number of clear indications counter */
                tx_dbit:1,      /* data packets with D-bit transmitted cnt */
                rx_dbit:1,      /* data packets with D-bit received counter */
                __pad1:2;

        __u8    __pad2;

        __u16   tx_ipackets_cnt;        /* I packets transmitted */
        __u16   rx_ipackets_cnt;        /* I packets received */
        __u16   tx_rrpackets_cnt;       /* RR packets transmitted */
        __u16   rx_rrpackets_cnt;       /* RR packets received */
        __u16   tx_rnrpackets_cnt;      /* RNR packets transmitted counter */
        __u16   rx_rnrpackets_cnt;      /* RNR packets received counter */
        __u16   tx_intpackets_cnt;      /* interrupt packets transmitted cnt */
        __u16   rx_intpackets_cnt;      /* interrupt packets received counter */        __u16   connreq_cnt;    /* Total number of connection requests */
        __u16   conns_cnt;      /* Total number of connections*/
        __u16   rstindic_cnt;   /* Number of reset indications */
        __u16   clrindic_cnt;   /* Number of clear indications */
        __u16   tx_dbit_cnt;    /* Number of data pkts with D-bit transmitted */        __u16   rx_dbit_cnt;    /* Number of data packets with D-bit received */};

struct sna_comm_adptr_stats
{
        struct recfms_request *request; /* header */
        __u8    errset;         /* Comm adapter error counter sets */

        union {         /* Data sets */
                struct sna_comm_adptr_set1      set1;
                struct sna_comm_adptr_set1      set2;
                struct sna_comm_adptr_set3      set3;
                struct sna_comm_adptr_set4      set4;
                struct sna_comm_adptr_set5      set5;
                struct sna_comm_adptr_set6      set6;
        } counter;
};

struct sna_generic_stats
{
        struct recfms_request *request;
        unsigned char *data;
};

/* Data selection indicators */
#define LCSUB_LSC_SEQ           0x02
#define LCSUB_RMT_DTE_STATS     0x03
#define LCSUB_RMT_MDM_STEST     0x04

/* Link connection subsystem type */
#define LCSUB_TYPE1     0x01
#define LCSUB_TYPE2     0x02

/* Validity indicators */
#define LCSUB_DATA_VALID        00
#define LCSUB_NRSP_MDM          01
#define LCSUB_ERSP_MDM          10
#define LCSUB_DATA_NOTVALID     11

struct sna_lcsub_stats
{
        struct recfms_request *request;

        __u8    datasel;        /* Data selection */
        __u8    lctype;         /* Link connection subsystem type */

        /* Validity indicators */
        __u16   rmodem_stat:2,  /* Remote modem status */
                lmodem_stat:2,  /* Local modem status */
                modem_stest:2,  /* Modem self test */
                __pad1:2,
                rdte_stat:2,    /* Remote DTE interface status */
                __pad2:4,
                lfmt:2;         /* Link conn subsystem data format indicator */

        /* Remote modem status */
        __u16   rhcount:6,      /* Hit count */
                rrinit:1,       /* Modem reinitialization was performed */
                rlrcvsig:1,     /* Loss of receive line signal */
                rquaderr:4,     /* Quadratic error value */
                rrdtepwroff:1,  /* Remote DTE power off detected */
                rldtr:1,        /* Data Terminal Ready loss detected */
                rswntwkbkup:1,  /* Switched-Network-Back-Up connected */
                rdtestream:1;   /* DTE streaming condition detected */

        /* Local modem status */
        __u16   lhcount:6,      /* Hit count */
                lrinit:1,       /* Modem reinitialization was performed */
                llrcvsig:1,     /* Loss of receive line signal */
                lquaderr:4,     /* Quadratic error value */
                lrdtepwroff:1,  /* Remote DTE power off detected */
                lspeed:1,       /* Speed */
                lswntwkbkup:1,  /* Switched-Network-Back-Up connected */
                ldtestream:1;   /* DTE streaming condition detected */

        __u16   modelbits:3,    /* Model bits */
                linktype:1,     /* Link connection type */
                config:1,       /* Configuration */
                mrole:1,        /* Modem role */
                ctds:1,         /* Clear To Send delay */
                rcvlsigdet:1,   /* Received line signal detector sensitivity */
                modelbit1:1,    /* Model bit */
                stest:1,        /* Modem self-test result */
                rttest:1,       /* Remote tone test result */
                fcarderr:1,     /* Feature card suspected in error */
                rcvxcard:1,     /* Receiver card extension suspected in error */                frntcard:1,     /* Front end card is suspected in error */
                modelbit2:1,    /* Model bit */
                fcardinst:1;    /* Feature card installed */
        __u8    swntwkbkup:1,   /* Switched-Network-Back-Up installed */
                modelbit3:1,    /* Model bit */
                modelbit4:1,    /* Model bit */
                microcode:5;    /* Microcode EC level */

        __u8    rts:1,          /* Request To Send */
                cts:1,          /* Clear To Send */
                __pad3:1,
                td:1,           /* Transmit Data */
                __pad4:1,
                dtr:1,          /* Data Terminal Ready */
                speed:1,        /* Speed */
                dtepwrloss:1;   /* DTE power loss */

        __u8    rtqc:1,         /* Request To Send changed at least once */
                ctsc:1,         /* Clear To Send changed at least once */
                rdc:1,          /* Received Data changed state */
                tdc:1,          /* Transmit Data changed state */
                rcvlsigc:1,     /* Received Line Signal loss was detected */
                dtrd:1,         /* Data Terminal Ready dropped */
                speedc:1,       /* Modem speed was changed */
                dtepwrlossc:1;  /* DTE power loss was detected */

                /* Channelization status */
        __u8    channelized:1,  /* Is associated with a channelized modem */
                tailed:1,       /* Is associated with a tailed link chnl mdm */
                channela:1,     /* associated with channel A */
                __pad5:5;

        __u16   channelnum;     /* Channelization correlation number */

        __u8    ldblvl;         /* Local modem receive dB level */
        unsigned char __pad6[6];

        __u8    rdblvl;         /* Remote modem receive dB level */
        unsigned char __pad7[6];

        __u8    lladdr;         /* Link-level addr used to addr the rmt modem */        __u8    rdteifacex;     /* Remote DTE Interface Extension */

        unsigned char   __pad8[5];
};

/* CNM Vectors not included here */

struct sna_recfms
{
//        sna_ru_request  *ru_request;

        __u16   cmntid;         /* CNM target ID */

        __u16   __pad1:2,
                cnmtidd:2,      /* CNM target ID descriptor */
                prid:12;        /* Procedure related identifier */

        union {         /* Request specific information */
                struct sna_sdlc_stats           sdlc;
                struct sna_summary_err          sna;
                struct sna_comm_adptr_stats     adptr;
                struct sna_generic_stats        pulu;
                struct sna_generic_stats        enginr;
                struct sna_lcsub_stats          lcsub;
        } stats;
};

struct sna_rms_gen_specific
{
        __u8    reset:1,        /* Reset indicator */
                __pad1:1,
                tcode:6;        /* Type code */
};

struct sna_rms_pulu
{
        __u8    reset:1,        /* Reset indicator */
                __pad1:1,
                tcode:6;        /* Type code */
        unsigned char *data;    /* PU- or LU-dependent request parameters */
};

struct sna_rms_lcsub
{
        __u8    reset:1,        /* Reset indicator */
                __pad1:1,
                tcode:6;        /* Type code */
        __u8    dselrq;         /* Data selection requested */

};

struct sna_rms
{
//        sna_ru_request  *ru_request;

        __u16   cmntid;         /* CNM target ID */

        __u16   __pad1:2,
                cnmtidd:2,      /* CNM target ID descriptor */
                prid:12;        /* Procedure related identifier */

        union {
                struct sna_rms_gen_specific     request;
                struct sna_rms_gen_specific     sdlc;
                struct sna_rms_gen_specific     sna;
                struct sna_rms_gen_specific     adptr;
                struct sna_rms_pulu             pulu;
                struct sna_rms_gen_specific     enginr;
                struct sna_rms_lcsub            lcsub;
        } rms;
};

/* XID.
 */

/* XID I-field structures. The goal is to provide one set of XID headers
 * for all device types, Eth, Tr, Sdlc, X.25, Channel, etc, etc..
 */
#define SNA_XID_TYPE_0          0    /* Xid Type 0 */
#define SNA_XID_TYPE_1          1    /* Xid Type 1 */
#define SNA_XID_TYPE_2          2    /* Xid Type 2 */
#define SNA_XID_TYPE_3          3    /* Xid Type 3 */

#define SNA_XID_NODE_T1         1    /* T1 node */
#define SNA_XID_NODE_T2         2    /* T2.0 or T2.1 node */
#define SNA_XID_NODE_T4         4    /* T4 or T5 node */
#define SNA_XID_NODE_T5         SNA_XID_NODE_T4

#define SNA_XID_XSTATE_NOSUPP	0x00
#define SNA_XID_XSTATE_NONACT	0x01
#define SNA_XID_XSTATE_NEG	0x02
#define SNA_XID_XSTATE_PN	0x03

typedef struct {
        __u16   rsv1;
        __u8    rsv2:2,
                ls_role:1,
                rsv3:1,
                ls_txrx_cap:4;
        __u8    rsv4:2,
                seg_asm_cap:2,
                rsv5:2,
                sh_mode_status:1,
                sh_mode_support:1;
        __u16   format:1,
                max_ifield_len:15;
        __u8    rsv6:4,
                sdlc_cmd_rsp_profile:4;
        __u8    rsv7:2,
                sdlc_init_mode:1,
                rsv8:5;
        __u16   rsv9;
        __u8    rsv10:1,
                max_rx_iframe_win:7;
        __u8    rsv11;

        /* SDLC address assignment field */
        /*
         * Not supported yet.
         */
} sna_xid1;

typedef struct {
        __u8    raw;
} sna_xid2;

typedef struct {
        unsigned        rsv1:1                  __attribute__ ((packed));
        unsigned        abm:1                   __attribute__ ((packed));
        unsigned        ls_role_xid_sender:2    __attribute__ ((packed));
        unsigned        sh_mode_status:1        __attribute__ ((packed));
        unsigned        sh_mode:1               __attribute__ ((packed));
        unsigned        ls_txrx_cap:2           __attribute__ ((packed));
        unsigned        abm_nonact_xid:1        __attribute__ ((packed));
        unsigned        rsv2:7                  __attribute__ ((packed));
        unsigned        max_btu_len_format:1    __attribute__ ((packed));
        __u16           max_btu_len:15          __attribute__ ((packed));
        unsigned        rsv3                    __attribute__ ((packed));
        unsigned        rsv4:2                  __attribute__ ((packed));
        unsigned        dlc_init_mode:1         __attribute__ ((packed));
        unsigned        rsv5:5                  __attribute__ ((packed));
        __u16           rsv6                    __attribute__ ((packed));
        unsigned        rsv7:1                  __attribute__ ((packed));
        unsigned        max_rx_iframes:7        __attribute__ ((packed));
} sna_xid_dlc_satf;

typedef struct {
        __u16   cdlc_chg:1,
                attn_timeout_sup:1,
                data_stream:1,
                cdlc_chg_sup:1,
                rsv1:12;
        __u16   max_lpiu_len;
        __u8    buf_prefetch;
        __u16   num_read_cmds;
        __u16   buf_size;
        __u16   blocking_delay;
        __u16   attn_timeout;
        __u16   pre_num_read_cmds;
        __u16   prev_pri_buf_size;
        __u8    time_unit;
} sna_xid_dlc_s390_channel;

typedef struct {
        __u8    rsv1:2,
                xid_role:2,
                rsv2:4;
        __u8    rsv3;
        __u16   max_btu_size;

        /* Start of Control Vectors */
        __u8    cv;
} sna_xid_dlc_appn_channel;

#pragma pack(1)
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u_int8_t	type:4,
			format:4		__attribute__ ((packed));
	u_int8_t	len			__attribute__ ((packed));
	u_int32_t	nodeid			__attribute__ ((packed));
	u_int16_t	rsv1			__attribute__ ((packed));
	u_int8_t	rsv2:4,
                        whole_bind_piu_req:1,
                        whole_bind:1,
                        standalone_bind:1,
                        init_self:1		__attribute__ ((packed));
	u_int8_t	cp_name_chg:1,
			sec_nonactive_xchg:1,
			state:2,
			cp_cp_support:1,
			cp_services:1,
			network_node:1,
			actpu_suppression:1	__attribute__ ((packed));
	u_int8_t	adptv_bind_pacing:2,
			rsv3:1,
			appn_pbn:1,
			pu_cap_sup:1,
			quiesce_tg:1,
			rx_adaptive_bind_pacing:1,
			tx_adaptive_bind_pacing:1 __attribute__ ((packed));
	u_int8_t	rsv5:5,
			dedicated_scv:1,
			tg_sharing_prohibited:1,
			rsv4:1			__attribute__ ((packed));
	u_int8_t	rsv6[3]			__attribute__ ((packed));
	u_int8_t	rsv7:3,
			gen_odai_usage_opt_set:1,
			xhpr_bn:1,
			dlus_lu_reg:1,
			dlur_actpu:1,
			parallel_tg_sup:1	__attribute__ ((packed));
	u_int8_t	tg_num			__attribute__ ((packed));	
	u_int8_t	dlc_type		__attribute__ ((packed));
	u_int8_t	dlc_len			__attribute__ ((packed));
	u_int8_t	ls_txrx_cap:2,
			sh_mode:1,
			sh_mode_status:1,
			ls_role:2,
			dlc_abm:1,
			rsv8:1			__attribute__ ((packed));
	u_int8_t	rsv9:7,
			abm_nonact_xid:1	__attribute__ ((packed));
	u_int16_t	max_btu_len		__attribute__ ((packed));
	u_int8_t	rsv10			__attribute__ ((packed));
	u_int8_t	rsv12:5,
			dlc_init_mode:1,
			rsv11:2			__attribute__ ((packed));
	u_int16_t	rsv13			__attribute__ ((packed));
	u_int8_t	max_iframes		__attribute__ ((packed));
	u_int8_t	rsv14			__attribute__ ((packed));
#elif defined(__BIG_ENDIAN_BITFIELD)
	u_int8_t        format:4,
                        type:4                	__attribute__ ((packed));
        u_int8_t        len                     __attribute__ ((packed));
	u_int32_t       nodeid                  __attribute__ ((packed));
        u_int16_t       rsv1                    __attribute__ ((packed));
        u_int8_t       	init_self:1,
                        standalone_bind:1,
                        whole_bind:1,
                        whole_bind_piu_req:1,
                        rsv2:4			__attribute__ ((packed));
	u_int8_t	actpu_suppression:1,
                        network_node:1,
                        cp_services:1,
                        cp_cp_support:1,
                        state:2,
                        sec_nonactive_xchg:1,
                        cp_name_chg:1           __attribute__ ((packed));
        u_int8_t        tx_adaptive_bind_pacing:1,
                        rx_adaptive_bind_pacing:1,
                        quiesce_tg:1,
                        pu_cap_sup:1,
                        appn_pbn:1,
                        rsv3:1,
                        adptv_bind_pacing:2     __attribute__ ((packed));
        u_int8_t        rsv4:1,
                        tg_sharing_prohibited:1,
                        dedicated_scv:1,
                        rsv5:5                  __attribute__ ((packed));
        u_int8_t        rsv6[3]                 __attribute__ ((packed));
        u_int8_t        parallel_tg_sup:1,
                        dlur_actpu:1,
                        dlus_lu_reg:1,
                        xhpr_bn:1,
                        gen_odai_usage_opt_set:1,
                        rsv7:3                  __attribute__ ((packed));
        u_int8_t        tg_num                  __attribute__ ((packed));
        u_int8_t        dlc_type                __attribute__ ((packed));
        u_int8_t        dlc_len                 __attribute__ ((packed));
        u_int8_t        rsv8:1,
                        dlc_abm:1,
                        ls_role:2,
                        sh_mode_status:1,
                        sh_mode:1,
                        ls_txrx_cap:2           __attribute__ ((packed));
        u_int8_t        abm_nonact_xid:1,
                        rsv9:7                  __attribute__ ((packed));
        u_int16_t       max_btu_len             __attribute__ ((packed));
        u_int8_t       	rsv10                   __attribute__ ((packed));
        u_int8_t        rsv11:2,
                        dlc_init_mode:1,
                        rsv12:5                 __attribute__ ((packed));
        u_int16_t       rsv13                   __attribute__ ((packed));
        u_int8_t        max_iframes             __attribute__ ((packed));
	u_int8_t        rsv14                   __attribute__ ((packed));
#else
#error  "Please fix <asm/byteorder.h>"
#endif
} sna_xid3;
#pragma pack()

#endif  /* __KERNEL__ */
#endif  /* __NET_SNA_FORMATS_H */
