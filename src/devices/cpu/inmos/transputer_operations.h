#ifndef MAME_CPU_INMOS_TRANSPUTER_OPERATIONS
#define MAME_CPU_INMOS_TRANSPUTER_OPERATIONS

int transputer_cpu_device::execute_one(int opcode)
{
	/*
	 *  F.3.1 - Decoding an Instruction
	 *  Instruction set definitions:
	 *  ByteMem: byte values at byte address
	 *      - ByteMem addresses can be unaligned.
	 *  Mem: word values at word addresses
	 *      - Mem addresses are word-aligned.
	 *  Workspace: word values at word offsets from Wptr
	 *      - Workspace addresses are word-aligned.
	 */

	// The return value is the number of cycles the instruction took.
	u8 direct_function = transputer_ops::get_direct_function(opcode);
	int cycles_elapsed = transputer_ops::direct_function_cycle_table[direct_function];

	// The high 4 bits of the opcode are a function code.
	// The low 4 bits are ORed with Oreg.
	m_OReg0 = m_OREG | (opcode & 0x0F);

	// Now we're at the state where we can execute the instruction.

	switch(direct_function)
	{
		// F.3.3 - Direct Functions
		case transputer_ops::DIRECT_J:   
		{   // 0
			OP_ClearOreg();
			m_IPTR = OP_ByteIndex(m_IPTR+1, Oreg0);
			// TODO: do a timeslice check
			break;
		}

		case transputer_ops::DIRECT_LDLP:
		{   // 1
			OP_StackPush(OP_Index(Wptr, Oreg0));
			OP_ClearOreg();
			OP_IptrAdvance();
			break;
		}

		case transputer_ops::DIRECT_PFIX:
		{   // 2
			OregPrime = Oreg0 << 4;
			m_IPTR++; // Explicitly 1.
			break;
		}

		case transputer_ops::DIRECT_LDNL:
		{   // 3
			m_AREG = OP_MaskByteSelector(m_AREG);
			m_AREG = memory.read_word(OP_Index(m_AREG, Oreg0));
			OP_ClearOreg();
			OP_IptrAdvance();
			break;
		}

		case transputer_ops::DIRECT_LDC:
		{   // 4
			OP_StackPush(Oreg0);
			OregPrime = 0;
			OP_IptrAdvance();
			break;
		}

		case transputer_ops::DIRECT_LDNLP:
		{   // 5
			m_AREG = OP_MaskByteSelector(m_AREG);
			m_AREG = OP_Index(m_AREG, Oreg0);
			OP_ClearOreg();
			OP_IptrAdvance();
			break;
		}

		case transputer_ops::DIRECT_NFIX:
		{   // 6
			m_OREG = Oreg0;
			m_OREG = ~m_OREG;
			m_OREG = m_OREG << 4;
			m_IPTR++; // Explicitly 1.
			break;
		}

		case transputer_ops::DIRECT_LDL:
		{   // 7
			OP_StackPush(OP_ReadWordFromWorkspace(Oreg0));
			OP_ClearOreg();
			OP_IptrAdvance();
			break;
		}

		case transputer_ops::DIRECT_ADC:
		{   // 8
			m_errorFlag = ((m_AREG + Oreg0) < m_AREG);
			m_AREG = m_AREG + Oreg0;
			OregPrime = 0;
			OP_IptrAdvance();
			break;
		}

		case transputer_ops::DIRECT_CALL:
		{
			// Areg' = NextInst
			// Oreg' = 0
			// Wptr' = Index Wptr (-4)
			// Iptr' = ByteIndex NextInst Oreg0
			// Shifted.Workspace+0 <- Iptr
			// Shifted.Workspace+1 <- Areg
			// Shifted.Workspace+2 <- Breg
			// Shifted.Workspace+3 <- Creg
			// 
			// where: Workspace addr = Shifted.Workspace addr+4
			
			m_AREG = OP_NextInst();
			OregPrime = 0;
			OP_UpdateWptr(OP_Index(Wptr, -4));
			m_IPTR = OP_ByteIndex(OP_NextInst(), Oreg0);

			// Build the call stack at Workspace+16? TODO: Verify
			OP_WriteWordToWorkspace(16+12, m_IPTR);
			OP_WriteWordToWorkspace(16+8, m_AREG);
			OP_WriteWordToWorkspace(16+4, m_BREG);
			OP_WriteWordToWorkspace(16+0, m_CREG);

			break;
		}

		case transputer_ops::DIRECT_CJ:
		{
			// A

			// Jump taken, 4 cycles. Not taken, 2 cycles.
			if(m_AREG == 0)
			{
				m_IPTR = OP_ByteIndex(OP_NextInst(), Oreg0);
				cycles_elapsed = 4;
			}
			else
			{
				OP_StackPop();
				OP_IptrAdvance();
				cycles_elapsed = 2;
			}
			OP_ClearOreg();
			break;
		}

		case transputer_ops::DIRECT_AJW:
		{   // B
			OP_UpdateWptr(OP_Index(Wptr, Oreg0));
			OP_ClearOreg();
			OP_IptrAdvance();
			break;
		}

		case transputer_ops::DIRECT_EQC:
		{   // C
			m_AREG = (m_AREG == Oreg0);
			OP_ClearOreg();
			OP_IptrAdvance();
			break;
		}

		case transputer_ops::DIRECT_STL:
		{   // D
			OP_WriteWordToWorkspace(Oreg0, m_AREG);
			OP_StackPop();
			OP_ClearOreg();
			OP_IptrAdvance();
			break;
		}

		case transputer_ops::DIRECT_STNL:
		{   // E
			m_AREG = OP_MaskByteSelector(m_AREG);
			memory.write_word(OP_Index(m_AREG, Oreg0), m_BREG);
			m_AREG = m_CREG;
			OP_ClearOreg();
			OP_IptrAdvance();
			break;
		}

		case transputer_ops::DIRECT_OPR:
		{
			// F
			cycles_elapsed = DoOperation();
			OP_ClearOreg(); // Always executed after an OPR.
		}

		default:
		break;
	}

	return cycles_elapsed;
}

int transputer_cpu_device::DoOperation()
{
	// OPR instruction dispatcher.
	// Perform the operation specified in Oreg0.
	
	int cycles = transputer_ops::ops_lookup[Oreg0].cycles;

	switch(Oreg0)
	{
		case 0x00:  // rev
		{
			u32 tmp = m_AREG;
			m_AREG = m_BREG;
			m_BREG = tmp;
			OP_IptrAdvance();
			break;
		}
		case 0x0F:  // outword
		{
			// CPU overwrites Workspace+0 with the output value.
			OP_WriteWordToWorkspace(0, m_AREG);

			// BREG contains the link output address.
			LOG("OUTWORD: writing %08X to channel pointed to by %08X\n", m_AREG, m_BREG);

			OP_IptrAdvance();
			break;
		}
		case 0x18:  // sthf
		{
			// Pop Areg into the high-priority list register.
			m_FPTR0 = OP_StackPop();
			OP_IptrAdvance();
			break;
		}
		case 0x1C:  // stlf
		{
			// Pop Areg into the low-priority list register.
			m_FPTR1 = OP_StackPop();
			OP_IptrAdvance();
			break;
		}
		case 0x42:  // mint
		{
			OP_StackPush(transputer_ops::MostNeg);
			OP_IptrAdvance();
			break;            
		}

		default:
		LOG("%s: OPR %08X not implemented\n", FUNCNAME, Oreg0);
		OP_IptrAdvance();
		break;
	}

	return cycles;
}

#endif