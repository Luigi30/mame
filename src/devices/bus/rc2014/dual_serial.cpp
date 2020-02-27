// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    Dual Serial Module SIO/2 by RFC2795 Ltd

**********************************************************************/

#include "emu.h"
#include "dual_serial.h"

#include "machine/clock.h"
#include "machine/z80sio.h"

#include "bus/rs232/rs232.h"

#define Z80SIO_TAG	"z80sio"
#define RS232_A_TAG "rs232a"
#define RS232_B_TAG	"rs232b"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> dual_serial_device

class dual_serial_device : public device_t, public device_rc2014_card_interface
{
public:
	// construction/destruction
	dual_serial_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	// device-specific overrides
	virtual ioport_constructor device_input_ports() const override;
	virtual void device_start() override;

	virtual void device_add_mconfig(machine_config &config) override;

private:
	// object finder
	required_device<z80sio_device> m_z80sio;
	required_device<rs232_port_device> m_rs232a;
	required_device<rs232_port_device> m_rs232b;
	rc2014_slot_device *m_slot;
};

DEFINE_DEVICE_TYPE_PRIVATE(RC2014_DUAL_SERIAL, device_rc2014_card_interface, dual_serial_device, "dual_serial", "Dual Serial Module SIO/2")


//**************************************************************************
//  CONFIGURATION SETTINGS
//**************************************************************************

    /* Jumpers that enable/disable address lines. */
static INPUT_PORTS_START(dual_serial)

INPUT_PORTS_END


//**************************************************************************
//  DEVICE DEFINITION
//**************************************************************************

//-------------------------------------------------
//  dual_serial_device - constructor
//-------------------------------------------------

dual_serial_device::dual_serial_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, RC2014_DUAL_SERIAL, tag, owner, clock)
	, device_rc2014_card_interface(mconfig, *this)
	, m_z80sio(*this, Z80SIO_TAG)
	, m_rs232a(*this, RS232_A_TAG)
	, m_rs232b(*this, RS232_B_TAG)
	, m_slot(nullptr)
{
}



//-------------------------------------------------
//  device_input_ports - input port construction
//-------------------------------------------------

ioport_constructor dual_serial_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(dual_serial);
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void dual_serial_device::device_start()
{
	m_slot = dynamic_cast<rc2014_slot_device *>(owner());
	address_space &space = m_slot->cpu().space(AS_IO);

	space.install_readwrite_handler(0x80, 0x80, read8smo_delegate(*m_z80sio, FUNC(z80sio_device::ca_r)), write8smo_delegate(*m_z80sio, FUNC(z80sio_device::ca_w)));
	space.install_readwrite_handler(0x81, 0x81, read8smo_delegate(*m_z80sio, FUNC(z80sio_device::da_r)), write8smo_delegate(*m_z80sio, FUNC(z80sio_device::da_w)));
	space.install_readwrite_handler(0x82, 0x82, read8smo_delegate(*m_z80sio, FUNC(z80sio_device::cb_r)), write8smo_delegate(*m_z80sio, FUNC(z80sio_device::cb_w)));
	space.install_readwrite_handler(0x83, 0x83, read8smo_delegate(*m_z80sio, FUNC(z80sio_device::db_r)), write8smo_delegate(*m_z80sio, FUNC(z80sio_device::db_w)));
}

void dual_serial_device::device_add_mconfig(machine_config &config)
{
	Z80SIO(config, m_z80sio, 7.3728_MHz_XTAL);
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

	clock_device &uart_clock(CLOCK(config, "uart_clock", 7.3728_MHz_XTAL));
	uart_clock.signal_handler().set(m_z80sio, FUNC(z80sio_device::txca_w));
	uart_clock.signal_handler().append(m_z80sio, FUNC(z80sio_device::rxca_w));
}