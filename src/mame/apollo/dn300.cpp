// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

/*
    M68010 system with a funky custom MMU.
    http://bitsavers.org/pdf/apollo/002398-01_Domain_Engineering_Handbook_Rev1_Apr83.pdf

	Video hardware:
	1024x1024 1bpp framebuffer. There's a hardware blitter.
*/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/hd63450.h"
#include "machine/mc68681.h"
#include "machine/6850acia.h"
#include "machine/upd765.h"
#include "machine/6840ptm.h"
#include "machine/mc146818.h"
#include "machine/dn300_mmu.h"
#include "machine/ram.h"
#include "video/dn300.h"
#include "apollo_kbd_new.h"
#include "apollo.h"

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

static void floppies(device_slot_interface &device)
{
	device.option_add("525sd", FLOPPY_525_SD);
	device.option_add("525dd", FLOPPY_525_DD);
}

void apollo_keyboard_devices(device_slot_interface &device)
{
	device.option_add("keyboard", APOLLO_KEYBOARD_NEW);
}

class apollo_dn300_state : public driver_device
{
public:
	apollo_dn300_state(const machine_config &mconfig, device_type type, const char *tag) : 
		driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_mmu(*this, "mmu")
		, m_dmac(*this, "dmac")
		, m_duart(*this, "duart")
		, m_acia(*this, "acia")
		, m_fdc(*this, "fdc")
		, m_ptm(*this, "ptm")
		, m_rtc(*this, "rtc")
		, m_display(*this, "display")
		, m_kbd(*this, "kbd")
		, m_view(*this, "view")
	{ }

    required_device<m68010_device> 				m_maincpu;
	required_device<apollo_dn300_mmu_device> 	m_mmu;
	required_device<hd63450_device> 			m_dmac;
	required_device<scn2681_device> 			m_duart;
	required_device<acia6850_device> 			m_acia;
	required_device<upd765a_device> 			m_fdc;
	required_device<ptm6840_device> 			m_ptm;
	required_device<mc146818_device> 			m_rtc;
	required_device<dn300_display_device>		m_display;
	required_device<rs232_port_device>			m_kbd;

    void dn300(machine_config &config);

	void init_dn300();

protected:
	void machine_start() override;
	void machine_reset() override;
    
	void physical_map(address_map &map);
	void virtual_map(address_map &map);

    void pid_priv_w(uint16_t data);

private:
	memory_view m_view;
};

void apollo_dn300_state::pid_priv_w(uint16_t data)
{
	m_view.select(BIT(data, 0));
	m_mmu->pid_priv_w(data);
}

void apollo_dn300_state::machine_reset()
{
	m_view.select(0);
}

void apollo_dn300_state::machine_start()
{
	m_view.select(0);
	// address_space &pgmspace = m_maincpu->space(AS_PROGRAM);

	// pgmspace.install_read_tap(0x000000, 0xffffff, "mmu_tap_r",[this](offs_t offset, u16 &data, u16 mem_mask)
	// {
	// 	if(m_mmu->mmu_is_enabled())
	// 	{
	// 		m_mmu->translated_read(offset, mem_mask);
	// 		// Don't call the parent read handler, we already took care of it.
	// 	}
	// 	else
	// 	{
	// 		// pass through
	// 	}
	// 	return data;
	// });

	// pgmspace.install_write_tap(0x000000, 0xffffff, "mmu_tap_w",[this](offs_t offset, u16 &data, u16 mem_mask)
	// {
	// 	if(m_mmu->mmu_is_enabled())
	// 	{
	// 		m_mmu->translated_write(offset, data, mem_mask);
	// 		// Don't call the parent write handler, we already took care of it.
	// 	}
	// 	else
	// 	{
	// 		// pass through
	// 	}
	// 	return data;
	// });
}

void apollo_dn300_state::init_dn300()
{

}

void apollo_dn300_state::virtual_map(address_map &map)
{
	map(0x000000, 0xffffff).view(m_view);

	// Physical addresses.
	m_view[0](0x000000, 0x003FFF).rom().region(":prom", 0);
	m_view[0](0x004000, 0x007FFF).rw(":mmu", FUNC(apollo_dn300_mmu_device::pft_r), FUNC(apollo_dn300_mmu_device::pft_w));
	m_view[0](0x008000, 0x008001).r(":mmu", FUNC(apollo_dn300_mmu_device::pid_priv_r));
	m_view[0](0x008000, 0x008001).w(FUNC(apollo_dn300_state::pid_priv_w));
	m_view[0](0x008002, 0x008003).rw(":mmu", FUNC(apollo_dn300_mmu_device::status_r), FUNC(apollo_dn300_mmu_device::status_w));
	m_view[0](0x008004, 0x008005).rw(":mmu", FUNC(apollo_dn300_mmu_device::mem_ctrl_r), FUNC(apollo_dn300_mmu_device::mem_ctrl_w)).umask16(0x00FF);
	m_view[0](0x008006, 0x008007).rw(":mmu", FUNC(apollo_dn300_mmu_device::mem_status_r), FUNC(apollo_dn300_mmu_device::mem_status_w));
	m_view[0](0x008400, 0x00841F).rw(":duart", FUNC(scn2681_device::read), FUNC(scn2681_device::write)).umask16(0x00FF);	// Serial I/O Interface
	m_view[0](0x008420, 0x008423).rw(":acia", FUNC(acia6850_device::read), FUNC(acia6850_device::write)).umask16(0x00FF);	// Display Keyboard Interface
	m_view[0](0x008800, 0x008803).rw(":ptm", FUNC(ptm6840_device::read), FUNC(ptm6840_device::write)); 					// TIMERS
	m_view[0](0x009000, 0x0090FF).rw(":dmac", FUNC(hd63450_device::read), FUNC(hd63450_device::write));					// DMA CTL
	m_view[0](0x009400, 0x00941F).rw(":display", FUNC(dn300_display_device::regs_r), FUNC(dn300_display_device::regs_w)); // DISP 1
	m_view[0](0x009800, 0x009803).rw(":mmu", FUNC(apollo_dn300_mmu_device::ring_2_r), FUNC(apollo_dn300_mmu_device::ring_2_w)); 	// RING 2
	// Winchester controller at 0x9C00-0x9C0F
	m_view[0](0x009C00, 0x009C0F).rw(":mmu", FUNC(apollo_dn300_mmu_device::dsk_r), FUNC(apollo_dn300_mmu_device::dsk_w));
	m_view[0](0x009C10, 0x009C15).m(":fdc", FUNC(upd765a_device::map)).umask16(0x00FF);									// FLP
	m_view[0](0x009C20, 0x009C25).rw(":rtc", FUNC(mc146818_device::read), FUNC(mc146818_device::write)).umask16(0x00FF);	// CAL
	m_view[0](0x020000, 0x03FFFF).rw(":display", FUNC(dn300_display_device::vram_r), FUNC(dn300_display_device::vram_w));
	m_view[0](0x100000, 0x3FFFFF).ram().share(":phys_mem");
	m_view[0](0x700000, 0x7FFFFF).rw(":mmu", FUNC(apollo_dn300_mmu_device::ptt_r), FUNC(apollo_dn300_mmu_device::ptt_w)); // PTT

	m_view[1](0x000000, 0xFFFFFF).rw(":mmu", FUNC(apollo_dn300_mmu_device::translated_read), FUNC(apollo_dn300_mmu_device::translated_write)).cswidth(16);
}

void apollo_dn300_state::physical_map(address_map &map)
{
	// Physical addresses.
	map(0x000000, 0x003FFF).rom().region(":prom", 0);
	map(0x004000, 0x007FFF).rw(":mmu", FUNC(apollo_dn300_mmu_device::pft_r), FUNC(apollo_dn300_mmu_device::pft_w));
	map(0x008000, 0x008001).r(":mmu", FUNC(apollo_dn300_mmu_device::pid_priv_r));
	map(0x008000, 0x008001).w(FUNC(apollo_dn300_state::pid_priv_w));
	map(0x008002, 0x008003).rw(":mmu", FUNC(apollo_dn300_mmu_device::status_r), FUNC(apollo_dn300_mmu_device::status_w));
	map(0x008004, 0x008005).rw(":mmu", FUNC(apollo_dn300_mmu_device::mem_ctrl_r), FUNC(apollo_dn300_mmu_device::mem_ctrl_w)).umask16(0x00FF);
	map(0x008006, 0x008007).rw(":mmu", FUNC(apollo_dn300_mmu_device::mem_status_r), FUNC(apollo_dn300_mmu_device::mem_status_w));
	map(0x008400, 0x00841F).rw(":duart", FUNC(scn2681_device::read), FUNC(scn2681_device::write)).umask16(0x00FF);	// Serial I/O Interface
	map(0x008420, 0x008423).rw(":acia", FUNC(acia6850_device::read), FUNC(acia6850_device::write)).umask16(0x00FF);	// Display Keyboard Interface
	map(0x008800, 0x008803).rw(":ptm", FUNC(ptm6840_device::read), FUNC(ptm6840_device::write)); 					// TIMERS
	map(0x009000, 0x0090FF).rw(":dmac", FUNC(hd63450_device::read), FUNC(hd63450_device::write));					// DMA CTL
	map(0x009400, 0x00941F).rw(":display", FUNC(dn300_display_device::regs_r), FUNC(dn300_display_device::regs_w)); // DISP 1
	map(0x009800, 0x009803).rw(":mmu", FUNC(apollo_dn300_mmu_device::ring_2_r), FUNC(apollo_dn300_mmu_device::ring_2_w)); 	// RING 2
	// Winchester controller at 0x9C00-0x9C0F
	map(0x009C00, 0x009C0F).rw(":mmu", FUNC(apollo_dn300_mmu_device::dsk_r), FUNC(apollo_dn300_mmu_device::dsk_w));
	map(0x009C10, 0x009C15).m(":fdc", FUNC(upd765a_device::map)).umask16(0x00FF);									// FLP
	map(0x009C20, 0x009C25).rw(":rtc", FUNC(mc146818_device::read), FUNC(mc146818_device::write)).umask16(0x00FF);	// CAL
	map(0x020000, 0x03FFFF).rw(":display", FUNC(dn300_display_device::vram_r), FUNC(dn300_display_device::vram_w));
	map(0x100000, 0x3FFFFF).ram().share(":phys_mem");
	map(0x700000, 0x7FFFFF).rw(":mmu", FUNC(apollo_dn300_mmu_device::ptt_r), FUNC(apollo_dn300_mmu_device::ptt_w)); // PTT
}

static DEVICE_INPUT_DEFAULTS_START(keyboard)
	DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_1200 )
	DEVICE_INPUT_DEFAULTS( "RS232_DATABITS", 0xff, RS232_DATABITS_8 )
	DEVICE_INPUT_DEFAULTS( "RS232_PARITY", 0xff, RS232_PARITY_NONE )
	DEVICE_INPUT_DEFAULTS( "RS232_STOPBITS", 0xff, RS232_STOPBITS_1 )
DEVICE_INPUT_DEFAULTS_END

void apollo_dn300_state::dn300(machine_config &config)
{
	// IRQ1: SIO
	// IRQ2: keyboard input
	// IRQ3: ring
	// IRQ4: display
	// IRQ5: disk/floppy
	// IRQ6: timers
	// IRQ7: parity error

	// DMA0: ring receive header
	// DMA1: ring receive data
	// DMA2: ring transmit
	// DMA3: Winchester/floppy

    M68010(config, m_maincpu, 16_MHz_XTAL / 2);
	m_maincpu->set_addrmap(AS_PROGRAM, &apollo_dn300_state::virtual_map);

	APOLLO_DN300_MMU(config, m_mmu, 0);
	m_mmu->set_addrmap(AS_PROGRAM, &apollo_dn300_state::physical_map);
	m_mmu->set_space(m_mmu, AS_PROGRAM);

	// DMA controller
	HD63450(config, m_dmac, 16_MHz_XTAL / 2, m_maincpu);

	// Serial I/O controller
	SCN2681(config, m_duart, 3.6864_MHz_XTAL);		// TODO: confirm speed
	m_duart->irq_cb().set_inputline(m_maincpu, M68K_IRQ_1);

	// "Display Keyboard Interface"
	ACIA6850(config, m_acia, 0);
	m_acia->irq_handler().set_inputline(m_maincpu, M68K_IRQ_2);

	clock_device &acia_clock(CLOCK(config, "acia_clock", 1200*16));
	acia_clock.signal_handler().set(m_acia, FUNC(acia6850_device::write_txc));
	acia_clock.signal_handler().append(m_acia, FUNC(acia6850_device::write_rxc));

	// Timers
	PTM6840(config, m_ptm, 3.6864_MHz_XTAL / 2);	// TODO: confirm speed
	m_ptm->irq_callback().set_inputline(m_maincpu, M68K_IRQ_6);

	// FDC
	UPD765A(config, m_fdc, 16_MHz_XTAL / 2);
	m_fdc->intrq_wr_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ5);
	m_fdc->drq_wr_callback().set(m_dmac, FUNC(hd63450_device::drq3_w));

	// floppy drives
	FLOPPY_CONNECTOR(config, "fdc:0", floppies, "525dd", floppy_image_device::default_fm_floppy_formats);
	FLOPPY_CONNECTOR(config, "fdc:1", floppies, "525dd", floppy_image_device::default_fm_floppy_formats);

	// RTC
	MC146818(config, m_rtc, 32.768_kHz_XTAL);

	// Display Controller
	DN300_DISPLAY(config, m_display, 0);

	// APOLLO_KEYBOARD_NEW(config, m_kbd, 0);
	rs232_port_device &kbd(RS232_PORT(config, "kbd", apollo_keyboard_devices, "keyboard"));
	kbd.rxd_handler().set(m_acia, FUNC(acia6850_device::write_rxd));
	kbd.set_option_device_input_defaults("keyboard", DEVICE_INPUT_DEFAULTS_NAME(keyboard));
}

////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////

ROM_START( dn300 )
	ROM_REGION16_BE(0x4000, "prom", 0)
	ROM_LOAD16_BYTE( "even.bin", 0x00000, 0x2000, CRC(76c36d1a) SHA1(c68d52a2e5fbd303225ebb006f91869b29ef700a))
	ROM_LOAD16_BYTE( "odd.bin", 0x00001, 0x2000, CRC(82cf0f7d) SHA1(13bb39225757b89749af70e881af0228673dbe0c))
ROM_END

COMP( 1989, dn300,     0,      0,      dn300, 0,  apollo_dn300_state, init_dn300,  "Apollo", "Apollo DN300", MACHINE_NOT_WORKING)
