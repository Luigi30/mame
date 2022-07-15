// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

/*
    M68010 system with a funky custom MMU.
    http://bitsavers.org/pdf/apollo/002398-01_Domain_Engineering_Handbook_Rev1_Apr83.pdf
*/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/hd63450.h"
#include "machine/mc68681.h"
#include "machine/6850acia.h"
#include "machine/upd765.h"
#include "machine/6840ptm.h"
#include "machine/mc146818.h"

#define LOG_GENERAL 0x01
#define LOG_SETUP   0x02

#define VERBOSE (LOG_SETUP | LOG_GENERAL)

#include "logmacro.h"

#define LOG(...)      LOGMASKED(LOG_GENERAL, __VA_ARGS__)
#define LOGSETUP(...) LOGMASKED(LOG_SETUP,   __VA_ARGS__)

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

class apollo_dn300_state : public driver_device
{
public:
	apollo_dn300_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_dmac(*this, "dmac"),
		m_duart(*this, "duart"),
		m_acia(*this, "acia"),
		m_fdc(*this, "fdc"),
		m_ptm(*this, "ptm"),
		m_rtc(*this, "rtc")
	{ }

    required_device<m68010_device> 		m_maincpu;
	required_device<hd63450_device> 	m_dmac;
	required_device<scn2681_device> 	m_duart;
	required_device<acia6850_device> 	m_acia;
	required_device<upd765a_device> 	m_fdc;
	required_device<ptm6840_device> 	m_ptm;
	required_device<mc146818_device> 	m_rtc;

    void dn300(machine_config &config);

	void init_dn300();

protected:
	void machine_start() override;
    void maincpu_map(address_map &map);

	uint16_t pft_r(offs_t offset);
	uint16_t big_pft_r(offs_t offset);
	uint16_t mmu_pid_r(offs_t offset);
	uint16_t mmu_status_r(offs_t offset);
	uint8_t mem_ctrl_r(offs_t offset);
	uint16_t mem_status_r(offs_t offset);
	uint16_t sios_r(offs_t offset);
	uint16_t timers_r(offs_t offset);
	uint16_t disp_1_r(offs_t offset);
	uint16_t ring_2_r(offs_t offset);
	uint16_t flp_win_cal_r(offs_t offset);
	uint16_t ptt_r(offs_t offset);

	void pft_w(offs_t offset, uint16_t data);
	void big_pft_w(offs_t offset, uint16_t data);
	void mmu_pid_w(offs_t offset, uint16_t data);
	void mmu_status_w(offs_t offset, uint16_t data);
	void mem_ctrl_w(offs_t offset, uint8_t data);
	void mem_status_w(offs_t offset, uint16_t data);
	void sios_w(offs_t offset, uint16_t data);
	void timers_w(offs_t offset, uint16_t data);
	void disp_1_w(offs_t offset, uint16_t data);
	void ring_2_w(offs_t offset, uint16_t data);
	void flp_win_cal_w(offs_t offset, uint16_t data);
	void ptt_w(offs_t offset, uint16_t data);

private:
	uint16_t m_pid_priv;
	uint16_t m_mmu_status;

	uint8_t m_mcr;
	uint8_t m_msr;
};

void apollo_dn300_state::machine_start()
{


}

void apollo_dn300_state::init_dn300()
{

}

void apollo_dn300_state::dn300(machine_config &config)
{
	// IRQ1: SIO
	// IRQ2: keyboard input
	// IRQ3: ring
	// IRQ4: display
	// IRQ5: disk/floppy
	// IRQ6: timers
	// IRQ7: parity error

    M68010(config, m_maincpu, 16_MHz_XTAL / 2);
	m_maincpu->set_addrmap(AS_PROGRAM, &apollo_dn300_state::maincpu_map);

	// DMA controller
	HD63450(config, m_dmac, 16_MHz_XTAL / 2, m_maincpu);
	m_dmac->irq_callback().set_inputline(m_maincpu, M68K_IRQ_3);

	// Serial I/O controller
	SCN2681(config, m_duart, 3.6864_MHz_XTAL);		// TODO: confirm speed
	m_duart->irq_cb().set_inputline(m_maincpu, M68K_IRQ_1);

	// "Display Keyboard Interface"
	ACIA6850(config, m_acia, 0);
	m_acia->irq_handler().set_inputline(m_maincpu, M68K_IRQ_2);

	// Timers
	PTM6840(config, m_ptm, 3.6864_MHz_XTAL / 2);	// TODO: confirm speed
	m_ptm->irq_callback().set_inputline(m_maincpu, M68K_IRQ_6);

	// FDC
	UPD765A(config, m_fdc, 3.6864_MHz_XTAL);

	// RTC
	MC146818(config, m_rtc, 32.768_kHz_XTAL);
}

void apollo_dn300_state::maincpu_map(address_map &map)
{
	// Physical addresses.
	map.unmap_value_high();
	map(0x000000, 0x003FFF).rom().region("prom", 0);
	map(0x004000, 0x004FFF).rw(FUNC(apollo_dn300_state::pft_r), FUNC(apollo_dn300_state::pft_w)); 					// PFT
	map(0x005000, 0x007FFF).rw(FUNC(apollo_dn300_state::big_pft_r), FUNC(apollo_dn300_state::big_pft_w)); 			// Big PFT
	map(0x008000, 0x008001).rw(FUNC(apollo_dn300_state::mmu_pid_r), FUNC(apollo_dn300_state::mmu_pid_w)); 			// MMU
	map(0x008002, 0x008003).rw(FUNC(apollo_dn300_state::mmu_status_r), FUNC(apollo_dn300_state::mmu_status_w)); // MMU
	map(0x008004, 0x008005).rw(FUNC(apollo_dn300_state::mem_ctrl_r), FUNC(apollo_dn300_state::mem_ctrl_w)).umask16(0x00FF);
	map(0x008006, 0x008007).rw(FUNC(apollo_dn300_state::mem_status_r), FUNC(apollo_dn300_state::mem_status_w));
	map(0x008400, 0x00841F).rw(m_duart, FUNC(scn2681_device::read), FUNC(scn2681_device::write)).umask16(0x00FF);	// Serial I/O Interface
	map(0x008420, 0x008423).rw(m_acia, FUNC(acia6850_device::read), FUNC(acia6850_device::write)).umask16(0x00FF);	// Display Keyboard Interface
	map(0x008800, 0x008803).rw(FUNC(apollo_dn300_state::timers_r), FUNC(apollo_dn300_state::timers_w)); 			// TIMERS
	map(0x009000, 0x0090FF).rw(m_dmac, FUNC(hd63450_device::read), FUNC(hd63450_device::write));					// DMA CTL
	map(0x009400, 0x00940F).rw(FUNC(apollo_dn300_state::disp_1_r), FUNC(apollo_dn300_state::disp_1_w)); 			// DISP 1
	map(0x009800, 0x009803).rw(FUNC(apollo_dn300_state::ring_2_r), FUNC(apollo_dn300_state::ring_2_w)); 			// RING 2
	// Winchester controller at 0x9C00-0x9C0F
	map(0x009C10, 0x009C15).m(m_fdc, FUNC(upd765a_device::map)).umask16(0x00FF);									// FLP
	map(0x009C20, 0x009C25).rw(m_rtc, FUNC(mc146818_device::read), FUNC(mc146818_device::write)).umask16(0x00FF);	// CAL
	map(0x020000, 0x03FFFF).ram().share("displ_mem");
	map(0x100000, 0x1FFFFF).ram().share("phys_mem");
	map(0x700000, 0x7FFFFF).rw(FUNC(apollo_dn300_state::ptt_r), FUNC(apollo_dn300_state::ptt_w)); 					// PTT
}

////////////////////////////////////////////////////////////////////////////

uint16_t apollo_dn300_state::pft_r(offs_t offset)
{
	/* page frame table
	 *	
	 * entry format:
	 * b31-b25: address space ID
	 * b24: supervisor domain
	 * b23: domain
	 * b22: write access
	 * b21: read access
	 * b20: execute access
	 * b19-b16: excess virtual page number
	 * b15: end of chain
	 * b14: page is modified
	 * b13: page is referenced
	 * b12: page is global
	 * b11-b0: PFT hash thread
	 * 
	 * One entry per physical page of memory.
	 */

	LOG("%s O:%04X\n", FUNCNAME, offset);
	return 0;
}

uint16_t apollo_dn300_state::big_pft_r(offs_t offset)
{
	LOG("%s O:%04X\n", FUNCNAME, offset);
	return 0;
}

uint16_t apollo_dn300_state::mmu_pid_r(offs_t offset)
{
	/*
	 * b8-b14: ASID
	 * b3: domain
	 * b2: enable PTT access
	 * b1: enable MMU
	 */

	LOG("%s O:%X\n", FUNCNAME, offset);
	return 0;
}

uint16_t apollo_dn300_state::mmu_status_r(offs_t offset)
{
	/*
	 * b7: access violation
	 * b6: page fault
	 * b5: bus timeout
	 * b4: normal mode
	 * b3: interrupt pending
	 * b2: 4K PFT
	 * b1: PTT access enabled
	 * b0: MMU enabled
	 */

	LOG("%s O:%X\n", FUNCNAME, offset);
	return 0;
}

uint8_t apollo_dn300_state::mem_ctrl_r(offs_t offset)
{
	/*
	 * b7-b4: LEDs (top to bottom)
	 * b3: parity during DMA cycle
	 * b2: force left byte parity
	 * b1: force right byte parity
	 * b0: disable parity err traps
	 */

	LOG("%s O:%X\n", FUNCNAME, offset);

	return m_mcr;
}

uint16_t apollo_dn300_state::mem_status_r(offs_t offset)
{
	/*
	 * b15-b4: failing PPN
	 * b3: reserved
	 * b2: left byte parity error
	 * b1: right byte parity error
	 * b0: parity error traps enabled
	 */

	LOG("%s O:%X\n", FUNCNAME, offset);
	return m_msr;
}

uint16_t apollo_dn300_state::sios_r(offs_t offset)
{
	LOG("%s O:%X\n", FUNCNAME, offset);
	return 0;
}

uint16_t apollo_dn300_state::timers_r(offs_t offset)
{
	LOG("%s O:%X\n", FUNCNAME, offset);
	return 0;
}

uint16_t apollo_dn300_state::disp_1_r(offs_t offset)
{
	/* 
	 * When read: Display Status (page 3-4 in Rev.1 manual)
	 *
	 * b15: blit in progress
	 * b7: end of frame interrupt
	 * b1: reserved
	 * b0: reserved
	 */

	LOG("%s O:%X\n", FUNCNAME, offset);
	return 0;
}

uint16_t apollo_dn300_state::ring_2_r(offs_t offset)
{
	LOG("%s O:%X\n", FUNCNAME, offset);
	return 0;
}

uint16_t apollo_dn300_state::flp_win_cal_r(offs_t offset)
{
	LOG("%s O:%X\n", FUNCNAME, offset);
	return 0;
}

uint16_t apollo_dn300_state::ptt_r(offs_t offset)
{
	/* Page Translation Table
	 *
	 * entry format:
	 * b11-b0: physical page number
	 * 
	 * One PPTE every 1024 bytes in table.
	 */

	LOG("%s O:%06X\n", FUNCNAME, offset);
	return 0;
}

void apollo_dn300_state::pft_w(offs_t offset, uint16_t data)
{
	
	LOG("%s O:%04X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_state::big_pft_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%04X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_state::mmu_pid_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_state::mmu_status_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_state::mem_ctrl_w(offs_t offset, uint8_t data)
{
	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);

	m_mcr = data & 0xFF;

	LOG("%s: LED state %d%d%d%d\n", FUNCNAME, BIT(m_mcr, 7), BIT(m_mcr, 6), BIT(m_mcr, 5), BIT(m_mcr, 4));
}

void apollo_dn300_state::mem_status_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_state::sios_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_state::timers_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_state::disp_1_w(offs_t offset, uint16_t data)
{
	/* 
	 * When written: Display Control (page 3-4 in Rev.1 manual)
	 *
	 * b15: GO (start blit operation)
	 * b5: interrupt at end of frame?
	 * b4: interrupt at end of blit?
	 * b3: increment Y coord?
	 * b2: increment X coord?
	 * b1: fill-mode blit?
	 * b0: display enabled?
	 */

	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_state::ring_2_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_state::flp_win_cal_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_state::ptt_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%06X D:%04X\n", FUNCNAME, offset, data);
}

////////////////////////////////////////////////////////////////////////////

ROM_START( dn300 )
	ROM_REGION16_BE(0x4000, "prom", 0)
	ROM_LOAD16_BYTE( "even.bin", 0x00000, 0x2000, CRC(76c36d1a) SHA1(c68d52a2e5fbd303225ebb006f91869b29ef700a))
	ROM_LOAD16_BYTE( "odd.bin", 0x00001, 0x2000, CRC(82cf0f7d) SHA1(13bb39225757b89749af70e881af0228673dbe0c))
ROM_END

COMP( 1989, dn300,     0,      0,      dn300, 0,  apollo_dn300_state, init_dn300,  "Apollo", "Apollo DN300", MACHINE_NOT_WORKING)
