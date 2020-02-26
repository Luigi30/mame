// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    RC2014 bus

**********************************************************************/

#include "emu.h"
#include "rc2014.h"


//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

DEFINE_DEVICE_TYPE(RC2014_BUS,  rc2014_bus_device,  "rc2014_bus",  "RC2014 bus")
DEFINE_DEVICE_TYPE(RC2014_SLOT, rc2014_slot_device, "rc2014_slot", "RC2014 slot")



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  device_s100_card_interface - constructor
//-------------------------------------------------

device_rc2014_card_interface::device_rc2014_card_interface(const machine_config &mconfig, device_t &device) :
	device_interface(device, "rc2014bus"),
	m_bus(nullptr),
	m_next(nullptr)
{
}

void device_rc2014_card_interface::interface_pre_start()
{
	if (!m_bus)
		throw device_missing_dependencies();
}


//-------------------------------------------------
//  s100_slot_device - constructor
//-------------------------------------------------
rc2014_slot_device::rc2014_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, RC2014_SLOT, tag, owner, clock),
	device_single_card_slot_interface<device_rc2014_card_interface>(mconfig, *this),
	m_bus(*this, DEVICE_SELF_OWNER),
	m_cpu(*this, finder_base::DUMMY_TAG)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void rc2014_slot_device::device_start()
{
	device_rc2014_card_interface *const dev = get_card_device();
	if (dev)
		m_bus->add_card(dev);
}


//-------------------------------------------------
//  s100_bus_device - constructor
//-------------------------------------------------

rc2014_bus_device::rc2014_bus_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, RC2014_BUS, tag, owner, clock),
	m_write_irq(*this),
	m_write_m1(*this),
	m_write_mreq(*this),
	m_write_ireq(*this),
	m_write_wr(*this),
	m_write_rd(*this),
	m_write_usr1(*this),
	m_write_usr2(*this),
	m_write_usr3(*this),
	m_write_usr4(*this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void rc2014_bus_device::device_start()
{
	// resolve callbacks
	m_write_irq.resolve_safe();

}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void rc2014_bus_device::device_reset()
{
}


//-------------------------------------------------
//  add_card - add card
//-------------------------------------------------

void rc2014_bus_device::add_card(device_rc2014_card_interface *card)
{
	card->m_bus = this;
	m_device_list.append(*card);
}


//-------------------------------------------------
//  rd_r - memory read
//-------------------------------------------------

uint8_t rc2014_bus_device::rd_r(offs_t offset)
{
	uint8_t data = 0xff;

	device_rc2014_card_interface *entry = m_device_list.first();

	while (entry)
	{
		data &= entry->rc2014_rd_r(offset);
		entry = entry->next();
	}

	return data;
}


//-------------------------------------------------
//  wr_w - memory write
//-------------------------------------------------

void rc2014_bus_device::wr_w(offs_t offset, uint8_t data)
{
	device_rc2014_card_interface *entry = m_device_list.first();

	while (entry)
	{
		entry->rc2014_wr_w(offset, data);
		entry = entry->next();
	}
}

//-------------------------------------------------
//  sinp_r - I/O read
//-------------------------------------------------

uint8_t rc2014_bus_device::io_r(offs_t offset)
{
	uint8_t data = 0xff;

	device_rc2014_card_interface *entry = m_device_list.first();

	while (entry)
	{
		data &= entry->rc2014_io_r(offset);
		entry = entry->next();
	}

	return data;
}


//-------------------------------------------------
//  sout_w - I/O write
//-------------------------------------------------

void rc2014_bus_device::io_w(offs_t offset, uint8_t data)
{
	device_rc2014_card_interface *entry = m_device_list.first();

	while (entry)
	{
		entry->rc2014_io_w(offset, data);
		entry = entry->next();
	}
}
