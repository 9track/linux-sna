/*****************************************************************************/
/* LUA VERB TYPES                                                            */
/*****************************************************************************/
#define LUA_VERB_RUI                    (0x0052)

/*****************************************************************************/
/* LUA OPCODES ( RUI and SLI )                                               */
/*****************************************************************************/
#define LUA_OPCODE_RUI_INIT             (0x8001)
#define LUA_OPCODE_RUI_TERM             (0x8002)
#define LUA_OPCODE_RUI_READ             (0x8003)
#define LUA_OPCODE_RUI_WRITE            (0x8004)
#define LUA_OPCODE_RUI_PURGE            (0x8005)
#define LUA_OPCODE_RUI_BID              (0x8006)
#define LUA_OPCODE_RUI_INIT_STATUS      (0x8007)
#define LUA_OPCODE_RUI_REINIT           (0x8008)

/*****************************************************************************/
/* LUA PRIMARY RETURN CODES                                                  */
/*****************************************************************************/
#define LUA_OK                          (0x0000)
#define LUA_PARAMETER_CHECK             (0x0100)
#define LUA_STATE_CHECK                 (0x0200)
#define LUA_SESSION_FAILURE             (0x0F00)
#define LUA_UNSUCCESSFUL                (0x1400)
#define LUA_NEGATIVE_RSP                (0x1800)
#define LUA_CANCELLED                   (0x2100)
#define LUA_IN_PROGRESS                 (0x3000)
#define LUA_STATUS                      (0x4000)
#define LUA_COMM_SUBSYSTEM_ABENDED      (0x03F0)
#define LUA_COMM_SUBSYSTEM_NOT_LOADED   (0x04F0)
#define LUA_INVALID_VERB_SEGMENT        (0x08F0)
#define LUA_UNEXPECTED_DOS_ERROR        (0x11F0)
#define LUA_STACK_TOO_SMALL             (0x15F0)
#define LUA_INVALID_VERB                (0xFFFF)

/*****************************************************************************/
/* LUA SECONDARY RETURN CODES                                                */
/*****************************************************************************/
#define LUA_SEC_RC_OK                   (0x00000000L)
#define LUA_INVALID_LUNAME              (0x01000000L)
#define LUA_BAD_SESSION_ID              (0x02000000L)
#define LUA_DATA_TRUNCATED              (0x03000000L)
#define LUA_BAD_DATA_PTR                (0x04000000L)
#define LUA_DATA_SEG_LENGTH_ERROR       (0x05000000L)
#define LUA_RESERVED_FIELD_NOT_ZERO     (0x06000000L)
#define LUA_INVALID_POST_HANDLE         (0x07000000L)
#define LUA_PURGED                      (0x0C000000L)
#define LUA_BID_VERB_SEG_ERROR          (0x0F000000L)
#define LUA_NO_PREVIOUS_BID_ENABLED     (0x10000000L)
#define LUA_NO_DATA                     (0x11000000L)
#define LUA_BID_ALREADY_ENABLED         (0x12000000L)
#define LUA_VERB_RECORD_SPANS_SEGMENTS  (0x13000000L)
#define LUA_INVALID_FLOW                (0x14000000L)
#define LUA_NOT_ACTIVE                  (0x15000000L)
#define LUA_VERB_LENGTH_INVALID         (0x16000000L)
#define LUA_REQUIRED_FIELD_MISSING      (0x19000000L)
#define LUA_READY                       (0x30000000L)
#define LUA_NOT_READY                   (0x31000000L)
#define LUA_INIT_COMPLETE               (0x32000000L)
#define LUA_SESSION_END_REQUESTED       (0x33000000L)
#define LUA_NO_SLI_SESSION              (0x34000000L)
#define LUA_SESSION_ALREADY_OPEN        (0x35000000L)
#define LUA_INVALID_OPEN_INIT_TYPE      (0x36000000L)
#define LUA_INVALID_OPEN_DATA           (0x37000000L)
#define LUA_UNEXPECTED_SNA_SEQUENCE     (0x38000000L)
#define LUA_NEG_RSP_FROM_BIND_ROUTINE   (0x39000000L)
#define LUA_NEG_RSP_FROM_CRV_ROUTINE    (0x3A000000L)
#define LUA_NEG_RSP_FROM_STSN_ROUTINE   (0x3B000000L)
#define LUA_CRV_ROUTINE_REQUIRED        (0x3C000000L)
#define LUA_STSN_ROUTINE_REQUIRED       (0x3D000000L)
#define LUA_INVALID_OPEN_ROUTINE_TYPE   (0x3E000000L)
#define LUA_MAX_NUMBER_OF_SENDS         (0x3F000000L)
#define LUA_SEND_ON_FLOW_PENDING        (0x40000000L)
#define LUA_INVALID_MESSAGE_TYPE        (0x41000000L)
#define LUA_RECEIVE_ON_FLOW_PENDING     (0x42000000L)
#define LUA_DATA_LENGTH_ERROR           (0x43000000L)
#define LUA_CLOSE_PENDING               (0x44000000L)
#define LUA_NEGATIVE_RSP_CHASE          (0x46000000L)
#define LUA_NEGATIVE_RSP_SHUTC          (0x47000000L)
#define LUA_NEGATIVE_RSP_RSHUTD         (0x48000000L)
#define LUA_NO_RECEIVE_TO_PURGE         (0x4A000000L)
#define LUA_CANCEL_COMMAND_RECEIVED     (0x4D000000L)
#define LUA_RUI_WRITE_FAILURE           (0x4E000000L)
#define LUA_SLI_BID_PENDING             (0x51000000L)
#define LUA_SLI_PURGE_PENDING           (0x52000000L)
#define LUA_PROCEDURE_ERROR             (0x53000000L)
#define LUA_INVALID_SLI_ENCR_OPTION     (0x54000000L)
#define LUA_RECEIVED_UNBIND             (0x55000000L)
#define LUA_SLI_LOGIC_ERROR             (0x7F000000L)
#define LUA_TERMINATED                  (0x80000000L)
#define LUA_NO_RUI_SESSION              (0x81000000L)
#define LUA_DUPLICATE_RUI_INIT          (0x82000000L)
#define LUA_INVALID_PROCESS             (0x83000000L)
#define LUA_API_MODE_CHANGE             (0x85000000L)
#define LUA_COMMAND_COUNT_ERROR         (0x87000000L)
#define LUA_NO_READ_TO_PURGE            (0x88000000L)
#define LUA_MULTIPLE_WRITE_FLOWS        (0x89000000L)
#define LUA_DUPLICATE_READ_FLOW         (0x8A000000L)
#define LUA_DUPLICATE_WRITE_FLOW        (0x8B000000L)
#define LUA_LINK_NOT_STARTED            (0x8C000000L)
#define LUA_INVALID_ADAPTER             (0x8D000000L)
#define LUA_ENCR_DECR_LOAD_ERROR        (0x8E000000L)
#define LUA_ENCR_DECR_PROC_ERROR        (0x8F000000L)
#define LUA_INVALID_PUNAME              (0x90000000L)
#define LUA_INVALID_LUNUMBER            (0x91000000L)
#define LUA_INVALID_FORMAT              (0x92000000L)
#define LUA_NEG_NOTIFY_RSP              (0xBE000000L)
#define LUA_RUI_LOGIC_ERROR             (0xBF000000L)
#define LUA_LU_INOPERATIVE              (0xFF000000L)

/*****************************************************************************/
/* SNAP-IX V4 had a typo with this secondary return code. Maintain this      */
/* error for V% but also provide the corrected version (above).              */
/*****************************************************************************/
#define LUA_INVALID_ADAPTED             LUA_INVALID_ADAPTER

/*****************************************************************************/
/* THE FOLLOWING SECONDARY RETURN CODES ARE SNA SENSE CODES                  */
/*****************************************************************************/
#define LUA_NON_UNIQ_ID                 (0x011000C0L)
#define LUA_NON_UNIQ_NAU_AD             (0x021000C0L)
#define LUA_INV_NAU_ADDR                (0x012000C0L)
#define LUA_INV_ADPT_NUM                (0x022000C0L)

#define LUA_RESOURCE_NOT_AVAILABLE      (0x00000108L)
#define LUA_SESSION_LIMIT_EXCEEDED      (0x00000508L)
#define LUA_SLU_SESSION_LIMIT_EXCEEDED  (0x0A000508L)
#define LUA_MODE_INCONSISTENCY          (0x00000908L)
#define LUA_BRACKET_RACE_ERROR          (0x00000B08L)
#define LUA_INSUFFICIENT_RESOURCES      (0x00001208L)
#define LUA_BB_REJECT_NO_RTR            (0x00001308L)
#define LUA_BB_REJECT_RTR               (0x00001408L)
#define LUA_RECEIVER_IN_TRANSMIT_MODE   (0x00001B08L)
#define LUA_REQUEST_NOT_EXECUTABLE      (0x00001C08L)
#define LUA_INVALID_SESSION_PARAMETERS  (0x00002108L)
#define LUA_UNIT_OF_WORK_ABORTED        (0x00002408L)
#define LUA_FM_FUNCTION_NOT_SUPPORTED   (0x00002608L)
#define LUA_LU_COMPONENT_DISCONNECTED   (0x00003108L)
#define LUA_INVALID_PARAMETER_FLAGS     (0x00003308L)
#define LUA_INVALID_PARAMETER           (0x00003508L)
#define LUA_NEGOTIABLE_BIND_ERROR       (0x01003508L)
#define LUA_BIND_FM_PROFILE_ERROR       (0x02003508L)
#define LUA_BIND_TS_PROFILE_ERROR       (0x03003508L)
#define LUA_BIND_LU_TYPE_ERROR          (0x0E003508L)
#define LUA_CRYPTOGRAPHY_INOPERATIVE    (0x00004808L)
#define LUA_REQ_RESOURCES_NOT_AVAIL     (0x00004B08L)
#define LUA_SSCP_LU_SESSION_NOT_ACTIVE  (0x00005708L)
#define LUA_SYNC_EVENT_RESPONSE         (0x00006708L)
#define LUA_REC_CORR_TABLE_FULL         (0x01007808L)
#define LUA_SEND_CORR_TABLE_FULL        (0x02007808L)
#define LUA_SESSION_SERVICE_PATH_ERROR  (0x00007D08L)

#define LUA_RU_DATA_ERROR               (0x00000110L)
#define LUA_RU_LENGTH_ERROR             (0x00000210L)
#define LUA_FUNCTION_NOT_SUPPORTED      (0x00000310L)
#define LUA_HDX_BRACKET_STATE_ERROR     (0x21010510L)
#define LUA_RESPONSE_ALREADY_SENT       (0x22010510L)
#define LUA_EXR_SENSE_INCORRECT         (0x23010510L)
#define LUA_RESPONSE_OUT_OF_ORDER       (0x24010510L)
#define LUA_CHASE_RESPONSE_REQUIRED     (0x25010510L)
#define LUA_CATEGORY_NOT_SUPPORTED      (0x00000710L)

#define LUA_INCORRECT_SEQUENCE_NUMBER   (0x00000120L)
#define LUA_CHAINING_ERROR              (0x00000220L)
#define LUA_BRACKET                     (0x00000320L)
#define LUA_DIRECTION                   (0x00000420L)
#define LUA_DATA_TRAFFIC_RESET          (0x00000520L)
#define LUA_DATA_TRAFFIC_QUIESCED       (0x00000620L)
#define LUA_DATA_TRAFFIC_NOT_RESET      (0x00000720L)
#define LUA_NO_BEGIN_BRACKET            (0x00000820L)
#define LUA_SC_PROTOCOL_VIOLATION       (0x00000920L)
#define LUA_IMMEDIATE_REQ_MODE_ERROR    (0x00000A20L)
#define LUA_QUEUED_RESPONSE_ERROR       (0x00000B20L)
#define LUA_ERP_SYNC_EVENT_ERROR        (0x00000C20L)
#define LUA_RSP_BEFORE_SENDING_REQ      (0x00000D20L)
#define LUA_RSP_CORRELATION_ERROR       (0x00000E20L)
#define LUA_RSP_PROTOCOL_ERROR          (0x00000F20L)

#define LUA_INVALID_SC_OR_NC_RH         (0x00000140L)
#define LUA_BB_NOT_ALLOWED              (0x00000340L)
#define LUA_EB_NOT_ALLOWED              (0x00000440L)
#define LUA_EXCEPTION_RSP_NOT_ALLOWED   (0x00000640L)
#define LUA_DEFINITE_RSP_NOT_ALLOWED    (0x00000740L)
#define LUA_PACING_NOT_SUPPORTED        (0x00000840L)
#define LUA_CD_NOT_ALLOWED              (0x00000940L)
#define LUA_NO_RESPONSE_NOT_ALLOWED     (0x00000A40L)
#define LUA_CHAINING_NOT_SUPPORTED      (0x00000B40L)
#define LUA_BRACKETS_NOT_SUPPORTED      (0x00000C40L)
#define LUA_CD_NOT_SUPPORTED            (0x00000D40L)
#define LUA_INCORRECT_USE_OF_FI         (0x00000F40L)
#define LUA_ALTERNATE_CODE_NOT_SUPPORT  (0x00001040L)
#define LUA_INCORRECT_RU_CATEGORY       (0x00001140L)
#define LUA_INCORRECT_REQUEST_CODE      (0x00001240L)
#define LUA_INCORRECT_SPEC_OF_SDI_RTI   (0x00001340L)
#define LUA_INCORRECT_DR1I_DR2I_ERI     (0x00001440L)
#define LUA_INCORRECT_USE_OF_QRI        (0x00001540L)
#define LUA_INCORRECT_USE_OF_EDI        (0x00001640L)
#define LUA_INCORRECT_USE_OF_PDI        (0x00001740L)

#define LUA_NAU_INOPERATIVE             (0x00000380L)
#define LUA_NO_SESSION                  (0x00000580L)

/*****************************************************************************/
/* LUA_RH.RUC masks                                                          */
/*****************************************************************************/
#define LUA_RH_FMD                       0
#define LUA_RH_NC                        1
#define LUA_RH_DFC                       2
#define LUA_RH_SC                        3

/*****************************************************************************/
/* LUA MESSAGE TYPES                                                         */
/*****************************************************************************/
#define LUA_MESSAGE_TYPE_LU_DATA          0x01
#define LUA_MESSAGE_TYPE_SSCP_DATA        0x11
#define LUA_MESSAGE_TYPE_RSP              0x02
#define LUA_MESSAGE_TYPE_BID              0xC8
#define LUA_MESSAGE_TYPE_BIND             0x31
#define LUA_MESSAGE_TYPE_BIS              0x70
#define LUA_MESSAGE_TYPE_CANCEL           0x83
#define LUA_MESSAGE_TYPE_CHASE            0x84
#define LUA_MESSAGE_TYPE_CLEAR            0xA1
#define LUA_MESSAGE_TYPE_CRV              0xD0
#define LUA_MESSAGE_TYPE_LUSTAT_LU        0x04
#define LUA_MESSAGE_TYPE_LUSTAT_SSCP      0x14
#define LUA_MESSAGE_TYPE_QC               0x81
#define LUA_MESSAGE_TYPE_QEC              0x80
#define LUA_MESSAGE_TYPE_RELQ             0x82
#define LUA_MESSAGE_TYPE_RQR              0xA3
#define LUA_MESSAGE_TYPE_RSHUTD           0xC2
#define LUA_MESSAGE_TYPE_RTR              0x05
#define LUA_MESSAGE_TYPE_SBI              0x71
#define LUA_MESSAGE_TYPE_SHUTC            0xC1
#define LUA_MESSAGE_TYPE_SHUTD            0xC0
#define LUA_MESSAGE_TYPE_SIGNAL           0xC9
#define LUA_MESSAGE_TYPE_SDT              0xA0
#define LUA_MESSAGE_TYPE_STSN             0xA2
#define LUA_MESSAGE_TYPE_UNBIND           0x32

/*****************************************************************************/
/* LUA VERB RECORD STRUCTURES                                                */
/*****************************************************************************/

/**STRUCT+********************************************************************/
/* Structure: struct LUA_COMMON                                              */
/*                                                                           */
/* Desription: Common header for all LUA verbs                               */
/*                                                                           */
/* Note that fields are defined in terms of their length, rather than in OS  */
/* specific types, APART FROM lua_post_handle.  This has always been defined */
/* as an unsigned long, but used for a pointer to a callback routine.  If    */
/* the definition is changes to an AP_UINT32 it will not work on 64-bit      */
/* platforms.  Ideally it would be changed to a void*, but this is not done  */
/* for backwards compatability reasons - it is left as it was.               */
/*                                                                           */
/* Note also that lua_data_ptr is defined as an NB_HANDLE in the APPN        */
/* version of this structure, but this (external) definition must not be     */
/* changed for reasons of back compatability.  The lua library copes with    */
/* the conversion (where required).                                          */
/*****************************************************************************/
struct LUA_COMMON
{
  unsigned short lua_verb;                    /* Verb Code                   */
  unsigned short lua_verb_length;             /* Length of Verb Record       */
  unsigned short lua_prim_rc;                 /* Primary Return Code         */
  unsigned long  lua_sec_rc;                  /* Secondary Return Code       */
  unsigned short lua_opcode;                  /* Verb Operation Code         */
  unsigned long  lua_correlator;              /* User Correlation Field      */
  unsigned char  lua_luname[8];               /* Local LU Name               */
  unsigned short lua_extension_list_offset;   /* Offset of DLL Extention List*/
  unsigned short lua_cobol_offset;            /* Offset of Cobol Extension   */
  unsigned long  lua_sid;                     /* Session ID                  */
  unsigned short lua_max_length;              /* Receive Buffer Length       */
  unsigned short lua_data_length;             /* Data Length                 */
  char          *lua_data_ptr;                /* Data Buffer Pointer         */
  unsigned long  lua_post_handle;             /* Posting handle              */

  struct LUA_TH                               /* LUA TH Fields               */
  {
    unsigned         flags_fid  : 4;          /* Format Identification Type 2*/
    unsigned         flags_mpf  : 2;          /* Segmenting Mapping Field    */
    unsigned         flags_odai : 1;          /* OAF-DAF Assignor Indicator  */
    unsigned         flags_efi  : 1;          /* Expedited Flow Indicator    */
    unsigned                    : 8;          /* Reserved Field              */
    unsigned char    daf;                     /* Destination Address Field   */
    unsigned char    oaf;                     /* Originating Address Field   */
    unsigned char    snf[2];                  /* Sequence Number Field       */
  } lua_th;

  struct LUA_RH                               /* LUA RH Fields               */
  {
    unsigned         rri  : 1;                /* Request-Response Indicator  */
    unsigned         ruc  : 2;                /* RU Category                 */
    unsigned              : 1;                /* Reserved Field              */
    unsigned         fi   : 1;                /* Format Indicator            */
    unsigned         sdi  : 1;                /* Sense Data Included Ind     */
    unsigned         bci  : 1;                /* Begin Chain Indicator       */
    unsigned         eci  : 1;                /* End Chain Indicator         */

    unsigned         dr1i : 1;                /* DR 1 Indicator              */
    unsigned              : 1;                /* Reserved Field              */
    unsigned         dr2i : 1;                /* DR 2 Indicator              */
    unsigned         ri   : 1;                /* Response Indicator          */
    unsigned              : 2;                /* Reserved Field              */
    unsigned         qri  : 1;                /* Queued Response Indicator   */
    unsigned         pi   : 1;                /* Pacing Indicator            */

    unsigned         bbi  : 1;                /* Begin Bracket Indicator     */
    unsigned         ebi  : 1;                /* End Bracket Indicator       */
    unsigned         cdi  : 1;                /* Change Direction Indicator  */
    unsigned              : 1;                /* Reserved Field              */
    unsigned         csi  : 1;                /* Code Selection Indicator    */
    unsigned         edi  : 1;                /* Enciphered Data Indicator   */
    unsigned         pdi  : 1;                /* Padded Data Indicator       */
    unsigned              : 1;                /* Reserved Field              */
  } lua_rh;

  struct LUA_FLAG1                            /* LUA_FLAG1                   */
  {
    unsigned       bid_enable  : 1;           /* Bid Enabled Indicator       */
    unsigned       reserv1     : 1;           /* reserved                    */
    unsigned       close_abend : 1;           /* Close Immediate Flag        */
    unsigned       nowait      : 1;           /* Don't Wait for Data Flag    */
    unsigned       sscp_exp    : 1;           /* SSCP expedited flow         */
    unsigned       sscp_norm   : 1;           /* SSCP normal flow            */
    unsigned       lu_exp      : 1;           /* LU expedited flow           */
    unsigned       lu_norm     : 1;           /* lu normal flow              */
  } lua_flag1;

  unsigned char lua_message_type;             /* sna message command type    */

  struct LUA_FLAG2                            /* LUA_FLAG2                   */
  {
    unsigned       bid_enable  : 1;           /* Bid Enabled Indicator       */
    unsigned       async       : 1;           /* flags asynchronous verb
                                                 completion                  */
    unsigned                   : 2;           /* reserved                    */
    unsigned       sscp_exp    : 1;           /* SSCP expedited flow         */
    unsigned       sscp_norm   : 1;           /* SSCP normal flow            */
    unsigned       lu_exp      : 1;           /* LU expedited flow           */
    unsigned       lu_norm     : 1;           /* lu normal flow              */
  } lua_flag2;

  unsigned char lua_resv56[7];                /* Reserved Field              */
  unsigned char lua_encr_decr_option;         /* Cryptography Option         */
} ;
/**STRUCT-********************************************************************/


/**STRUCT+********************************************************************/
/* Structure: struct SLI_OPEN                                                */
/*                                                                           */
/* Desription: Specific fields for the SLI_OPEN verb                         */
/*****************************************************************************/
struct SLI_OPEN
{
  unsigned char  lua_init_type;               /* Type of Session Initiation  */
  unsigned char  lua_resv65;                  /* Reserved Field              */
  unsigned short lua_wait;                    /* Secondary Retry Wait Time   */

  struct LUA_EXT_ENTRY
  {
    unsigned char lua_routine_type;           /* Extension Routine Type      */
    unsigned char lua_module_name[9];         /* Extension Module Name       */
    unsigned char lua_procedure_name[33];     /* Extension Procedure Name    */
  } lua_open_extension[3];
  unsigned char lua_ending_delim;             /* Extension List Delimiter    */
} ;
/**STRUCT-********************************************************************/

/**STRUCT+********************************************************************/
/* Structure: RUI_INIT                                                       */
/*                                                                           */
/* Description: Specific fields for the RUI_INIT verb                        */
/*****************************************************************************/

struct RUI_INIT
{
  unsigned char rui_init_format;              /* verb format                 */
  unsigned char lua_puname[8];                /* PU name                     */
  unsigned char lua_lunumber;                 /* LU number                   */
} ;

/**STRUCT-********************************************************************/

/*****************************************************************************/
/* LUA SPECIFIC FIELDS FOR THE SLI_OPEN, SLI_SEND, RUI_BID AND SLI_BID VERBS */
/*****************************************************************************/
union LUA_SPECIFIC
{
  struct   SLI_OPEN       open;
  unsigned char           lua_sequence_number[2];     /* sequence number     */
  unsigned char           lua_peek_data[12];          /* Data Pending        */
  struct   RUI_INIT       init;
} ;

/**STRUCT+********************************************************************/
/* Structure: LUA_VERB_RECORD                                                */
/*                                                                           */
/* Desription: Generic LUA verb record                                       */
/*****************************************************************************/
typedef struct
{
  struct LUA_COMMON       common;     /* common verb header                  */
  union  LUA_SPECIFIC     specific;   /* command specific portion of record  */
} LUA_VERB_RECORD;
/**STRUCT-********************************************************************/

/*****************************************************************************/
/* RUI ENTRY POINT DECLARATION                                               */
/*****************************************************************************/

extern int fake(void)
{
	return (0);
}

#define SNA_GET_FD()		fake()
#define SNA_EVENT_FD()		fake()
#define SNA_USE_FD_SCHED()	fake()

extern void rui1(void *x)
{
	/* Nothing */
}

extern void rui_sem1();

#define  RUI(X)              rui1(X)
#define  RUI_SEM(X,Y)        rui_sem1(X,Y)
