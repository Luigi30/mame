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
	apollo_dn300_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_mmu(*this, "mmu"),
		m_dmac(*this, "dmac"),
		m_duart(*this, "duart"),
		m_acia(*this, "acia"),
		m_fdc(*this, "fdc"),
		m_ptm(*this, "ptm"),
		m_rtc(*this, "rtc"),
		m_display(*this, "display"),
		m_kbd(*this, "kbd")
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
    
	void use_physical_map(address_map &map);
	void use_virtual_map(address_map &map);

	uint16_t mmu_r(offs_t offset);
	void mmu_w(offs_t offset, uint16_t data);

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

void apollo_dn300_state::use_physical_map(address_map &map)
{
	map.unmap_value_high();
	map(0x000000, 0xffffff).m(m_mmu, FUNC(apollo_dn300_mmu_device::physical_map));
}

void apollo_dn300_state::use_virtual_map(address_map &map)
{
	map.unmap_value_high();
	map(0x000000, 0xffffff).m(m_mmu, FUNC(apollo_dn300_mmu_device::virtual_map));
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
	m_maincpu->set_addrmap(AS_PROGRAM, &apollo_dn300_state::use_physical_map);

	APOLLO_DN300_MMU(config, m_mmu, 0);

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
