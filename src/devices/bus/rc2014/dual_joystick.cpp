// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    Twin Port Joystick Module by RFC2795 Ltd

**********************************************************************/

#include "emu.h"
#include "dual_joystick.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> dual_joystick_device

class dual_joystick_device : public device_t, public device_rc2014_card_interface
{
public:
	// construction/destruction
	dual_joystick_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

    DECLARE_READ8_MEMBER( joystick_r );
protected:
	// device-specific overrides
	virtual ioport_constructor device_input_ports() const override;
	virtual void device_start() override;

	virtual void device_add_mconfig(machine_config &config) override;

private:
	// object finder
	rc2014_slot_device *m_slot;
    required_ioport m_joy0;
	required_ioport m_joy1;
};

DEFINE_DEVICE_TYPE_PRIVATE(RC2014_DUAL_JOYSTICK, device_rc2014_card_interface, dual_joystick_device, "dual_joystick", "Twin Port Joystick Module")


//**************************************************************************
//  CONFIGURATION SETTINGS
//**************************************************************************

static INPUT_PORTS_START(dual_joystick_device)
    PORT_START("P1")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START("P2")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
INPUT_PORTS_END


//**************************************************************************
//  DEVICE DEFINITION
//**************************************************************************

//-------------------------------------------------
//  dual_serial_device - constructor
//-------------------------------------------------

dual_joystick_device::dual_joystick_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, RC2014_DUAL_JOYSTICK, tag, owner, clock)
	, device_rc2014_card_interface(mconfig, *this)
	, m_slot(nullptr)
    , m_joy0(*this, "P1")
	, m_joy1(*this, "P2")
{
}

//-------------------------------------------------
//  device_input_ports - input port construction
//-------------------------------------------------

ioport_constructor dual_joystick_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(dual_joystick_device);
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void dual_joystick_device::device_start()
{
	m_slot = dynamic_cast<rc2014_slot_device *>(owner());
	address_space &space = m_slot->cpu().space(AS_IO);

    space.install_read_handler(0x01, 0x02, read8_delegate(*this, FUNC(dual_joystick_device::joystick_r)));
}

void dual_joystick_device::device_add_mconfig(machine_config &config)
{

}

READ8_MEMBER( dual_joystick_device::joystick_r )
{
	uint8_t data = 0xff;

    if(offset == 0x00) data = m_joy0->read();
    if(offset == 0x01) data = m_joy1->read();

	return data;
}