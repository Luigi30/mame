// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller
/*****************************************************************************
 *
 *   Portable Transputer disassembler
 *
 *
 *****************************************************************************/

#include "emu.h"
#include "transputer_dasm.h"

u32 transputer_disassembler::opcode_alignment() const
{
	return 1;
}

void transputer_disassembler::get_operation_mnemonic(char *mnemonic, uint32_t operation_code)
{   
    for(auto it = std::begin(transputer_ops::ops_lookup); it != std::end(transputer_ops::ops_lookup); ++it) {
        if(operation_code == (*it).code)
        {
            strcpy(mnemonic, (char *)((*it).mnemonic));
            return;
        }
    }
    
    strcpy(mnemonic, "ILLEGAL");
}

offs_t transputer_disassembler::disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params)
{
    static uint32_t collected_prefixes = 0;

    offs_t flags = 0;
	uint8_t op;
   	unsigned prevpc = pc;

    op = opcodes.r8(pc++);
    transputer_ops::FUNCTION_CODE direct_function = transputer_ops::get_direct_function(op);
    uint8_t operand = transputer_ops::get_operand(op);  // 4 bits

    bool clear_prefixes = true;

	switch (direct_function)
	{
        case transputer_ops::DIRECT_J:      util::stream_format(stream, "j     $%01X", operand); break;
        case transputer_ops::DIRECT_LDLP:   util::stream_format(stream, "ldlp  $%01X", operand); break;
        case transputer_ops::DIRECT_PFIX:   
        {   
            util::stream_format(stream, "pfix  $%01X", operand); 
            clear_prefixes = false; 
            collected_prefixes = (collected_prefixes << 4) | operand;
            break;  
        }
        case transputer_ops::DIRECT_LDNL:   util::stream_format(stream, "ldnl  $%01X", operand); break;
        case transputer_ops::DIRECT_LDC:    util::stream_format(stream, "ldc   $%01X", operand); break;
        case transputer_ops::DIRECT_LDNLP:  util::stream_format(stream, "ldnlp $%01X", operand); break;
        case transputer_ops::DIRECT_NFIX:
        {
            util::stream_format(stream, "nfix  $%01X", operand); 
            clear_prefixes = false; 
            collected_prefixes = (~(collected_prefixes << 4)) | operand;
            break;  
        }
        case transputer_ops::DIRECT_LDL:    util::stream_format(stream, "ldl   $%01X", operand); break;
        case transputer_ops::DIRECT_ADC:    util::stream_format(stream, "adc   $%01X", operand); break;
        case transputer_ops::DIRECT_CALL:   util::stream_format(stream, "call  $%01X", operand); break;
        case transputer_ops::DIRECT_CJ:     util::stream_format(stream, "cj    $%01X", operand); break;
        case transputer_ops::DIRECT_AJW:    util::stream_format(stream, "ajw   $%01X", operand); break;
        case transputer_ops::DIRECT_EQC:    util::stream_format(stream, "eqc   $%01X", operand); break;
        case transputer_ops::DIRECT_STL:    util::stream_format(stream, "stl   $%01X", operand); break;
        case transputer_ops::DIRECT_STNL:   util::stream_format(stream, "stnl  $%01X", operand); break;
        case transputer_ops::DIRECT_OPR:
        {
            collected_prefixes = (collected_prefixes << 4) | operand;
            char mnemonic[20];
            get_operation_mnemonic(mnemonic, collected_prefixes);
            util::stream_format(stream, "opr   $%01X [$%X, %s]", operand, collected_prefixes, mnemonic); break;
        }

        default:
        util::stream_format(stream, "opcode"); break;
    }

    if(clear_prefixes) collected_prefixes = 0;

    return (pc - prevpc) | flags | SUPPORTED;
}