// license:BSD-3-Clause
// copyright-holders:AJR
/**********************************************************************

    64K RAM by RFC2795 Ltd

**********************************************************************/

#include "emu.h"
#include "ram64k.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> poly_16k_ram_device

class ram_64k_ram_device : public device_t, public device_rc2014_card_interface
{
public:
	// construction/destruction
	ram_64k_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	// device-specific overrides
	virtual ioport_constructor device_input_ports() const override;
	virtual void device_start() override;

	// memory access handlers
	virtual u8 rc2014_rd_r(offs_t offset) override;
	virtual void rc2014_wr_w(offs_t offset, u8 data) override;

	// internal state
	std::unique_ptr<u8[]> m_ram;

private:
	// object finder
	required_ioport m_dsw;
};

DEFINE_DEVICE_TYPE_PRIVATE(RC2014_RAM_64K, device_rc2014_card_interface, ram_64k_ram_device, "ram64k", "64K RAM by RFC2795 Ltd")


//**************************************************************************
//  CONFIGURATION SETTINGS
//**************************************************************************

    /* Jumpers that enable/disable address lines. */
static INPUT_PORTS_START(ram64k)
	PORT_START("DSW")
	PORT_DIPNAME(0x7, 0x2, "RAM Start Address") PORT_DIPLOCATION("JP1-JP3")
	PORT_DIPSETTING(0x0, "0000H")
    PORT_DIPSETTING(0x7, "1000H")
    PORT_DIPSETTING(0x6, "2000H")
	PORT_DIPSETTING(0x3, "4000H")
    
    PORT_DIPNAME(0x1, 0x1, "Enable Upper 32K") PORT_DIPLOCATION("JP4")
	PORT_DIPSETTING(0x0, "No")
    PORT_DIPSETTING(0x1, "Yes")
INPUT_PORTS_END


//**************************************************************************
//  DEVICE DEFINITION
//**************************************************************************

//-------------------------------------------------
//  poly_16k_ram_device - constructor
//-------------------------------------------------

ram_64k_ram_device::ram_64k_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, RC2014_RAM_64K, tag, owner, clock)
	, device_rc2014_card_interface(mconfig, *this)
	, m_dsw(*this, "DSW")
{
}


//-------------------------------------------------
//  device_input_ports - input port construction
//-------------------------------------------------

ioport_constructor ram_64k_ram_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(ram64k);
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void ram_64k_ram_device::device_start()
{
	m_ram = make_unique_clear<u8[]>(0x10000);
	save_pointer(NAME(m_ram), 0x4000);
}


//-------------------------------------------------
//  s100_smemr_r - memory read
//-------------------------------------------------

u8 ram_64k_ram_device::rc2014_rd_r(offs_t offset)
{
    return m_ram[offset];
}


//-------------------------------------------------
//  s100_mwrt_w - memory write
//-------------------------------------------------

void ram_64k_ram_device::rc2014_wr_w(offs_t offset, u8 data)
{
    m_ram[offset] = data;
}
