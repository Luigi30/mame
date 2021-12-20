// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*
 * Motorola MVME320 VMEbus Disk Controller Module
 *
 * ST-506 and Shugart SA-400 disk controller, FM and MFM.
 * Supports 8" and 5.25" disk drives plus ST-506 hard drives.
 * 
 * Consists of a Signetics N8X305 running sequencer firmware and a bunch of TTL for controlling disks.
 *
 * There are three versions of the MVME320:
 * - MVME320: Has weird connectors and requires an MVME901 adapter board to connect to drives.
 * - MVME320/A: Simplified layout, standard connectors.
 * - MVME320/B: Adds support for 1.2M floppy drives.
 *  */

#include "emu.h"
#include "vme_mvme320.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define LOG_PRINTF  (1U << 1)
#define LOG_SETUP   (1U << 2)
#define LOG_GENERAL (1U << 3)

#define VERBOSE (LOG_PRINTF | LOG_SETUP | LOG_GENERAL)

#include "logmacro.h"

#define LOGPRINTF(...)  LOGMASKED(LOG_PRINTF,   __VA_ARGS__)
#define LOGSETUP(...)   LOGMASKED(LOG_SETUP,    __VA_ARGS__)
#define LOGGENERAL(...) LOGMASKED(LOG_GENERAL,  __VA_ARGS__)

#define VSR1_RSTVECT	0x01 // Software-controlled reset vector. If asserted, on reset the MCU doesn't take a branch at $0004.
#define VSR1_RGMND		0x02 // ?
#define VSR1_IACKR		0x04 // Interrupt Acknowledge Read? Another reset vector, this time at $0002.
#define VSR1_WRITE		0x08 // VME /WRITE is asserted
#define VSR1_EXA01		0x10 // VME A01 is asserted
#define VSR1_EXA02		0x20 // VME A02 is asserted
#define VSR1_EXA03		0x40 // VME A03 is asserted
#define VSR1_1			0x80 // Always 1

#define VSR2_ACFAIL		0x01 // Power failure
#define VSR2_BCLR		0x02 // Bus arbitrator demanding current master give up the bus
#define VSR2_LBERRn		0x04 // Local BERR
#define VSR2_CYACTIV	0x08 // VME bus cycle is active
#define VSR2_SYSRESETn 	0x10 // VME SYSRESET
#define VSR2_EXA01		0x20 // VME A01 is asserted
#define VSR2_EXA03		0x40 // VME A03 is asserted
#define VSR2_EXA02		0x80 // VME A02 is asserted

void vme_control_line_log(char *logdata, uint8_t data);
void vme_dbc3_line_log(char *logdata, uint8_t data);

DEFINE_DEVICE_TYPE(VME_MVME320, vme_mvme320_card_device, "mvme320", "Motorola MVME320/B Disk Controller")

vme_mvme320_device::vme_mvme320_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock)
	, device_vme_card_interface(mconfig, *this)
	, m_maincpu(*this, "maincpu")
	, m_fd0(*this, "floppy0")
	, m_dbcr(*this, "dbcr")
	, m_in_sc_cb(*this)
	, m_in_wc_cb(*this)
	, m_in_lb_cb(*this)
	, m_in_rb_cb(*this)
	, m_in_iv_cb(*this)
{

}

vme_mvme320_card_device::vme_mvme320_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme320_card_device(mconfig, VME_MVME320, tag, owner, clock)
{

}

void vme_mvme320_device::device_start()
{
	m_us_to_vme_read_timer = timer_alloc(0);
	m_us_to_vme_write_timer = timer_alloc(1);
	m_vme_to_us_read_timer = timer_alloc(2);
	m_vme_to_us_write_timer = timer_alloc(3);

	unscramble_proms();

	m_vme->install_device(vme_device::A16_SC, 0xFFB000, 0xFFB00F,
		read8sm_delegate(*this, FUNC(vme_mvme320_device::registers_r)), 
		write8sm_delegate(*this, FUNC(vme_mvme320_device::registers_w)), 
		0x00FF00FF);

	address_space &program = m_maincpu->space(AS_PROGRAM);
	m_state_machine_tap = program.install_read_tap(0x000000, 0x000FFF, "state_machine",[this](offs_t offset, u16 &data, u16 mem_mask)
	{
		update_state_machine(offset, data, mem_mask);
	});

	save_item(NAME(m_state_machine));
	save_item(NAME(m_vme_control_register));
	save_item(NAME(m_vme_status_1));
	save_item(NAME(m_vme_status_2));
	save_item(NAME(m_disk_buffer));
	save_item(NAME(m_disk_buffer_address_latch));

	save_item(NAME(m_vme_address));
	save_item(NAME(m_vme_data));

	save_item(NAME(m_iv_state));

	m_maincpu->state_add(23, "FAST_IO", m_state_machine).mask(0xFF);
	m_maincpu->state_add(24, "VSR1", m_vme_status_1).mask(0xFF);
	m_maincpu->state_add(25, "VSR2", m_vme_status_2).mask(0xFF);
	m_maincpu->state_add(26, "VCR1", m_vme_control_register).mask(0xFF);
	m_maincpu->state_add(27, "DBA", m_disk_buffer_address_latch).mask(0xFF);
	m_maincpu->state_add(28, "VMEADDR", m_vme_address);
	m_maincpu->state_add(29, "VMEDATA", m_vme_data);
	m_maincpu->state_add(30, "IV", m_iv_state);
}

void vme_mvme320_device::update_state_machine(offs_t offset, uint8_t data, uint8_t mem_mask)
{
	if(!machine().side_effects_disabled())
	{
		m_state_machine = memregion("fast_io")->base()[offset];
		m_maincpu->set_state_int(23, m_state_machine);

		m_read_state 	= (ReadState)(m_state_machine & 7);
		m_write_state 	= (WriteState)((m_state_machine >> 3) & 15);
		m_watchdog 		= BIT(m_state_machine, 7);
		m_s0 			= BIT(m_state_machine, 5);
	}
}

void vme_mvme320_device::device_reset()
{
	m_eca_address = 0;
	m_interrupt_src_status = 0;
	m_interrupt_vec = 0xF;
	m_drive_status = 0;

	m_vme_address = 0;
	m_vme_data = 0;

	m_vme_status_1 = VSR1_1 | VSR1_RSTVECT;
	m_vme_status_2 = VSR2_LBERRn | VSR2_BCLR | VSR2_SYSRESETn;
	m_vme_control_register = 0;

	m_io_bank = IO_BANK_NONE;
	m_io_phase = IO_PHASE_NONE;


	// Set up the control lines for the DBC register
	m_dbcr->uoc_w(0);
	m_dbcr->uic_w(1);
	m_dbcr->me_w(1);
	m_dbcr->rc_w(1);
	m_dbcr->wc_w(0);
}

void vme_mvme320_device::unscramble_proms()
{
	{
		// Data lines are reversed between the CPU and PROMs.
		// Unscramble them before we start.
		int length = memregion("maincpu")->bytes();
		u8 *rom = memregion("maincpu")->base();
		std::unique_ptr<u8[]> tmp = std::make_unique<u8[]>(length);

		memcpy(tmp.get(), rom, length);
		for (int i = 0; i < length; i++)
		{
			rom[i] = bitswap<8>(rom[i],0,1,2,3,4,5,6,7);
		}
	}

	{
		// Data lines are reversed between the CPU and PROMs.
		// Unscramble them before we start.
		int length = memregion("fast_io")->bytes();
		u8 *rom = memregion("fast_io")->base();
		std::unique_ptr<u8[]> tmp = std::make_unique<u8[]>(length);

		memcpy(tmp.get(), rom, length);
		for (int i = 0; i < length; i++)
		{
			rom[i] = bitswap<8>(rom[i],0,1,2,3,4,5,6,7);
		}
	}
}

uint8_t vme_mvme320_device::registers_r(offs_t offset)
{
	// Populate the high 4 bits of VME Status 2 with the low 3 address bits.
	uint8_t exA = 0, exB = 0;
	if(BIT(offset, 1)) { exA |= VSR2_EXA01; exB |= VSR1_EXA01; };
	if(BIT(offset, 2)) { exA |= VSR2_EXA02; exB |= VSR1_EXA02; };
	if(BIT(offset, 3)) { exA |= VSR2_EXA03; exB |= VSR1_EXA03; };

	m_vme_status_2 = VSR2_LBERRn | VSR2_SYSRESETn | exA;
	m_vme_status_1 = VSR1_1 | exB;

	m_vme_to_us_read_timer->adjust(m_maincpu->cycles_to_attotime(16));
	return 0xFF;
}

void vme_mvme320_device::registers_w(offs_t offset, uint8_t data)
{
	// Populate the high 4 bits of VME Status 2 with the low 3 address bits.
	uint8_t exA = 0, exB = 0;
	if(BIT(offset, 1)) { exA |= VSR2_EXA01; exB |= VSR1_EXA01; };
	if(BIT(offset, 2)) { exA |= VSR2_EXA02; exB |= VSR1_EXA02; };
	if(BIT(offset, 3)) { exA |= VSR2_EXA03; exB |= VSR1_EXA03; };

	m_vme_status_2 = VSR2_LBERRn | VSR2_SYSRESETn | exA;
	m_vme_status_1 = VSR1_1 | VSR1_WRITE | exB;
	m_vme_data = 0xFF00 | data; // Place the byte in the data latch

	m_vme_to_us_write_timer->adjust(m_maincpu->cycles_to_attotime(16));

	LOG("MVME320 register write %02X: %02X\n", offset, data);
}

void vme_mvme320_device::floppy_formats(format_registration &fr)
{
	fr.add_pc_formats();
}

/*
 * CPU to I/O write lines.
 */

// Select the active IO phase, either SC or WC.
// If the line is cleared, reset to NONE.
void vme_mvme320_device::sc_w(uint8_t val)
{
	if(val) m_io_phase = IO_PHASE_SC; else if (m_io_phase == IO_PHASE_SC) m_io_phase = IO_PHASE_NONE;

	if(m_s0 && m_io_phase == IO_PHASE_SC)
	{
		// S0 & SC -> increment VME address register
		m_vme_address = (m_vme_address + 1) & 0xFFFFFF;
	}

	decoder_read_outputs();
	decoder_write_outputs();
}

void vme_mvme320_device::wc_w(uint8_t val)
{
	if(val) m_io_phase = IO_PHASE_WC; else if (m_io_phase == IO_PHASE_WC) m_io_phase = IO_PHASE_NONE;

	// Propagate to 8X371 buffers.
	m_dbcr->wc_w(val);

	decoder_read_outputs();
	decoder_write_outputs();
}

// Select the active IO bank, either LB or RB.
// If the line is cleared, reset to NONE.
void vme_mvme320_device::lb_w(uint8_t val)
{
	if(val) m_io_bank = IO_BANK_LB; else if (m_io_bank == IO_BANK_LB) m_io_bank = IO_BANK_NONE;

	decoder_read_outputs();
	decoder_write_outputs();
}

void vme_mvme320_device::rb_w(uint8_t val)
{
	if(val) m_io_bank = IO_BANK_RB; else if (m_io_bank == IO_BANK_RB) m_io_bank = IO_BANK_NONE;

	decoder_read_outputs();
	decoder_write_outputs();
}

// Update the data on the IV bus coming from the CPU.
void vme_mvme320_device::iv_w(uint8_t val)
{
	m_iv_state = val;
}

// The CPU's MCLK line.
void vme_mvme320_device::mclk_w(uint8_t val)
{
	if(val) m_mclk_state = MCLK_ASSERTED; else m_mclk_state = MCLK_CLEARED;

	if(m_mclk_state == MCLK_CLEARED && m_io_bank == IO_BANK_RB && m_io_phase == IO_PHASE_WC)
	{
		// RB Output Phase - Subcycle 3/4
		switch(m_write_state)
		{
			default:
				break;
		}
	}
	else if(m_mclk_state == MCLK_ASSERTED && m_io_bank == IO_BANK_RB && m_io_phase == IO_PHASE_WC)
	{
		// RB Output Phase - Subcycle 4/4
		LOG("%04X: RB Data phase: %02X, IV %02X\n", m_maincpu->pc(), val, m_iv_state);

		char tmp[256];

		switch(m_write_state)
		{
			case WUASn: // IV bus is latched when /WUAS & /MCLK
				LOG("%04X: Writing WUAS: %02X\n", m_maincpu->pc(), m_iv_state);
				m_vme_address = 0x0000FFFF | (m_iv_state << 16);
				break;
			case WUDSn: // IV bus is latched when /WUDS & /MCLK
				LOG("%04X: Writing WUDS: %02X\n", m_maincpu->pc(), m_iv_state);
				m_vme_data = 0x00FF | (m_iv_state << 8);
				break;
			case WRDn:	// IV bus is latched when /WRD & MCLK
				LOG("%04X: Writing Drive Status: %02X\n", m_maincpu->pc(), m_iv_state);
				m_drive_status = m_iv_state;
				break;
			case WLDSn: // IV bus is latched when /WLDS & /MCLK
				LOG("%04X: Writing WLDS: %02X\n", m_maincpu->pc(), m_iv_state);
				m_vme_data = 0xFF00 | m_iv_state;
				break;
			case VCRn:	// IV bus is latched when /VCR & /MCLK
				// IV -> VME control reg
				update_vme_control_lines(~m_iv_state); // inverted output
				break;
			case WMASn: // IV bus is latched when /WMAS &/ MCLK
				LOG("%04X: Writing WMAS: %02X\n", m_maincpu->pc(), m_iv_state);
				m_vme_address = 0x00FF00FF | (m_iv_state << 8);
				break;
			case WLASn: // IV bus is latched when /WLAS & /MCLK
				LOG("%04X: Writing WLAS: %02X\n", m_maincpu->pc(), m_iv_state);
				m_vme_address = 0x00FFFF00 | m_iv_state;
				break;
			case WDC1n:	// IV bus is latched when /WDC1 & /MCLK
				// Write Disk Control 1 (U44)
				LOG("%04X: writing Disk Control 1: %02X\n", m_maincpu->pc(), m_iv_state);
				m_disk_control_1 = m_iv_state & (~0x04); // IV5 is not connected, it comes from Disk Status Reg
				break;
			case WDC2n: // IV bus is latched when /WDC2 & /MCLK
				// Write Disk Control 2 (U49)
				LOG("%04X: writing Disk Control 2: %02X\n", m_maincpu->pc(), m_iv_state);
				m_disk_control_2 = m_iv_state;
				break;
			case WDC3n: // IV bus is latched when /WDC3 & /MCLK
				// Write Disk Control 3 (U58)
				vme_dbc3_line_log(tmp, ~m_iv_state);
				LOG("%04X: writing Disk Control 3: %02X (%s)\n", m_maincpu->pc(), m_iv_state, tmp);
				m_disk_control_3 = m_iv_state;
				break;
			case WDBCn:
				// Write Disk Bit Control - handled separately
				break;
			case WBUn:
				// Write BUffer (write from disk buffer RAM)
				// /WBU is asserted when we get here so the write always goes through.
				ram_bank_w(m_disk_buffer_address_latch, m_iv_state);
				break;
			default:
				break;
		}
	}
	else if(m_mclk_state == MCLK_ASSERTED && m_io_bank == IO_BANK_LB && m_io_phase == IO_PHASE_SC)
	{
		// LB Address phase
		LOG("%04X: MCU updating DBU address: %02X\n", m_maincpu->pc(), m_iv_state);
		m_disk_buffer_address_latch = m_iv_state;
	}
}

void vme_mvme320_device::mvme320_program_map(address_map &map)
{
	// the A bus, 16-bit
	map(0x0000, 0x0fff).rom().region("maincpu", 0x0000);
}

void vme_mvme320_device::mvme320_io_map(address_map &map)
{
	// the IV bus, 8-bit
	map(0x000, 0x0ff).rw(FUNC(vme_mvme320_device::io_lb_r), FUNC(vme_mvme320_device::io_lb_w));
	map(0x100, 0x1ff).rw(FUNC(vme_mvme320_device::io_rb_r), FUNC(vme_mvme320_device::io_rb_w));
}

uint8_t vme_mvme320_device::vme_read_lobyte()
{
	LOG("%04X: MVME320 reading VMEbus [D7:D0], %08X\n", m_maincpu->pc(), m_vme_address);
	//return m_vme->read16(vme_device::A24_SC, vme_address) & 0xFF;
	m_us_to_vme_read_timer->adjust(m_maincpu->cycles_to_attotime(4));
	return 0xFF;
}

uint8_t vme_mvme320_device::vme_read_hibyte()
{
	LOG("%04X: MVME320 reading VMEbus [D15:D8], %08X\n", m_maincpu->pc(), m_vme_address);
	//return (m_vme->read16(vme_device::A24_SC, vme_address) & 0xFF00) >> 8;
	m_us_to_vme_read_timer->adjust(m_maincpu->cycles_to_attotime(4));
	return 0xFF;
}

void vme_mvme320_device::vme_write_byte()
{
	LOG("%04X: MVME320 writing VMEbus [D7:D0], %08X\n", m_maincpu->pc(), m_vme_address);
	m_vme_status_2 |= VSR2_CYACTIV;
	// TODO: Work out how many machine cycles it takes for CYACTIV to clear.
	m_us_to_vme_write_timer->adjust(m_maincpu->cycles_to_attotime(4));

	//m_vme->write8(vme_device::A24_SC, vme_address, m_vme_lower_data);
}

void vme_mvme320_device::update_vme_control_lines(uint8_t data)
{
	char logdata[100];
	vme_control_line_log(logdata, data);
	LOG("%04X: MVME320 wrote %02X to VME control register (%s)\n", m_maincpu->pc(), data, logdata);

	//m_vme->sysfail_w(BIT(data, 0));
	//m_vme->berr_w(BIT(data, 5));

	if(BIT(data, 1)) // CDS0
	{
		// Assert DS0
	}
	if(BIT(data, 2)) // CDS1
	{
		// Assert DS1	
	}
	if(BIT(data, 3)) // CWRT
	{
		vme_write_byte();
	}
	if(BIT(data, 6)) // CBR
	{
		// Assert the proper BR lines for arbitration
	}
}

uint8_t vme_mvme320_device::io_lb_r(offs_t offset)
{
	// No devices are hooked up to respond to LB.
	// Return the current IV bus contents.
	return m_iv_state;
}

void vme_mvme320_device::io_lb_w(offs_t offset, uint8_t data)
{
	// dummy
}

uint8_t vme_mvme320_device::io_rb_r(offs_t offset)
{
	// CPU is reading the IV bus Right Bank.
	if(m_io_phase == IO_PHASE_NONE)
	{
		// This will handle anything that doesn't need subcycle-accurate signal handling.
		switch(m_read_state)
		{
			case VSR1n:
				m_iv_state = bitswap<8>(m_vme_status_1, 0, 1, 2, 3, 4, 5, 6, 7); // inverted buffer with swapped data lines
				LOG("%04X: MCU reading VSR1, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case RDBCn:
				m_dbcr->me_w(0);
				m_dbcr->rc_w(0);
				m_iv_state = m_dbcr->read_iv();
				m_dbcr->rc_w(1);
				m_dbcr->me_w(1);
				LOG("%04X: MCU reading DBC, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case VRDLn:
				m_iv_state = vme_read_lobyte();
				LOG("%04X: MCU reading VRDL, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case RBUn:
				m_iv_state = ram_bank_r(m_disk_buffer_address_latch);
				LOG("%04X: MCU reading DBU %04X, value %02X\n", m_maincpu->pc(), m_disk_buffer_address_latch, m_iv_state);
				break;
			case VSR2n:
				// When enabled, the latch reads out b0-b3 as IV3-IV0.
				// ACFAIL 	- b0 - IV3 (IV4)
				// BCLR 	- b1 - IV2 (IV5)
				// /LBERR 	- b2 - IV1 (IV6)
				// CYACTIV 	- b3 - IV0 (IV7)
				m_iv_state = bitswap<4>(~m_vme_status_2, 3, 2, 1, 0) << 4; // non-inverting, only returns the 4 low bits of VSR2
				LOG("%04X: MCU reading VSR2, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case RDSn:	// Output enabled when /RB & /RDS.
				// All pins but Q5 of U48 pulled high when N/C but go through a transparent buffer to an active-low bus.
				m_iv_state = 0x04;
				LOG("%04X: MCU reading DS, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case VRDUn:
				m_iv_state = vme_read_hibyte();
				LOG("%04X: MCU reading VRDU, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case NOP:
				LOG("%04X: MCU reading old IV, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
		}
		return m_iv_state;
	}
	else
	{
		// SC/WC are not asserted during the input phase.
		fatalerror("rb_r at %04X but SC or WC asserted?\n", m_maincpu->pc());
	}
}

void vme_mvme320_device::io_rb_w(offs_t offset, uint8_t data)
{
	// dummy function, discrete logic handled through callbacks above
}

uint8_t vme_mvme320_device::ram_bank_r(offs_t offset)
{
	// U34 b6/b7 select the RAM bank.
	//offset += (m_disk_bit_control & 0x03) * 256;
	LOG("%04X: MVME050 reading RAM %04X\n", m_maincpu->pc(), offset);
	return m_disk_buffer[offset];
}

void vme_mvme320_device::ram_bank_w(offs_t offset, uint8_t data)
{
	//offset += (m_disk_bit_control & 0x03) * 256;
	LOG("%04X: MVME050 writing RAM %04X %02X\n", m_maincpu->pc(), offset, data);
	m_disk_buffer[offset] = data;
}

/*
 * Timers
 */
void vme_mvme320_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch(id)
	{
		case 0:
			// 320-to-VME read timer
			m_vme_status_2 &= ~VSR2_CYACTIV;
			break;
		case 1:
			// 320-to-VME write timer
			m_vme_status_2 &= ~VSR2_CYACTIV;
			break;
		case 2:
			// VME-to-320 read timer
			m_vme_status_2 &= ~VSR2_CYACTIV;
			break;
		case 3:
			// VME-to-320 write timer
			m_vme_status_2 &= ~VSR2_CYACTIV;
			break;
		default:
		LOG("Unhandled timer %d\n", id);
	}
}

/*
 * Read lines from the decoder
 */
void vme_mvme320_device::decoder_read_outputs()
{
	// Start by disabling all of them.
	VSR1n_w(1);
	RDBCn_w(1);
	VRDLn_w(1);
	RBUn_w(1);
	VSR2n_w(1);
	RDSn_w(1);
	VRDUn_w(1);

	if(m_io_phase == IO_PHASE_NONE)
	{
		switch(m_read_state)
		{
			case VSR1n:
				VSR1n_w(0);
				break;
			case RDBCn:
				RDBCn_w(0);
				break;
			case VRDLn:
				VRDLn_w(0);
				break;
			case RBUn:
				RBUn_w(0);
				break;
			case VSR2n:
				VSR2n_w(0);
				break;
			case RDSn:
				RDSn_w(0);
				break;
			case VRDUn:
				VRDUn_w(0);
				break;
			case NOP:
				break;
		}
	}
}
void vme_mvme320_device::VSR1n_w(bool state)
{
	m_read_line_state[VSR1n] = state;
}
void vme_mvme320_device::RDBCn_w(bool state)
{
	m_read_line_state[RDBCn] = state;
	m_dbcr->me_w(state);
}
void vme_mvme320_device::VRDLn_w(bool state)
{
	m_read_line_state[VRDLn] = state;
}
void vme_mvme320_device::RBUn_w(bool state)
{
	m_read_line_state[RBUn] = state;
}
void vme_mvme320_device::VSR2n_w(bool state)
{
	m_read_line_state[VSR2n] = state;
}
void vme_mvme320_device::RDSn_w(bool state)
{
	m_read_line_state[RDSn] = state;
}
void vme_mvme320_device::VRDUn_w(bool state)
{
	m_read_line_state[VRDUn] = state;
}

/*
 * Write lines from the decoder
 */
void vme_mvme320_device::decoder_write_outputs()
{
	// Start by disabling all of them.
	WUASn_w(1);
	WUDSn_w(1);
	WRDn_w(1);
	WLDSn_w(1);
	VCRn_w(1);
	WMASn_w(1);
	WLASn_w(1);
	WDC1n_w(1);
	WDBCn_w(1);
	WDC3n_w(1);
	WDC2n_w(1);
	WBUn_w(1);

	// Now enable the one that's selected.
	if(m_io_phase == IO_PHASE_WC && m_io_bank == IO_BANK_RB)
	{
		switch(m_write_state)
		{
			case WUASn:
				WUASn_w(0);
				break;
			case WUDSn:
				WUDSn_w(0);
				break;
			case WRDn:
				WRDn_w(0);
				break;
			case WLDSn:
				WLDSn_w(0);
				break;
			case VCRn:
				VCRn_w(0);
				break;
			case WMASn:
				WMASn_w(0);
				break;
			case WLASn:
				WLASn_w(0);
				break;
			case WDC1n:
				WDC1n_w(0);
				break;
			case WDBCn:
				WDBCn_w(0);
				break;
			case WDC3n:
				WDC3n_w(0);
				break;
			case WDC2n:
				WDC2n_w(0);
				break;
			case WBUn:
				WBUn_w(0);
				break;
			default:
				break;
		}
	}
}

void vme_mvme320_device::WUASn_w(bool state)
{
	m_write_line_state[WUASn] = state;
}
void vme_mvme320_device::WUDSn_w(bool state)
{
	m_write_line_state[WUDSn] = state;
}
void vme_mvme320_device::WRDn_w(bool state) 
{
	m_write_line_state[WRDn] = state;
}
void vme_mvme320_device::WLDSn_w(bool state)
{
	m_write_line_state[WLDSn] = state;
}
void vme_mvme320_device::VCRn_w(bool state) 
{
	m_write_line_state[VCRn] = state;
}
void vme_mvme320_device::WMASn_w(bool state)
{
	m_write_line_state[WMASn] = state;
}
void vme_mvme320_device::WLASn_w(bool state)
{
	m_write_line_state[WLASn] = state;
}
void vme_mvme320_device::WDC1n_w(bool state) 
{
	m_write_line_state[WDC1n] = state;
}
void vme_mvme320_device::WDBCn_w(bool state)
{
	m_write_line_state[WDBCn] = state;
	m_dbcr->me_w(state);
}
void vme_mvme320_device::WDC3n_w(bool state)
{
	m_write_line_state[WDC3n] = state;
}
void vme_mvme320_device::NOP11_w(bool state) {}
void vme_mvme320_device::WDC2n_w(bool state)
{
	m_write_line_state[WDC2n] = state;
}
void vme_mvme320_device::NOP13_w(bool state) {}
void vme_mvme320_device::WBUn_w(bool state) 
{
	m_write_line_state[WBUn] = state;
}
void vme_mvme320_device::NOP15_w(bool state) {}

/*
 * Machine configuration
 */
void vme_mvme320_device::device_add_mconfig(machine_config &config)
{
	// basic machine hardware
	N8X305(config, m_maincpu, 16_MHz_XTAL/2); // N8X305, 8MHz. MCLK output is 2MHz
	m_maincpu->set_addrmap(AS_PROGRAM, &vme_mvme320_device::mvme320_program_map);
	m_maincpu->set_addrmap(AS_IO, &vme_mvme320_device::mvme320_io_map);
	m_maincpu->sc_callback().set(FUNC(vme_mvme320_device::sc_w));
	m_maincpu->wc_callback().set(FUNC(vme_mvme320_device::wc_w));
	m_maincpu->lb_callback().set(FUNC(vme_mvme320_device::lb_w));
	m_maincpu->rb_callback().set(FUNC(vme_mvme320_device::rb_w));
	m_maincpu->mclk_callback().set(FUNC(vme_mvme320_device::mclk_w));
	m_maincpu->iv_callback().set(FUNC(vme_mvme320_device::iv_w));

	FLOPPY_CONNECTOR(config, m_fd0, 0);
	m_fd0->option_add("8ssdd", FLOPPY_8_SSDD);
	m_fd0->option_add("8dsdd", FLOPPY_8_DSDD);
	m_fd0->option_add("525dd", FLOPPY_525_DD);
	m_fd0->option_add("525hd", FLOPPY_525_HD);
	m_fd0->set_default_option("525dd");
	m_fd0->set_formats(floppy_image_device::default_mfm_floppy_formats);

	N8X371(config, m_dbcr, 0);
	m_maincpu->mclk_callback().append(m_dbcr, FUNC(n8x371_device::mclk_w));
	m_maincpu->iv_callback().append(m_dbcr, FUNC(n8x371_device::write_iv));
	m_maincpu->wc_callback().append(m_dbcr, FUNC(n8x371_device::wc_w));
}

// ROM definitions
ROM_START(mvme320)
    ROM_REGION16_LE(0x2000, "maincpu", 0)
	ROM_LOAD16_BYTE("3.0-u1.bin", 0x0000, 0x1000, CRC(522ed0f3) SHA1(b69c8ea1d5ed18407b597467ca6488b2127b68f2))
    ROM_LOAD16_BYTE("3.0-u9.bin", 0x0001, 0x1000, CRC(980eea2c) SHA1(5a58d5da9eb80bd9f439ea1b95f3af70362bafa7))

	ROM_REGION(0x1000, "fast_io", 0)
	ROM_LOAD("3.0-u3.bin", 0x0000, 0x1000, CRC(3ed42fe5) SHA1(d1b020753aa4be0f0e8b89ace8fa2856f1bf5c79))
ROM_END

const tiny_rom_entry *vme_mvme320_device::device_rom_region() const
{
	return ROM_NAME(mvme320);
}

void vme_control_line_log(char *logdata, uint8_t data)
{
	memset(logdata, 0, 100);
	if(!BIT(data, 0))
	{	
		strcat(logdata, "/CLED1 ");
	}
	if(BIT(data, 1))
	{	
		strcat(logdata, "CDS0 ");
	}
	if(BIT(data, 2))
	{	
		strcat(logdata, "CDS1 ");
	}
	if(BIT(data, 3))
	{	
		strcat(logdata, "CWRT ");
	}
	if(BIT(data, 4))
	{	
		strcat(logdata, "NC ");
	}
	if(BIT(data, 5))
	{	
		strcat(logdata, "CBERR ");
	}
	if(BIT(data, 6))
	{	
		strcat(logdata, "CBR ");
	}
	if(!BIT(data, 7))
	{	
		strcat(logdata, "/START ");
	}
}


void vme_dbc3_line_log(char *logdata, uint8_t data)
{
	memset(logdata, 0, 100);
	if(!BIT(data, 0))
	{	
		strcat(logdata, "/SDR ");
	}
	if(BIT(data, 1))
	{	
		strcat(logdata, "U64.A7 ");
	}
	if(BIT(data, 2))
	{	
		strcat(logdata, "CIRQ ");
	}
	if(BIT(data, 3))
	{	
		strcat(logdata, "PRECOMP ");
	}
	if(BIT(data, 4))
	{	
		strcat(logdata, "ECC ");
	}
	if(BIT(data, 5))
	{	
		strcat(logdata, "MS2 ");
	}
	if(BIT(data, 6))
	{	
		strcat(logdata, "MS1 ");
	}
	if(BIT(data, 7))
	{	
		strcat(logdata, "FDHD ");
	}
}

