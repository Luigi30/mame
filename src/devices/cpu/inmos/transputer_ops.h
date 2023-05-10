// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

#ifndef TRANSPUTER_OPS_H
#define TRANSPUTER_OPS_H

namespace transputer_ops
{
    // Table for direct functions #0 to #F.
    // -1 means it is not a constant value (CJ and OPR are not constant.)
    const static int direct_function_cycle_table[16] = {
        3, 1, 1, 2, 1, 1, 1, 2, 1, 7, -1, 1, 2, 1, 2, -1
    };

    typedef enum {
        DIRECT_J,
        DIRECT_LDLP,
        DIRECT_PFIX,
        DIRECT_LDNL,
        DIRECT_LDC,
        DIRECT_LDNLP,
        DIRECT_NFIX,
        DIRECT_LDL,
        DIRECT_ADC,
        DIRECT_CALL,
        DIRECT_CJ,
        DIRECT_AJW,
        DIRECT_EQC,
        DIRECT_STL,
        DIRECT_STNL,
        DIRECT_OPR
    } FUNCTION_CODE;

    inline transputer_ops::FUNCTION_CODE get_direct_function(u8 opcode) { return (transputer_ops::FUNCTION_CODE)(opcode >> 4); }
	inline u8 get_operand(u8 opcode) { return opcode & 0x0F; }

    static const uint32_t MostNeg = 0x80000000;
    static const uint32_t MostPos = 0x7FFFFFFF;
    static const uint32_t NotProcessP = MostNeg;

	// An OPERATION is an instruction that is dispatched by an OPERATE direct-function.
    typedef struct
    {
        uint32_t code;					// The opcode.
        bool is_long;					// I don't remember.
        char mnemonic[20];				// Operation mnemonic in the instruction set manual.
        uint8_t cycles;					// The base cycle count. If an Operation has variable timing, that's handled in execute_one.
        bool can_deschedule_process;	// Can this Operation deschedule a process?
        bool can_set_error_flag;		// Can this Operation set the transputer's error flag?
    } OPERATION;

    #define DEFINE_OPERATION(code, is_long, mnemonic, cycles, can_deschedule, can_error) { code, is_long, mnemonic, cycles, can_deschedule, can_error }

    const OPERATION ops_lookup[] = {
        DEFINE_OPERATION(0x00, false, "rev",    		1,	false,	false),
        DEFINE_OPERATION(0x01, false, "lb",     		1,	false,	false),
        DEFINE_OPERATION(0x02, false, "bsub",   		1,	false,	false),
        DEFINE_OPERATION(0x03, false, "endp",   		1,	true,	false),
        DEFINE_OPERATION(0x04, false, "diff",   		1,	false,	false),
        DEFINE_OPERATION(0x05, false, "add",    		1,	false,	true),
        DEFINE_OPERATION(0x06, false, "gcall",  		1,	false,	false),
        DEFINE_OPERATION(0x07, false, "in",     		1,	true,	false),
        DEFINE_OPERATION(0x08, false, "prod",   		1,	false,	false),
        DEFINE_OPERATION(0x09, false, "gt",     		1,	false,	false),
        DEFINE_OPERATION(0x0A, false, "wsub",   		1,	false,	false),
        DEFINE_OPERATION(0x0B, false, "out",    		1, 	true,	false),
        DEFINE_OPERATION(0x0C, false, "sub",    		1,	false,	true),
        DEFINE_OPERATION(0x0D, false, "startp", 		1,	false,	false),
        DEFINE_OPERATION(0x0E, false, "outbyte",		1, 	true,	false),
        DEFINE_OPERATION(0x0F, false, "outword",		1, 	true,	false),

        DEFINE_OPERATION(0x10, false, "seterr", 		1,	false,	true),
        DEFINE_OPERATION(0x12, false, "resetch",		1,	false,	false),
        DEFINE_OPERATION(0x13, false, "csub0",  		1,	false,	true),
        DEFINE_OPERATION(0x15, false, "stopp",  		1, 	true, 	false),
        DEFINE_OPERATION(0x16, false, "ladd",   		1,	false,	true),
        DEFINE_OPERATION(0x17, false, "stlb",   		1,	false,	false),
        DEFINE_OPERATION(0x18, false, "sthf",   		1,	false,	false),
        DEFINE_OPERATION(0x19, false, "norm",   		1,	false,	false),
        DEFINE_OPERATION(0x1A, false, "ldiv",   		1,	false,	true),
        DEFINE_OPERATION(0x1B, false, "ldpi",   		1,	false,	false),
        DEFINE_OPERATION(0x1C, false, "stlf",  			1,	false,	false),
        DEFINE_OPERATION(0x1D, false, "xdble",  		1,	false,	false),
        DEFINE_OPERATION(0x1E, false, "ldpri",  		1,	false,	false),
        DEFINE_OPERATION(0x1F, false, "rem",    		1,	false,	true),

        DEFINE_OPERATION(0x20, false, "ret", 			1,	false,	false),
        DEFINE_OPERATION(0x21, false, "lend",			1, 	true,	false),
        DEFINE_OPERATION(0x22, false, "ldtimer",		1,	false,	false),
        DEFINE_OPERATION(0x29, false, "testerr",   		1,	false,	false),
        DEFINE_OPERATION(0x2A, false, "testpranal",		1,	false,	false),
        DEFINE_OPERATION(0x2B, false, "tin",   			1, 	true,	false),
        DEFINE_OPERATION(0x2C, false, "div",   			1,	false,	true),
        DEFINE_OPERATION(0x2E, false, "dist",  			1,	false,	false),
        DEFINE_OPERATION(0x2F, false, "disc",    		1,	false,	false),

        DEFINE_OPERATION(0x30, false, "diss", 			1,	false,	false),
        DEFINE_OPERATION(0x31, false, "lmul",			1,	false,	false),
        DEFINE_OPERATION(0x32, false, "not",			1,	false,	false),
        DEFINE_OPERATION(0x33, false, "xor",  			1,	false,	false),
        DEFINE_OPERATION(0x34, false, "bcnt",			1,	false,	false),
        DEFINE_OPERATION(0x35, false, "lshr",  			1,	false,	false),
        DEFINE_OPERATION(0x36, false, "lshl",   		1,	false,	false),
        DEFINE_OPERATION(0x37, false, "lsum",   		1,	false,	false),
        DEFINE_OPERATION(0x38, false, "lsub",   		1,	false,	true),
        DEFINE_OPERATION(0x39, false, "runp",   		1,	false,	false),
        DEFINE_OPERATION(0x3A, false, "xword",   		1,	false,	false),
        DEFINE_OPERATION(0x3B, false, "sb",   			1,	false,	false),
        DEFINE_OPERATION(0x3C, false, "gajw",   		1,	false,	false),
        DEFINE_OPERATION(0x3D, false, "savel",  		1,	false,	false),
        DEFINE_OPERATION(0x3E, false, "saveh",  		1,	false,	false),
        DEFINE_OPERATION(0x3F, false, "wcnt",    		1,	false,	false),

        DEFINE_OPERATION(0x40, false, "shr",   			1,	false,	false),
        DEFINE_OPERATION(0x41, false, "shl",   			1,	false,	false),
        DEFINE_OPERATION(0x42, false, "mint",   		1,	false,	false),
        DEFINE_OPERATION(0x43, false, "alt",   			1,	false,	false),
        DEFINE_OPERATION(0x44, false, "altwt",   		1, 	true,	false),
        DEFINE_OPERATION(0x45, false, "altend",   		1,	false,	false),
        DEFINE_OPERATION(0x46, false, "and",  			1,	false,	false),
        DEFINE_OPERATION(0x47, false, "enbt",  			1,	false,	false),
        DEFINE_OPERATION(0x48, false, "enbc",			1,	false,	false),
        DEFINE_OPERATION(0x49, false, "enbs",   		1,	false,	false),
        DEFINE_OPERATION(0x4A, false, "move",   		1,	false,	false),
        DEFINE_OPERATION(0x4B, false, "or",   			1,	false,	false),
        DEFINE_OPERATION(0x4C, false, "csngl",   		1,	false,	true),
        DEFINE_OPERATION(0x4D, false, "ccnt1",   		1,	false,	true),
        DEFINE_OPERATION(0x4E, false, "talt",   		1,	false,	false),
        DEFINE_OPERATION(0x4F, false, "ldiff",   		1,	false,	false),

        DEFINE_OPERATION(0x50, false, "sthb",   		1,	false,	false),
        DEFINE_OPERATION(0x51, false, "taltwt",			1,	true,	false),
        DEFINE_OPERATION(0x52, false, "sum",   			1,	false,	false),
        DEFINE_OPERATION(0x53, false, "mul",   			1,	false,	false),
        DEFINE_OPERATION(0x54, false, "sttimer",   		1,	false,	false),
        DEFINE_OPERATION(0x55, false, "stoperr",   		1,	false,	false),
        DEFINE_OPERATION(0x56, false, "cword",   		1,	false,	true),
        DEFINE_OPERATION(0x57, false, "clrhalterr",		1,	false,	false),
        DEFINE_OPERATION(0x58, false, "sethalterr",		1,	false,	false),
        DEFINE_OPERATION(0x59, false, "testhalterr",	1, 	false,	false),
        DEFINE_OPERATION(0x5A, false, "dup",   			1,	false,	false),
        DEFINE_OPERATION(0x5B, false, "move2dinit",		1,	false,	false),
        DEFINE_OPERATION(0x5C, false, "move2dall",		1, 	false,	false),
        DEFINE_OPERATION(0x5D, false, "move2dnonzero",	1, 	false,	false),
        DEFINE_OPERATION(0x5E, false, "move2dzero",		1,	false,	false),

        DEFINE_OPERATION(0x63, false, "unpacksn",		1,	false,	false),
        DEFINE_OPERATION(0x6C, false, "postnormsn",		1,	false,	false),
        DEFINE_OPERATION(0x6D, false, "roundsn",		1,	false,	false),

        DEFINE_OPERATION(0x71, false, "ldinf",   		1,	false,	true),
        DEFINE_OPERATION(0x72, false, "fmul",   		1,	false,	false),
        DEFINE_OPERATION(0x73, false, "cflerr",   		1,	false,	true),
        DEFINE_OPERATION(0x74, false, "crcword",   		1,	false,	false),
        DEFINE_OPERATION(0x75, false, "crcbyte",   		1,	false,	false),
        DEFINE_OPERATION(0x76, false, "bitcnt",   		1,	false,	false),
        DEFINE_OPERATION(0x77, false, "bitrevword",		1,	false,	false),
        DEFINE_OPERATION(0x78, false, "bitrevnbits",	1,	false,	false),

        DEFINE_OPERATION(0x81, false, "wsubdb",   		1,	false,	false),
        DEFINE_OPERATION(0x82, false, "fpldnldbi",		1,	false,	false),
        DEFINE_OPERATION(0x83, false, "fpchkerr",   	1,	false,	false),
        DEFINE_OPERATION(0x84, false, "fpstnldb",   	1,	false,	false),
        DEFINE_OPERATION(0x86, false, "fpldnlsni",		1,	false,	false),
        DEFINE_OPERATION(0x87, false, "fpadd",			1,	false,	false),
        DEFINE_OPERATION(0x88, false, "fpstnlsn",		1,	false,	false),
        DEFINE_OPERATION(0x89, false, "fpsub",			1,	false,	false),
        DEFINE_OPERATION(0x8A, false, "fpldnldb",		1,	false,	false),
        DEFINE_OPERATION(0x8B, false, "fpmul",			1,	false,	false),
        DEFINE_OPERATION(0x8C, false, "fpdiv",			1,	false,	false),
        DEFINE_OPERATION(0x8E, false, "fpldnlsn",		1,	false,	false),
        DEFINE_OPERATION(0x8F, false, "fpremfirst",		1,	false,	false),

        DEFINE_OPERATION(0x90, false, "fpremstep",		1,	false,	false),
        DEFINE_OPERATION(0x91, false, "fpnan",			1,	false,	false),
        DEFINE_OPERATION(0x92, false, "fpordered",		1,	false,	false),
        DEFINE_OPERATION(0x93, false, "fpnotfinite",	1,	false,	false),
        DEFINE_OPERATION(0x94, false, "fpgt",   		1,	false,	false),
        DEFINE_OPERATION(0x95, false, "fpeq",   		1,	false,	false),
        DEFINE_OPERATION(0x96, false, "fpi32tor32",		1,	false,	false),
        DEFINE_OPERATION(0x98, false, "fpi32tor64",		1,	false,	false),
        DEFINE_OPERATION(0x9A, false, "fpb32tor64",		1,	false,	false),
        DEFINE_OPERATION(0x9C, false, "fptesterr",		1,	false,	false),
        DEFINE_OPERATION(0x9D, false, "fprtoi32",		1,	false,	false),
        DEFINE_OPERATION(0x9E, false, "fpstnli32",		1,	false,	false),
        DEFINE_OPERATION(0x9F, false, "fpldzerosn",		1,	false,	false),

        DEFINE_OPERATION(0xA0, false, "fpldzerodb",		1,	false,	false),
        DEFINE_OPERATION(0xA1, false, "fpint",   		1,	false,	false),
        DEFINE_OPERATION(0xA3, false, "fpdup",   		1,	false,	false),
        DEFINE_OPERATION(0xA4, false, "fprev",   		1,	false,	false),
        DEFINE_OPERATION(0xA6, false, "fpldnladddb",	1,	false,	false),
        DEFINE_OPERATION(0xA8, false, "fpldnlmuldb",	1,	false,	false),
        DEFINE_OPERATION(0xAA, false, "fpldnladdsn",	1,	false,	false),
        DEFINE_OPERATION(0xAB, false, "fpentry",		1,	false,	false),
        DEFINE_OPERATION(0xAC, false, "fpldnlmulsn",	1,	false,	false)
    };
}

#endif