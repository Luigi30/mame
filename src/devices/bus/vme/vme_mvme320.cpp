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
#define LOG_DECODER (1U << 4)
#define LOG_VMEBUS	(1U << 5)
#define LOG_FLOPPY	(1U << 6)

#define VERBOSE (LOG_FLOPPY)

#include "logmacro.h"

#define LOGPRINTF(...)  LOGMASKED(LOG_PRINTF,   __VA_ARGS__)
#define LOGSETUP(...)   LOGMASKED(LOG_SETUP,    __VA_ARGS__)
#define LOGGENERAL(...) LOGMASKED(LOG_GENERAL,  __VA_ARGS__)
#define LOGDECODER(...) LOGMASKED(LOG_DECODER,	__VA_ARGS__)
#define LOGVMEBUS(...) 	LOGMASKED(LOG_VMEBUS,	__VA_ARGS__)
#define LOGFLOPPY(...) 	LOGMASKED(LOG_FLOPPY,	__VA_ARGS__)

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
void vme_dbc1_line_log(char *logdata, uint8_t data);

DEFINE_DEVICE_TYPE(VME_MVME320, vme_mvme320_card_device, "mvme320", "Motorola MVME320/B Disk Controller")

vme_mvme320_device::vme_mvme320_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock)
	, device_vme_card_interface(mconfig, *this)
	, m_maincpu(*this, "maincpu")
	, m_fd0(*this, "floppy0")
	, m_fd1(*this, "floppy1")
	, m_fd2(*this, "floppy2")
	, m_fd3(*this, "floppy3")
	, m_dbcr(*this, "dbcr")
	, m_in_sc_cb(*this)
	, m_in_wc_cb(*this)
	, m_in_lb_cb(*this)
	, m_in_rb_cb(*this)
	, m_in_iv_cb(*this)
	, m_dtack_timer(nullptr)
	, m_cyactiv_timer(nullptr)
{

}

vme_mvme320_card_device::vme_mvme320_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme320_card_device(mconfig, VME_MVME320, tag, owner, clock)
{

}

void vme_mvme320_device::device_start()
{
	m_dtack_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(vme_mvme320_device::dtack_timer_expired), this));
	m_cyactiv_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(vme_mvme320_device::cyactiv_timer_expired), this));

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
	m_maincpu->state_add(24, "VSR1", m_vme_status_1);
	m_maincpu->state_add(25, "VSR2", m_vme_status_2);
	m_maincpu->state_add(26, "VCR1", m_vme_control_register).mask(0xFF);
	m_maincpu->state_add(27, "DBA", m_disk_buffer_address_latch).mask(0xFF);
	m_maincpu->state_add(28, "VMEADDR", m_vme_address);
	m_maincpu->state_add(29, "VMEDATA", m_vme_data);
	m_maincpu->state_add(30, "IV", m_iv_state);

	m_eca_address = 0;
	m_interrupt_src_status = 0;
	m_interrupt_vec = 0xF;
	m_drive_status = 0;

	m_vme_address = 0;
	m_vme_data = 0;

	// RSTVECT is asserted on SYSRESET and cleared by the first IVL assertion at U70B.
	m_vme_status_1 = VSR1_1;
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


TIMER_CALLBACK_MEMBER(vme_mvme320_device::dtack_timer_expired)
{
	// Pulse DTACK
	m_vme->dtack_w(1);
	m_vme->dtack_w(0);
}

TIMER_CALLBACK_MEMBER(vme_mvme320_device::cyactiv_timer_expired)
{
	// Clear CYACTIV
	m_vme_status_2 = m_vme_status_2 & ~(VSR2_CYACTIV);
}

void vme_mvme320_device::update_state_machine(offs_t offset, uint8_t data, uint8_t mem_mask)
{
	if(!machine().side_effects_disabled())
	{
		m_state_machine = memregion("fast_io")->base()[offset];
		m_maincpu->set_state_int(23, m_state_machine);

		regDecoder.m_read_state 	= (RegDecoder::ReadState)(m_state_machine & 7);
		regDecoder.m_write_state 	= (RegDecoder::WriteState)((m_state_machine >> 3) & 15);
		m_watchdog 		= BIT(m_state_machine, 7);
		m_s0 			= BIT(m_state_machine, 5);
	}
}

void vme_mvme320_device::device_reset()
{
	// SYSRESETn, not I8X305n.

	m_eca_address = 0;
	m_interrupt_src_status = 0;
	m_interrupt_vec = 0xF;
	m_drive_status = 0;

	m_vme_address = 0;
	m_vme_data = 0;

	// RSTVECT is asserted on SYSRESET and cleared by the first IVL assertion at U70B.
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
	if(!machine().side_effects_disabled())
	{
		LOGVMEBUS("MVME register read: offset %02X\n", offset);

		// Populate the high 4 bits of VME Status 2 with the low 3 address bits.
		uint8_t exA = 0, exB = 0;
		if(BIT(offset, 0)) { exA |= VSR2_EXA01; exB |= VSR1_EXA01; };
		if(BIT(offset, 1)) { exA |= VSR2_EXA02; exB |= VSR1_EXA02; };
		if(BIT(offset, 2)) { exA |= VSR2_EXA03; exB |= VSR1_EXA03; };

		m_vme_status_2 = VSR2_LBERRn | VSR2_SYSRESETn | exA;
		m_vme_status_1 = VSR1_1 | exB;

		// PALs
		bool CIRQ = m_disk_control_3 & 0x04;
		bool CWRT = (m_vme_control_register & 0x10) == 0;
		m_paldecode.update(0x29, true, offset, 3, false, CIRQ);
		m_paldecode.log(this);

		m_palbuf.update(m_paldecode.REGSELn, m_paldecode.REG13RDn, true, CWRT, false, false, false, false);
		m_palbuf.log(this);
		m_palbuf.update(m_paldecode.REGSELn, m_paldecode.REG13RDn, true, CWRT, false, false, true, false);
		m_palbuf.log(this);

		if(m_palbuf.RGMND) m_vme_status_1 |= VSR1_RGMND; else m_vme_status_1 &= ~(VSR1_RGMND);

		if(m_palbuf.I8X305n)
		{
			m_maincpu->reset();
		}

		m_dtack_timer->adjust(attotime::from_usec(100));
		m_dtack_timer->enable(true);
		m_vme->dtack_w(0);

		// The weird addresses come from the formula:
		// ([A1|A2|A3] & $0F) + $20
		switch(offset)
		{
			case 0:
				return m_disk_buffer[0x20];
			case 1:
				return m_disk_buffer[0x24];
			case 2:
				return m_disk_buffer[0x22];
			case 3:
				return m_disk_buffer[0x26];
			case 4:
				return m_disk_buffer[0x21];
			case 5:
				return m_disk_buffer[0x25];
			case 6:
				return m_disk_buffer[0x23];
			case 7:
				return m_disk_buffer[0x27];
			default:
				return 0xFF;
				break;
		}
	}

	return (m_vme_data & 0xFF);
}

void vme_mvme320_device::registers_w(offs_t offset, uint8_t data)
{
	// Effectively asserted: VWRT, /BDSEL, address bits, AM29

	// Offset is the address shifted right by 1 because we mask the even addresses.
	// Populate the high 4 bits of VME Status 2 with the low 3 address bits.
	uint8_t exA = 0, exB = 0;
	if(BIT(offset, 0)) { exA |= VSR2_EXA01; exB |= VSR1_EXA01; };
	if(BIT(offset, 1)) { exA |= VSR2_EXA02; exB |= VSR1_EXA02; };
	if(BIT(offset, 2)) { exA |= VSR2_EXA03; exB |= VSR1_EXA03; };

 	m_vme_status_1 = VSR1_1 | VSR1_WRITE | exB;
	m_vme_status_2 = VSR2_LBERRn | VSR2_SYSRESETn | exA;
	m_vme_data = (m_vme_data & 0xFF00) | data; // Place the byte in the data latch

	// PALs
	bool CIRQ = m_disk_control_3 & 0x04;
	bool CWRT = (m_vme_control_register & 0x10) == 0;
	m_paldecode.update(0x29, true, offset, 3, true, CIRQ);
	m_paldecode.log(this);

	m_palbuf.update(m_paldecode.REGSELn, m_paldecode.REG13RDn, true, CWRT, false, false, false, false);
	m_palbuf.log(this);
	m_palbuf.update(m_paldecode.REGSELn, m_paldecode.REG13RDn, true, CWRT, false, false, true, false);
	m_palbuf.log(this);

	if(m_palbuf.RGMND) m_vme_status_1 |= VSR1_RGMND; else m_vme_status_1 &= ~(VSR1_RGMND);
	if(m_palbuf.TODISKn && m_palbuf.LDAOEn && !m_palbuf.DBDIR)
	{
		// Latch incoming VME low byte.
		m_vme_data = (m_vme_data & 0xFF00) | data;
	}
	if(m_palbuf.I8X305n)
	{
		m_maincpu->reset();
	}

	m_dtack_timer->adjust(attotime::from_usec(100));
	m_dtack_timer->enable(true);
	m_vme->dtack_w(0);

	LOGVMEBUS("MVME320 register write %02X: %02X\n", offset, data);
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
		switch(regDecoder.m_write_state)
		{
			default:
				break;
		}
	}
	else if(m_mclk_state == MCLK_ASSERTED && m_s0 && m_io_phase == IO_PHASE_SC)
	{
		// S0 & SC -> increment VME address register on rising edge of clock
		m_vme_address = (m_vme_address + 1) & 0xFFFFFF;
	}
	else if(m_mclk_state == MCLK_ASSERTED && m_io_bank == IO_BANK_RB && m_io_phase == IO_PHASE_WC)
	{
		// RB Output Phase - Subcycle 4/4
		LOGDECODER("%04X: RB Data phase: %02X, IV %02X\n", m_maincpu->pc(), val, m_iv_state);

		char tmp[256];
		uint8_t inverted = ~m_iv_state;

		switch(regDecoder.m_write_state)
		{
			case RegDecoder::WUASn: // IV bus is latched when /WUAS & /MCLK
				LOGDECODER("%04X: Writing WUAS: %02X\n", m_maincpu->pc(), m_iv_state);
				m_vme_address = (m_vme_address & 0x0000FFFF) | (((uint32_t)inverted) << 16);
				break;
			case RegDecoder::WUDSn: // IV bus is latched when /WUDS & /MCLK
				LOGDECODER("%04X: Writing WUDS: %02X\n", m_maincpu->pc(), m_iv_state);
				m_vme_data = (m_vme_data & 0x00FF) | (((uint32_t)m_iv_state) << 8);
				break;
			case RegDecoder::WRDn:	// IV bus is latched when /WRD & MCLK
				LOGDECODER("%04X: Writing Drive Status: %02X\n", m_maincpu->pc(), m_iv_state);
				m_drive_status = m_iv_state;
				break;
			case RegDecoder::WLDSn: // IV bus is latched when /WLDS & /MCLK
				LOGDECODER("%04X: Writing WLDS: %02X\n", m_maincpu->pc(), m_iv_state);
				m_vme_data = (m_vme_data & 0xFF00) | m_iv_state;
				break;
			case RegDecoder::VCRn:	// IV bus is latched when /VCR & /MCLK
				// IV -> VME control reg
				update_vme_control_lines(~m_iv_state); // inverted output
				break;
			case RegDecoder::WMASn: // IV bus is latched when /WMAS &/ MCLK
				LOGDECODER("%04X: Writing WMAS: %02X\n", m_maincpu->pc(), m_iv_state);
				m_vme_address = (m_vme_address & 0x00FF00FF) | (((uint32_t)inverted) << 8);
				break;
			case RegDecoder::WLASn: // IV bus is latched when /WLAS & /MCLK
				LOGDECODER("%04X: Writing WLAS: %02X\n", m_maincpu->pc(), m_iv_state);
				m_vme_address = (m_vme_address & 0x00FFFF00) | inverted;
				break;
			case RegDecoder::WDC1n:	// IV bus is latched when /WDC1 & /MCLK
				// Write Disk Control 1 (U44)
				// The outputs are enabled by RSTVECT.
				// These outputs go directly to J3/J4 floppy connectors. Active low.
				vme_dbc1_line_log(tmp, m_iv_state);
				LOGDECODER("%04X: writing Disk Control 1: %02X (%s)\n", m_maincpu->pc(), m_iv_state, tmp);
				m_disk_control_1 = m_iv_state & (~0x04); 
				update_disk_control_1(m_iv_state); // IV5 is not connected, it comes from Disk Status Reg
				break;
			case RegDecoder::WDC2n: // IV bus is latched when /WDC2 & /MCLK
				// Write Disk Control 2 (U49)
				LOGDECODER("%04X: writing Disk Control 2: %02X\n", m_maincpu->pc(), m_iv_state);
				m_disk_control_2 = m_iv_state;
				update_disk_control_2(m_iv_state);
				break;
			case RegDecoder::WDC3n: // IV bus is latched when /WDC3 & /MCLK
				// Write Disk Control 3 (U58)
				vme_dbc3_line_log(tmp, ~m_iv_state);
				LOGDECODER("%04X: writing Disk Control 3: %02X (%s)\n", m_maincpu->pc(), m_iv_state, tmp);
				m_disk_control_3 = m_iv_state;
				update_disk_control_3(m_iv_state);
				break;
			case RegDecoder::WDBCn:
				if(m_iv_state != 0) LOGFLOPPY("MVME WDBC: %02X\n", m_iv_state);
				// Write Disk Bit Control - handled separately
				break;
			case RegDecoder::WBUn:
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
		LOGDECODER("%04X: MCU updating DBU address: %02X\n", m_maincpu->pc(), m_iv_state);
		m_disk_buffer_address_latch = m_iv_state;

		// IVL is now 1.
		m_vme_status_1 &= ~VSR1_RSTVECT; // Disable RSTVECT
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
	// m_vme_address is shifted right by 1 bit.
	LOGVMEBUS("%04X: MVME320 reading VMEbus [D07:D00], %08X\n", m_maincpu->pc(), (m_vme_address << 1));
	m_vme_data = (m_vme_data & 0xFF00) | read8(vme_device::A24_SC, (m_vme_address << 1) + 1);
	return m_vme_data & 0xFF;
}

uint8_t vme_mvme320_device::vme_read_hibyte()
{
	LOGVMEBUS("%04X: MVME320 reading VMEbus [D15:D08], %08X\n", m_maincpu->pc(), (m_vme_address << 1));
	m_vme_data = (m_vme_data & 0x00FF);
	m_vme_data = m_vme_data | ((uint16_t)(read8(vme_device::A24_SC, (m_vme_address << 1))) << 8);

	return m_vme_data >> 8;
}

void vme_mvme320_device::vme_write_lobyte()
{
	LOGVMEBUS("%04X: MVME320 writing VMEbus [D07:D00], %08X\n", m_maincpu->pc(), (m_vme_address << 1));
	m_vme_status_2 |= VSR2_CYACTIV;
	write8(vme_device::A24_SC, (m_vme_address << 1) + 1, m_vme_data & 0xFF);
	m_cyactiv_timer->adjust(attotime::from_usec(100));
	m_cyactiv_timer->enable(true);
}

void vme_mvme320_device::vme_write_hibyte()
{
	LOGVMEBUS("%04X: MVME320 writing VMEbus [D15:D07], %08X\n", m_maincpu->pc(), (m_vme_address << 1));
	m_vme_status_2 |= VSR2_CYACTIV;
	write8(vme_device::A24_SC, (m_vme_address << 1), m_vme_data >> 8);
	m_cyactiv_timer->adjust(attotime::from_usec(100));
	m_cyactiv_timer->enable(true);
}

void vme_mvme320_device::update_vme_control_lines(uint8_t data)
{
	char logdata[100];
	vme_control_line_log(logdata, data);
	LOGVMEBUS("%04X: MVME320 wrote %02X to VME control register (%s)\n", m_maincpu->pc(), data, logdata);

	//m_vme->sysfail_w(BIT(data, 0));
	//m_vme->berr_w(BIT(data, 5));
	bool is_write = BIT(data, 3);

	// /START asserted here.
	if(BIT(data, 2)) // CDS1
	{
		if(is_write)
		{
			vme_write_hibyte();
		}
		else
		{
			m_vme_data = (m_vme_data & 0x00FF) | (((uint16_t)vme_read_hibyte()) << 8);
		}		
	}
	if(BIT(data, 1)) // CDS0
	{
		if(is_write)
		{
			vme_write_lobyte();
		}
		else
		{
			m_vme_data = (m_vme_data & 0xFF00) | (uint16_t)vme_read_lobyte();	// Maybe when /START asserted?
		}
		
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
		switch(regDecoder.m_read_state)
		{
			case RegDecoder::VSR1n:
				m_iv_state = bitswap<8>(m_vme_status_1, 0, 1, 2, 3, 4, 5, 6, 7); // inverted buffer with swapped data lines
				LOGDECODER("%04X: MCU reading VSR1, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case RegDecoder::RDBCn:
				m_dbcr->me_w(0);
				m_dbcr->rc_w(0);
				m_iv_state = m_dbcr->read_iv();
				m_dbcr->rc_w(1);
				m_dbcr->me_w(1);
				LOGDECODER("%04X: MCU reading DBC, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case RegDecoder::VRDLn:
				m_iv_state = m_vme_data & 0xFF; // Reads the register, NOT the VMEbus.
				LOGDECODER("%04X: MCU reading VRDL, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case RegDecoder::RBUn:
				m_iv_state = ram_bank_r(m_disk_buffer_address_latch);
				LOGDECODER("%04X: MCU reading DBU %04X, value %02X\n", m_maincpu->pc(), m_disk_buffer_address_latch, m_iv_state);
				break;
			case RegDecoder::VSR2n:
				// When enabled, the latch reads out b0-b3 as IV3-IV0.
				// ACFAIL 	- b0 - IV3 (IV4)
				// BCLR 	- b1 - IV2 (IV5)
				// /LBERR 	- b2 - IV1 (IV6)
				// CYACTIV 	- b3 - IV0 (IV7)
				m_iv_state = bitswap<4>(~m_vme_status_2, 3, 2, 1, 0) << 4; // non-inverting, only returns the 4 low bits of VSR2
				LOGDECODER("%04X: MCU reading VSR2, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case RegDecoder::RDSn:	// Output enabled when /RB & /RDS.
				// All pins but Q5 of U48 pulled high when N/C but go through a transparent buffer to an active-low bus.
				//m_iv_state = 0x04;
				m_iv_state = get_floppy_state(0);
				LOGDECODER("%04X: MCU reading DS, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case RegDecoder::VRDUn:
				m_iv_state = m_vme_data >> 8; // Reads the register, NOT the VMEbus.
				LOGDECODER("%04X: MCU reading VRDU, value %02X\n", m_maincpu->pc(), m_iv_state);
				break;
			case RegDecoder::NOP:
				LOGDECODER("%04X: MCU reading old IV, value %02X\n", m_maincpu->pc(), m_iv_state);
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

void vme_mvme320_device::update_disk_control_1(uint8_t value)
{
	if(value != 0) LOGFLOPPY("%04X: clocking disk control 1: %02X\n", m_maincpu->pc(), value);

	floppy_image_device *floppy;
		 if(BIT(m_disk_control_1, 3)) floppy = m_fd0->get_device();
	else if(BIT(m_disk_control_1, 4)) floppy = m_fd1->get_device();
	else if(BIT(m_disk_control_1, 5)) floppy = m_fd2->get_device();
	else if(BIT(m_disk_control_1, 6)) floppy = m_fd3->get_device();
	else return;

	// bit		MVME		pin34	pin50
	// 0x01		WRGATE		24		40
	// 0x02		WGTE/REZRO	X		6
	// 0x04		FMODE		X		2
	// 0x08		UNITSEL0	16		50		
	// 0x10		SEL1		10		26
	// 0x20		SEL2		12		28
	// 0x40		SEL3		14		30
	// 0x80		SEL4/MOTOR	6		32

	floppy->mon_w(BIT(value, 3));
}

void vme_mvme320_device::update_disk_control_2(uint8_t value)
{
	LOGFLOPPY("%04X: clocking disk control 2: %02X\n", m_maincpu->pc(), value);

	floppy_image_device *floppy;
		 if(BIT(m_disk_control_1, 3)) floppy = m_fd0->get_device();
	else if(BIT(m_disk_control_1, 4)) floppy = m_fd1->get_device();
	else if(BIT(m_disk_control_1, 5)) floppy = m_fd2->get_device();
	else if(BIT(m_disk_control_1, 6)) floppy = m_fd3->get_device();
	else return;

	// bit		MVME		pin34	pin50
	// 0x01		LOWCURRENT	X		2
	// 0x02		DIR			18		34
	// 0x04		STEP1SECT	X		24
	// 0x08		STEP		20		36
	// 0x10		HEAD22		X		4
	// 0x20		HEAD21		X		18
	// 0x40		HD20/SIDE	32		48
	// 0x80		HEAD20		X		14

	floppy->dir_w(BIT(value, 1));
	floppy->stp_w(BIT(value, 2));
	floppy->ss_w(BIT(value, 6));
}

void vme_mvme320_device::update_disk_control_3(uint8_t value)
{
	LOGFLOPPY("%04X: clocking disk control 3: %02X\n", m_maincpu->pc(), value);

	// floppy_image_device *floppy;
	// 	 if(BIT(m_disk_control_1, 3)) floppy = m_fd0->get_device();
	// else if(BIT(m_disk_control_1, 4)) floppy = m_fd1->get_device();
	// else if(BIT(m_disk_control_1, 5)) floppy = m_fd2->get_device();
	// else if(BIT(m_disk_control_1, 6)) floppy = m_fd3->get_device();
	// else return;

	// bit		MVME		pin34	pin50
	// 0x01		/SDR
	// 0x02		(U64.A7)
	// 0x04		CIRQ
	// 0x08		PRECOMP
	// 0x10		ECC
	// 0x20		MS2
	// 0x40		MS1
	// 0x80		FDHD
	
}

uint8_t vme_mvme320_device::get_floppy_state(int floppy_num)
{
	// todo: handle more than 1 floppy drive
	// 		 handle 8" floppy drives

	uint8_t retval = 0;
	floppy_image_device *floppy;
		 if(BIT(m_disk_control_1, 3)) floppy = m_fd0->get_device();
	else if(BIT(m_disk_control_1, 4)) floppy = m_fd1->get_device();
	else if(BIT(m_disk_control_1, 5)) floppy = m_fd2->get_device();
	else if(BIT(m_disk_control_1, 6)) floppy = m_fd3->get_device();
	else return 0x04;

	LOGFLOPPY("MVME320 %04X get_floppy_state: %02X\n", m_maincpu->pc(), m_disk_control_1 >> 3);

	// 8" floppy, use 50-pin signals (J3)
	// 5.25" floppy, use 34-pin signals (J4)

	// into the PLL
	// FLPYDSKRD 			46		30

	// bit		MVME		pin34	pin50
	// 0x80: 	SEEKCOMPL	X		8		
	// 0x40: 	IX			8		20		
	// 0x20: 	READY		34		22		(if J15 closed)
	// 0x10: 	2SDED/TR00	X		10
	// 0x08: 	TR000		26		42
	// 0x04: 	NOTBUSY		X		16
	// 0x02: 	DISKCHANGE	X		12
	// 0x01: 	WRFAULT		28		44

	// Floppy signals are active-low. Pins that aren't connected are pulled high, except NOTBUSY.
	bool seekcompl 	= 0;
	bool ix 		= floppy->idx_r();
	bool ready 		= floppy->ready_r();
	bool twosided	= 0;
	bool tr000		= floppy->trk00_r();
	bool notbusy	= 1;
	bool diskchange	= 0;
	bool wrfault	= floppy->wpt_r();

	retval = ((seekcompl << 7) | (ix << 6) | (ready << 5) | (twosided << 4) | (tr000 << 3) | (notbusy << 2) | (diskchange << 1) | wrfault);
	return retval;
}

void vme_mvme320_device::io_rb_w(offs_t offset, uint8_t data)
{
	// dummy function, discrete logic handled through callbacks above
}

uint8_t vme_mvme320_device::ram_bank_r(offs_t offset)
{
	// U34 b6/b7 select the RAM bank.
	//offset += (m_disk_bit_control & 0x03) * 256;
	LOGDECODER("%04X: MVME050 reading RAM %04X\n", m_maincpu->pc(), offset);
	return m_disk_buffer[offset];
}

void vme_mvme320_device::ram_bank_w(offs_t offset, uint8_t data)
{
	//offset += (m_disk_bit_control & 0x03) * 256;
	LOGDECODER("%04X: MVME050 writing RAM %04X %02X\n", m_maincpu->pc(), offset, data);
	m_disk_buffer[offset] = data;
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
		switch(regDecoder.m_read_state)
		{
			case RegDecoder::VSR1n:
				VSR1n_w(0);
				break;
			case RegDecoder::RDBCn:
				RDBCn_w(0);
				break;
			case RegDecoder::VRDLn:
				VRDLn_w(0);
				break;
			case RegDecoder::RBUn:
				RBUn_w(0);
				break;
			case RegDecoder::VSR2n:
				VSR2n_w(0);
				break;
			case RegDecoder::RDSn:
				RDSn_w(0);
				break;
			case RegDecoder::VRDUn:
				VRDUn_w(0);
				break;
			case RegDecoder::NOP:
				break;
		}
	}
}
void vme_mvme320_device::VSR1n_w(bool state)
{
	m_read_line_state[RegDecoder::VSR1n] = state;
}
void vme_mvme320_device::RDBCn_w(bool state)
{
	m_read_line_state[RegDecoder::RDBCn] = state;
	m_dbcr->me_w(state);
}
void vme_mvme320_device::VRDLn_w(bool state)
{
	m_read_line_state[RegDecoder::VRDLn] = state;
}
void vme_mvme320_device::RBUn_w(bool state)
{
	m_read_line_state[RegDecoder::RBUn] = state;
}
void vme_mvme320_device::VSR2n_w(bool state)
{
	m_read_line_state[RegDecoder::VSR2n] = state;
}
void vme_mvme320_device::RDSn_w(bool state)
{
	m_read_line_state[RegDecoder::RDSn] = state;
}
void vme_mvme320_device::VRDUn_w(bool state)
{
	m_read_line_state[RegDecoder::VRDUn] = state;
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
		switch(regDecoder.m_write_state)
		{
			case RegDecoder::WUASn:
				WUASn_w(0);
				break;
			case RegDecoder::WUDSn:
				WUDSn_w(0);
				break;
			case RegDecoder::WRDn:
				WRDn_w(0);
				break;
			case RegDecoder::WLDSn:
				WLDSn_w(0);
				break;
			case RegDecoder::VCRn:
				VCRn_w(0);
				break;
			case RegDecoder::WMASn:
				WMASn_w(0);
				break;
			case RegDecoder::WLASn:
				WLASn_w(0);
				break;
			case RegDecoder::WDC1n:
				WDC1n_w(0);
				break;
			case RegDecoder::WDBCn:
				WDBCn_w(0);
				break;
			case RegDecoder::WDC3n:
				WDC3n_w(0);
				break;
			case RegDecoder::WDC2n:
				WDC2n_w(0);
				break;
			case RegDecoder::WBUn:
				WBUn_w(0);
				break;
			default:
				break;
		}
	}
}

void vme_mvme320_device::WUASn_w(bool state)
{
	m_write_line_state[RegDecoder::WUASn] = state;
}
void vme_mvme320_device::WUDSn_w(bool state)
{
	m_write_line_state[RegDecoder::WUDSn] = state;
}
void vme_mvme320_device::WRDn_w(bool state) 
{
	m_write_line_state[RegDecoder::WRDn] = state;
}
void vme_mvme320_device::WLDSn_w(bool state)
{
	m_write_line_state[RegDecoder::WLDSn] = state;
}
void vme_mvme320_device::VCRn_w(bool state) 
{
	m_write_line_state[RegDecoder::VCRn] = state;
}
void vme_mvme320_device::WMASn_w(bool state)
{
	m_write_line_state[RegDecoder::WMASn] = state;
}
void vme_mvme320_device::WLASn_w(bool state)
{
	m_write_line_state[RegDecoder::WLASn] = state;
}
void vme_mvme320_device::WDC1n_w(bool state) 
{
	m_write_line_state[RegDecoder::WDC1n] = state;
}
void vme_mvme320_device::WDBCn_w(bool state)
{
	m_write_line_state[RegDecoder::WDBCn] = state;
	m_dbcr->me_w(state);
}
void vme_mvme320_device::WDC3n_w(bool state)
{
	m_write_line_state[RegDecoder::WDC3n] = state;
}
void vme_mvme320_device::NOP11_w(bool state) {}
void vme_mvme320_device::WDC2n_w(bool state)
{
	m_write_line_state[RegDecoder::WDC2n] = state;
}
void vme_mvme320_device::NOP13_w(bool state) {}
void vme_mvme320_device::WBUn_w(bool state) 
{
	m_write_line_state[RegDecoder::WBUn] = state;
}
void vme_mvme320_device::NOP15_w(bool state) {}

//
void vme_mvme320_device::PALDECODE::update(uint8_t vme_am, bool selected, uint8_t offset, uint8_t interrupt_level, bool write, bool cirq)
{
	// Registered PAL clocked by VMEbus DS0 + 20nS delay
	bool BDSELn;
	bool AM035, AM1;
	bool A1, A2, A3;
	bool IL1, IL2, IL3;
	bool CIRQ;
	bool VWRT;

	BDSELn = selected;
	AM035 = (vme_am & 0x29) == 0x29;
	AM1 = vme_am & 0x02;
	A1 = offset & 0x02; A2 = offset & 0x04; A3 = offset & 0x08; // 68K lines, count from A0
	IL1 = interrupt_level & 0x01; IL2 = interrupt_level & 0x02; IL3 = interrupt_level & 0x04; // VME lines, count from IL1
	CIRQ = cirq;
	VWRT = write;

	REG13RDn = AM035 & !A1 & A2 & A3 & !VWRT & !AM1 & BDSELn;
	REGSELn = (AM035 & VWRT & !AM1 & BDSELn) |
	 			(AM035 & !A2 & A3 & !VWRT & !AM1 & BDSELn) |
				(AM035 & !A3 & !VWRT & !AM1 & BDSELn);
	ILMATCHn = ((!VWRT & CIRQ) & (A1 == !IL1) & (A2 == !IL2) & (A3 == !IL3));
}

void vme_mvme320_device::PALDECODE::log(vme_mvme320_device *device)
{
	char tmp[256] = "MVME320 PALDECODE: ( ";
	if(REG13RDn) 	strcat(tmp, "/REG13RD ");
	if(REGSELn)		strcat(tmp, "/REGSEL ");
	if(ILMATCHn) 	strcat(tmp, "/ILMATCH ");
	strcat(tmp, " )\n");
	device->logerror(tmp);
}

void vme_mvme320_device::PALBUF::update(bool REGSELn, bool REG13RDn, bool VWRT, bool CWRT, bool DTBMn, bool OBG, bool DS060n, bool IACKRn)
{
	RGMND = !(!REGSELn | !DS060n);
	RDR13n = (DS060n & REG13RDn);
	TODISKn = (DTBMn & !CWRT) | (REGSELn & VWRT & DS060n);
	DBDIR = !((!DS060n & !CWRT) | (DTBMn & !CWRT) | (!DTBMn & !OBG & VWRT));
	TOMEMn = IACKRn | (DTBMn & CWRT) | (REGSELn & !VWRT & DS060n);
	I8X305n = (REGSELn & DS060n) | IACKRn;
	LDAOEn = IACKRn | DTBMn | (REGSELn & DS060n);
	CLRDTKn = !DS060n & !LDAOEn;
}

void vme_mvme320_device::PALBUF::log(vme_mvme320_device *device)
{
	char tmp[256] = "MVME320 PALBUF: ( ";
	if(RGMND) 		strcat(tmp, "RGMND ");
	if(RDR13n) 		strcat(tmp, "/RDR13 ");
	if(TODISKn) 	strcat(tmp, "/TODISK ");
	if(DBDIR) 		strcat(tmp, "DBDIR ");
	if(TOMEMn) 		strcat(tmp, "/TOMEM ");
	if(I8X305n) 	strcat(tmp, "/I8X305 ");
	if(LDAOEn) 		strcat(tmp, "/LDAOE ");
	if(CLRDTKn) 	strcat(tmp, "/CLRDTK ");
	strcat(tmp, " )\n");
	device->logerror(tmp);
}

void vme_mvme320_device::PALIACK::update(bool ILMATCHn, bool IACKIN, bool AS, bool DS020I, bool DS0, bool DS060In)
{
	DS060On = (DS0 & DS060In & DS020On);
	DS020On = (DS0 & DS020I & !DS060On) | (DS0 & DS020On);
	IACKOUTn = (AS & IACKIN & DS060On) | (AS & IACKOUTn);
	IACKRn = (IACKIN & DS060On) | (DS0 & IACKRn);
}

void vme_mvme320_device::PALIACK::log(vme_mvme320_device *device)
{
	char tmp[256] = "MVME320 PALIACK: ( ";
	if(DS060On) 	strcat(tmp, "/DS060O ");
	if(DS020On) 	strcat(tmp, "/DS020O ");
	if(IACKOUTn)	strcat(tmp, "/IACKOUT ");
	if(IACKRn)		strcat(tmp, "/IACKR ");
	strcat(tmp, " )\n");
	device->logerror(tmp);
}

void vme_mvme320_device::PALVSEQ::update(bool STARTn, bool DTBMn, bool ACKn, bool ACKSn, bool DTACK13n, bool CDTACK, bool SYSRSTn, bool N8XRSTn, bool CWRT, bool CLKI)
{

}

void vme_mvme320_device::PALVSEQ::log(vme_mvme320_device *device)
{
	char tmp[256] = "MVME320 PALVSEQ: ( ";

	strcat(tmp, " )\n");
	device->logerror(tmp);
}


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
	m_fd0->option_add("525qd", FLOPPY_525_QD);
	m_fd0->option_add("525hd", FLOPPY_525_HD);
	m_fd0->set_default_option("525qd");
	m_fd0->set_formats(floppy_image_device::default_mfm_floppy_formats);

	FLOPPY_CONNECTOR(config, m_fd1, 0);
	m_fd1->option_add("8ssdd", FLOPPY_8_SSDD);
	m_fd1->option_add("8dsdd", FLOPPY_8_DSDD);
	m_fd1->option_add("525dd", FLOPPY_525_DD);
	m_fd1->option_add("525qd", FLOPPY_525_QD);
	m_fd1->option_add("525hd", FLOPPY_525_HD);
	m_fd1->set_default_option("525qd");
	m_fd1->set_formats(floppy_image_device::default_mfm_floppy_formats);

	FLOPPY_CONNECTOR(config, m_fd2, 0);
	m_fd2->option_add("8ssdd", FLOPPY_8_SSDD);
	m_fd2->option_add("8dsdd", FLOPPY_8_DSDD);
	m_fd2->option_add("525dd", FLOPPY_525_DD);
	m_fd2->option_add("525qd", FLOPPY_525_QD);
	m_fd2->option_add("525hd", FLOPPY_525_HD);
	m_fd2->set_default_option("525qd");
	m_fd2->set_formats(floppy_image_device::default_mfm_floppy_formats);

	FLOPPY_CONNECTOR(config, m_fd3, 0);
	m_fd3->option_add("8ssdd", FLOPPY_8_SSDD);
	m_fd3->option_add("8dsdd", FLOPPY_8_DSDD);
	m_fd3->option_add("525dd", FLOPPY_525_DD);
	m_fd3->option_add("525qd", FLOPPY_525_QD);
	m_fd3->option_add("525hd", FLOPPY_525_HD);
	m_fd3->set_default_option("525qd");
	m_fd3->set_formats(floppy_image_device::default_mfm_floppy_formats);

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

void vme_dbc1_line_log(char *logdata, uint8_t data)
{
	memset(logdata, 0, 100);
	if(BIT(data, 0))
	{	
		strcat(logdata, "SEL4/MOTOR ");
	}
	if(BIT(data, 1))
	{	
		strcat(logdata, "SEL3 ");
	}
	if(BIT(data, 2))
	{	
		strcat(logdata, "SEL2 ");
	}
	if(BIT(data, 3))
	{	
		strcat(logdata, "SEL1 ");
	}
	if(BIT(data, 4))
	{	
		strcat(logdata, "SEL0 ");
	}
	if(BIT(data, 5))
	{	
		strcat(logdata, "FMODE ");
	}
	if(BIT(data, 6))
	{	
		strcat(logdata, "WGTE/REZRO ");
	}
	if(BIT(data, 7))
	{	
		strcat(logdata, "WRGATE ");
	}
}

