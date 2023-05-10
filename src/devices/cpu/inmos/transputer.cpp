// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

	transputer.cpp
	T8-series CPU implementation.

	The top-of-the-line transputer.

	TODO:
	- everything

	NOTES:
	- Wptr is Wdesc with b0 masked. b0 is the priority bit.
***************************************************************************/

#include "emu.h"
#include "transputer.h"
#include "transputer_dasm.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define LOG_LINK        (1U << 1)
#define LOG_BOOTSTRAP   (1U << 2)
#define LOG_SERIAL		(1U << 3)
#define VERBOSE (LOG_GENERAL|LOG_LINK|LOG_BOOTSTRAP|LOG_SERIAL)

#include "logmacro.h"

#include "transputer_bootstrap.h"

transputer_cpu_device::transputer_cpu_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock,
											 int cpu_type, u32 word_width)
	: cpu_device(mconfig, type, tag, owner, clock)
	, device_inmos_serial_link_interface(mconfig, *this)
	, m_program_config("program", ENDIANNESS_LITTLE, word_width, word_width, 0, 
						address_map_constructor(FUNC(transputer_cpu_device::internal_map), this))
	, m_error_cb(*this)
	, m_internal_sram(*this, "internal_sram")
{
}

device_memory_interface::space_config_vector transputer_cpu_device::memory_space_config() const
{
	return space_config_vector {
		std::make_pair(AS_PROGRAM, &m_program_config)
	};
}

/***************************************************************************
	CORE INITIALIZATION
***************************************************************************/

#include "transputer_operations.h"
#include "transputer_link.h"

uint8_t transputer_cpu_device::fetch()
{
	return memory.read_byte(m_IPTR);
}

void transputer_cpu_device::execute_run()
{  
	// Anything we need to do before execution?
	internal_update(total_cycles());

	while(m_bcount && m_icount <= m_bcount)
		internal_update(total_cycles() + m_icount - m_bcount);

	// TODO: Operations are not cycle-accurate.
	do
	{
		// Update the debugger state.
		debugger_instruction_hook(m_IPTR);

		// Update high and low priority clock registers.
		if(total_cycles() >= (m_last_timer_tick + cycles_per_microsecond()))
		{
			// 1uS per tick.
			m_ClockReg0++;
			if(m_ClockReg0 == transputer_ops::MostPos) m_ClockReg0 = transputer_ops::MostNeg;
			
			// 64 uSec per tick.
			m_low_priority_usec_to_go--;
			if(m_low_priority_usec_to_go == 0)
			{
				m_ClockReg1++;
				if(m_ClockReg1 == transputer_ops::MostPos) m_ClockReg1 = transputer_ops::MostNeg;
				m_low_priority_usec_to_go = 64;
			}

			m_last_timer_tick = total_cycles();
		}

		// Check the process scheduler.

		// CPU running or bootstrapping?
		if(m_cpu_state == CPU_RUNNING)
		{
			uint8_t opcode = fetch();
			m_icount -= execute_one(opcode);
		}
		else
		{
			bootstrap_step();
			m_links[0].rx_byte_present = false;
			m_icount--;
		}

		if(m_icount > 0)
			while(m_bcount && m_icount <= m_bcount)
				internal_update(total_cycles() + m_icount - m_bcount);

		
	} while (m_icount > 0);
}

std::unique_ptr<util::disasm_interface> transputer_cpu_device::create_disassembler()
{
	return std::make_unique<transputer_disassembler>();
}

/***************************************************************************
	COMMON RESET
***************************************************************************/
void transputer_cpu_device::device_start()
{
	m_last_timer_tick = 0;
	m_low_priority_usec_to_go = 64;
	m_event_step = 100;

	space(AS_PROGRAM).specific(memory);

	set_icountptr(m_icount);
	
	state_add(STATE_GENPCBASE, "CURPC",m_IPTR).callimport().noshow();
	state_add(TRANSPUTER_IPTR, "Iptr", m_IPTR).formatstr("%08X");
	state_add(TRANSPUTER_FPTR0, "Fptr0", m_FPTR0).formatstr("%08X");
	state_add(TRANSPUTER_BPTR0, "Bptr0", m_BPTR0).formatstr("%08X");
	state_add(TRANSPUTER_FPTR1, "Fptr1", m_FPTR1).formatstr("%08X");
	state_add(TRANSPUTER_BPTR1, "Bptr1", m_BPTR1).formatstr("%08X");
	state_add(TRANSPUTER_WDESC, "Wdesc", m_WDESC).formatstr("%08X");
	state_add(TRANSPUTER_AREG, "Areg", m_AREG).formatstr("%08X");
	state_add(TRANSPUTER_BREG, "Breg", m_BREG).formatstr("%08X");
	state_add(TRANSPUTER_CREG, "Creg", m_CREG).formatstr("%08X");
	state_add(TRANSPUTER_OREG, "Oreg", m_OREG).formatstr("%08X");
	state_add(TRANSPUTER_TIME0, "ClockReg0", m_ClockReg0).formatstr("%08X");
	state_add(TRANSPUTER_TIME1, "ClockReg1", m_ClockReg1).formatstr("%08X");
	state_add(RUN_STATE, "RUN", m_cpu_state).formatstr("%1u");
	//state_add(BOOTSTRAP_STATE, "BOOTSTRAP_STATE", m_bootstrap_status.state).formatstr("%d");

	m_is_analysed = false;
	m_boot_from_rom_in = false;
	m_analyse_in = false;
	m_reset_in = false;

	clear_error_flag();
}

void transputer_cpu_device::device_reset()
{
	for(int i=0; i<4; i++) m_links[i].reset();

	m_IPTR = 0x7FFFFFFE;

	if(m_boot_from_rom_in)
	{
		m_cpu_state = CPU_RUNNING;
	}
	else
	{
		m_cpu_state = CPU_WAITING_FOR_BOOTSTRAP;
	}
	m_bootstrap_status.state = transputer_bootstrap::STATE::WAITING_FOR_LENGTH_BYTE;
	m_bootstrap_status.command = transputer_bootstrap::COMMAND::WAIT;

	m_count_before_instruction_step = 0;
}

void transputer_cpu_device::device_add_mconfig(machine_config &config)
{
	RAM(config, m_internal_sram, 0);
	m_internal_sram->set_default_size("1K");
	m_internal_sram->set_default_value(0);
}

void transputer_cpu_device::device_resolve_objects()
{
	m_link_out_cb.resolve_all_safe();
	m_error_cb.resolve_safe();
}

void transputer_cpu_device::internal_map(address_map &map)
{
	// Transputers have 1KB of internal SRAM.
	map(0x80000000, 0x80000fff).ram().share("internal_sram");

	// Special register locations in low memory.
	map(0x80000000, 0x80000003).nopr().w(FUNC(transputer_cpu_device::link0_ccw_w));
	map(0x80000004, 0x80000007).nopr().w(FUNC(transputer_cpu_device::link1_ccw_w));
	map(0x80000008, 0x8000000b).nopr().w(FUNC(transputer_cpu_device::link2_ccw_w));
	map(0x8000000c, 0x8000000f).nopr().w(FUNC(transputer_cpu_device::link3_ccw_w));
	
	map(0x80000010, 0x80000013).r(FUNC(transputer_cpu_device::link0_ccw_r)).nopw();
	map(0x80000014, 0x80000017).r(FUNC(transputer_cpu_device::link1_ccw_r)).nopw();
	map(0x80000018, 0x8000001b).r(FUNC(transputer_cpu_device::link2_ccw_r)).nopw();
	map(0x8000001c, 0x8000001f).r(FUNC(transputer_cpu_device::link3_ccw_r)).nopw();

	map(0x80000020, 0x8000002f).rw(FUNC(transputer_cpu_device::Event_r), FUNC(transputer_cpu_device::Event_w));

	map(0x80000024, 0x80000027).rw(FUNC(transputer_cpu_device::TPtrLoc0_r), FUNC(transputer_cpu_device::TPtrLoc0_w));
	map(0x80000028, 0x8000002B).rw(FUNC(transputer_cpu_device::TPtrLoc1_r), FUNC(transputer_cpu_device::TPtrLoc1_w));

	map(0x8000002c, 0x8000002f).rw(FUNC(transputer_cpu_device::WdescIntSaveLoc_r), FUNC(transputer_cpu_device::WdescIntSaveLoc_w));
	map(0x80000030, 0x80000033).rw(FUNC(transputer_cpu_device::IptrIntSaveLoc_r), FUNC(transputer_cpu_device::IptrIntSaveLoc_w));
	map(0x80000034, 0x80000037).rw(FUNC(transputer_cpu_device::AregIntSaveLoc_r), FUNC(transputer_cpu_device::AregIntSaveLoc_w));
	map(0x80000038, 0x8000003b).rw(FUNC(transputer_cpu_device::BregIntSaveLoc_r), FUNC(transputer_cpu_device::BregIntSaveLoc_w));
	map(0x8000003c, 0x8000003f).rw(FUNC(transputer_cpu_device::CregIntSaveLoc_r), FUNC(transputer_cpu_device::CregIntSaveLoc_w));
	map(0x80000040, 0x80000043).rw(FUNC(transputer_cpu_device::STATUSIntSaveLoc_r), FUNC(transputer_cpu_device::STATUSIntSaveLoc_w));
	map(0x80000044, 0x80000047).rw(FUNC(transputer_cpu_device::EregIntSaveLoc_r), FUNC(transputer_cpu_device::EregIntSaveLoc_w));
}

/****************************
 *  Write lines
 ****************************/
WRITE_LINE_MEMBER(transputer_cpu_device::analyse_w)
{
	m_analyse_in = state;
	LOGMASKED(LOG_GENERAL, "analyse bit now %d\n", m_analyse_in);
}

WRITE_LINE_MEMBER(transputer_cpu_device::reset_w)
{
	bool previous_state = m_reset_in;

	if(previous_state == true && state == false)
	{
		// HIGH-LOW transition, so resetting.
		m_is_analysed = m_analyse_in; // ANALYSE & RESET == put CPU in analyse mode.
		device_reset();
	}

	m_reset_in = state;
	LOGMASKED(LOG_GENERAL, "reset bit now %d\n", m_reset_in);
}

WRITE_LINE_MEMBER(transputer_cpu_device::boot_from_rom_w)
{
	m_boot_from_rom_in = state;
}

/********************************************************
 * REGISTER ACCESSORS
 ********************************************************/
uint32_t transputer_cpu_device::TPtrLoc0_r()
{
	return m_TPtrLoc0;
}
uint32_t transputer_cpu_device::TPtrLoc1_r()
{
	return m_TPtrLoc1;
}
uint32_t transputer_cpu_device::Event_r()
{
	return m_event;
}
uint32_t transputer_cpu_device::WdescIntSaveLoc_r()
{
	return m_WdescIntSaveLoc;
}
uint32_t transputer_cpu_device::IptrIntSaveLoc_r()
{
	return m_IptrIntSaveLoc;
}
uint32_t transputer_cpu_device::AregIntSaveLoc_r()
{
	return m_AregIntSaveLoc;
}
uint32_t transputer_cpu_device::BregIntSaveLoc_r()
{
	return m_BregIntSaveLoc;
}
uint32_t transputer_cpu_device::CregIntSaveLoc_r()
{
	return m_CregIntSaveLoc;
}
uint32_t transputer_cpu_device::STATUSIntSaveLoc_r()
{
	return m_STATUSIntSaveLoc;
}
uint32_t transputer_cpu_device::EregIntSaveLoc_r()
{
	return m_EregIntSaveLoc;
}

void transputer_cpu_device::TPtrLoc0_w(uint32_t value)
{
	m_TPtrLoc0 = value;
}
void transputer_cpu_device::TPtrLoc1_w(uint32_t value)
{
	m_TPtrLoc0 = value;
}
void transputer_cpu_device::Event_w(uint32_t value)
{
	m_event = value;
}
void transputer_cpu_device::WdescIntSaveLoc_w(uint32_t value)
{
	m_WdescIntSaveLoc = value;
}
void transputer_cpu_device::IptrIntSaveLoc_w(uint32_t value)
{
	m_IptrIntSaveLoc = value;
}
void transputer_cpu_device::AregIntSaveLoc_w(uint32_t value)
{
	m_AregIntSaveLoc = value;
}
void transputer_cpu_device::BregIntSaveLoc_w(uint32_t value)
{
	m_BregIntSaveLoc = value;
}
void transputer_cpu_device::CregIntSaveLoc_w(uint32_t value)
{
	m_CregIntSaveLoc = value;
}
void transputer_cpu_device::STATUSIntSaveLoc_w(uint32_t value)
{
	m_STATUSIntSaveLoc = value;
}
void transputer_cpu_device::EregIntSaveLoc_w(uint32_t value)
{
	m_EregIntSaveLoc = value;
}
