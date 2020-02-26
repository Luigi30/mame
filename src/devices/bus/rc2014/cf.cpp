// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    CompactFlash host adapter by RFC2795 Ltd

**********************************************************************/

#include "emu.h"
#include "cf.h"

#include "bus/ata/ataintf.h"

#define ATA_TAG "ata"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> cf_host_device

class cf_host_device : public device_t, public device_rc2014_card_interface
{
public:
	// construction/destruction
	cf_host_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	// device-specific overrides
	virtual ioport_constructor device_input_ports() const override;
	virtual void device_start() override;

	virtual void device_add_mconfig(machine_config &config) override;

private:
	// object finder
	required_device<ata_interface_device> m_ata;
	rc2014_slot_device *m_slot;
};

DEFINE_DEVICE_TYPE_PRIVATE(RC2014_CF, device_rc2014_card_interface, cf_host_device, "cf", "CompactFlash host adapter")


//**************************************************************************
//  CONFIGURATION SETTINGS
//**************************************************************************

    /* Jumpers that enable/disable address lines. */
static INPUT_PORTS_START(cf)

INPUT_PORTS_END


//**************************************************************************
//  DEVICE DEFINITION
//**************************************************************************

//-------------------------------------------------
//  poly_16k_ram_device - constructor
//-------------------------------------------------

cf_host_device::cf_host_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, RC2014_CF, tag, owner, clock)
	, device_rc2014_card_interface(mconfig, *this)
	, m_ata(*this, ATA_TAG)
	, m_slot(nullptr)
{
}



//-------------------------------------------------
//  device_input_ports - input port construction
//-------------------------------------------------

ioport_constructor cf_host_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(cf);
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cf_host_device::device_start()
{
	m_slot = dynamic_cast<rc2014_slot_device *>(owner());
	address_space &space = m_slot->cpu().space(AS_IO);

	// There's no smarts on the board. It's a pass-through to the CF card.
	space.install_readwrite_handler(0x10, 0x17, read8_delegate(*m_ata, FUNC(ata_interface_device::cs0_8b_r)), write8_delegate(*m_ata, FUNC(ata_interface_device::cs0_8b_w)));
}

void cf_host_device::device_add_mconfig(machine_config &config)
{
	ATA_INTERFACE(config, m_ata).options(ata_devices, "hdd", nullptr, false);
}