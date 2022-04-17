// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*****************************************************************************
 *
 *   Portable Transputer disassembler
 *
 *****************************************************************************/

/*
 *	Opcodes consist of two parts: the Direct Function and the Operand.
 *	The high 4 bits of an opcode are the Direct Function, the low 4 bits are the Operand.
 *
 *	The Prefix is a special Direct Function that allows instructions to be built from
 *	multiple opcodes.
 */

#ifndef MAME_CPU_INMOS_TRANSPUTER_DASM_H
#define MAME_CPU_INMOS_TRANSPUTER_DASM_H

#pragma once

#include "transputer_ops.h"

class transputer_disassembler : public util::disasm_interface
{
public:
	transputer_disassembler() = default;
	virtual ~transputer_disassembler() = default;

	virtual u32 opcode_alignment() const override;
	virtual offs_t disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params) override;

private:
	void get_operation_mnemonic(char *mnemonic, uint32_t operation_code);
};

#endif
