// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

/*
    M68010 system with a funky custom MMU.
    http://bitsavers.org/pdf/apollo/002398-01_Domain_Engineering_Handbook_Rev1_Apr83.pdf
*/

#include "emu.h"
#include "cpu/m68000/m68000.h"

class apollo_dn300_state : public driver_device
{
public:
	apollo_dn300_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu")
	{ }

    required_device<m68010_device> m_maincpu;

    void dn300(machine_config &config);

protected:
	void machine_start() override;
    void maincpu_map(address_map &map);
    
};

void apollo_dn300_state::dn300(machine_config &config)
{
    M68010(config, m_maincpu, 8'000'000);
}

void apollo_dn300_state::maincpu_map(address_map &map)
{
	map.unmap_value_high();
	map(0x000400, 0x004FFF).rom().region("prom", 0);
}

ROM_START( dn300 )
	// ROM_REGION32_LE(0x20000, "prom", 0)
	// ROM_LOAD16_BYTE( "15f6637.bin", 0x00000, 0x10000, CRC(76c36d1a) SHA1(c68d52a2e5fbd303225ebb006f91869b29ef700a))
	// ROM_LOAD16_BYTE( "15f6639.bin", 0x00001, 0x10000, CRC(82cf0f7d) SHA1(13bb39225757b89749af70e881af0228673dbe0c))
ROM_END

COMP( 1984, dn300, 0, 0, dn300, 0, apollo_dn300_state, empty_init, "Apollo/Domain", "DN300", MACHINE_NOT_WORKING )
