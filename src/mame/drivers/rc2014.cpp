// license:BSD-3-Clause
// copyright-holders:LuigiThirty

#include "emu.h"

#include "imagedev/harddriv.h"

#include "cpu/z80/z80.h"
#include "machine/z80sio.h"
#include "machine/clock.h"

#include "bus/ata/ataintf.h"
#include "bus/rs232/rs232.h"

#include "bus/rc2014/rc2014.h"
#include "bus/rc2014/cf.h"

#include "video/tms9928a.h"

#define Z80SIO_TAG          "z80sio"
#define RS232_A_TAG         "rs232a"
#define RS232_B_TAG         "rs232b"
#define TMS9918A_TAG		"tms9918a"
#define ATA_TAG				"ata"
#define SCREEN_TAG			"screen"
#define BUS_TAG				"rc2014"

#define MASTER_CLOCK 		7.3728_MHz_XTAL

class rc2014_state: public driver_device
{
public:
	rc2014_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_z80sio(*this, Z80SIO_TAG)
		, m_rs232a(*this, RS232_A_TAG)
		, m_rs232b(*this, RS232_B_TAG)
		, m_vdp(*this, TMS9918A_TAG)
		, m_ata(*this, ATA_TAG)
		, m_rc2014(*this, BUS_TAG)
	{ }

	void rc2014(machine_config &config);

	void init_rc2014();

private:

	DECLARE_READ8_MEMBER(mem_r);
	DECLARE_WRITE8_MEMBER(mem_w);

	virtual void machine_start() override;

	void io_map(address_map &map);
	void mem_map(address_map &map);
	void tms_map(address_map &map);

	required_device<cpu_device> m_maincpu;
	required_device<z80sio_device> m_z80sio;
	required_device<rs232_port_device> m_rs232a;
	required_device<rs232_port_device> m_rs232b;
	required_device<tms9918a_device> m_vdp;
	required_device<ata_interface_device> m_ata;
	required_device<rc2014_bus_device> m_rc2014;
};

void rc2014_state::machine_start()
{
	
};

void rc2014_state::mem_map(address_map &map)
{
	// map.unmap_value_high();
	map(0x0000, 0x1fff).rom();
	map(0x2000, 0xffff).ram();
}

void rc2014_state::io_map(address_map &map)
{
	/* I/O devices... */
	map.unmap_value_high();
	map.global_mask(0xff);

	// map(0x10, 0x17).rw(m_ata, FUNC(ata_interface_device::cs0_8b_r), FUNC(ata_interface_device::cs0_8b_w));
	map(0x10, 0x17).rw(m_rc2014, FUNC(rc2014_bus_device::io_r), FUNC(rc2014_bus_device::io_w));

	map(0x80, 0x80).rw(Z80SIO_TAG, FUNC(z80sio_device::ca_r), FUNC(z80sio_device::ca_w));
	map(0x81, 0x81).rw(Z80SIO_TAG, FUNC(z80sio_device::da_r), FUNC(z80sio_device::da_w));
	map(0x82, 0x82).rw(Z80SIO_TAG, FUNC(z80sio_device::cb_r), FUNC(z80sio_device::cb_w));
	map(0x83, 0x83).rw(Z80SIO_TAG, FUNC(z80sio_device::db_r), FUNC(z80sio_device::db_w));

	map(0x98, 0x98).rw(TMS9918A_TAG, FUNC(tms9918a_device::vram_read), FUNC(tms9918a_device::vram_write));
	map(0x99, 0x99).rw(TMS9918A_TAG, FUNC(tms9918a_device::register_read), FUNC(tms9918a_device::register_write));

	// map(0x20, 0x27).rw(m_tms, FUNC(tms34010_device::host_r), FUNC(tms34010_device::host_w));

}

static void rc2014_rc2014_devices(device_slot_interface &device)
{
	device.option_add("cf", RC2014_CF);
}

void rc2014_state::rc2014(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, MASTER_CLOCK);
	m_maincpu->set_addrmap(AS_PROGRAM, &rc2014_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &rc2014_state::io_map);

	Z80SIO(config, m_z80sio, MASTER_CLOCK);
	m_z80sio->out_txda_callback().set(m_rs232a, FUNC(rs232_port_device::write_txd));
	m_z80sio->out_rtsa_callback().set(m_rs232a, FUNC(rs232_port_device::write_rts));
	m_z80sio->out_txdb_callback().set(m_rs232b, FUNC(rs232_port_device::write_txd));
	m_z80sio->out_rtsb_callback().set(m_rs232b, FUNC(rs232_port_device::write_rts));

	RS232_PORT(config, m_rs232a, default_rs232_devices, "terminal");
	m_rs232a->rxd_handler().set(m_z80sio, FUNC(z80sio_device::rxa_w));
	m_rs232a->cts_handler().set(m_z80sio, FUNC(z80sio_device::ctsa_w));

	RS232_PORT(config, m_rs232b, default_rs232_devices, nullptr);
	m_rs232b->rxd_handler().set(m_z80sio, FUNC(z80sio_device::rxb_w));
	m_rs232b->cts_handler().set(m_z80sio, FUNC(z80sio_device::ctsb_w));

	clock_device &uart_clock(CLOCK(config, "uart_clock", MASTER_CLOCK));
	uart_clock.signal_handler().set(m_z80sio, FUNC(z80sio_device::txca_w));
	uart_clock.signal_handler().append(m_z80sio, FUNC(z80sio_device::rxca_w));

	ATA_INTERFACE(config, m_ata).options(ata_devices, "hdd", nullptr, false);

	/* video hardware */
	SCREEN(config, SCREEN_TAG, SCREEN_TYPE_RASTER);
	TMS9918A(config, m_vdp, 10.738635_MHz_XTAL);
	m_vdp->set_screen(SCREEN_TAG);
	m_vdp->set_vram_size(0x4000);
	// m_vdp->int_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);	

	RC2014_BUS(config, m_rc2014, 0);
	RC2014_SLOT(config, "rc2014:1", rc2014_rc2014_devices, "cf");
	RC2014_SLOT(config, "rc2014:2", rc2014_rc2014_devices, nullptr);
	RC2014_SLOT(config, "rc2014:3", rc2014_rc2014_devices, nullptr);
	RC2014_SLOT(config, "rc2014:4", rc2014_rc2014_devices, nullptr);
}

void rc2014_state::init_rc2014()
{

}

INPUT_PORTS_START( rc2014 )

INPUT_PORTS_END

/* ROM definition */
ROM_START( rc2014 )
	ROM_REGION( 0x2000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "rc2014.bin", 0x0000, 0x2000, CRC(dd5b9cd9) SHA1(97c176fcb63674f0592851b7858cb706886b5857))
ROM_END

/*    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT   CLASS         INIT         COMPANY        FULLNAME         FLAGS */
COMP( 2014, rc2014, 0,      0,      rc2014,  rc2014, rc2014_state, empty_init, "RFC2795 Ltd", "RC2014", MACHINE_NO_SOUND_HW)
