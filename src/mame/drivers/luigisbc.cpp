#include "emu.h"
#include "bus/rs232/rs232.h"
#include "cpu/m68000/m68000.h"
#include "machine/mc68901.h"

#include "bus/isa/cga.h"
#include "bus/isa/ega.h"
#include "bus/isa/fdc.h"
#include "bus/isa/isa.h"
#include "bus/isa/isa_cards.h"
#include "bus/isa/lpt.h"
#include "bus/isa/mda.h"
#include "bus/isa/vga.h"
#include "bus/isa/svga_tseng.h"

#define MASTER_CLOCK      XTAL(8'000'000)
#define Y1      XTAL(2'457'600)

#define M68K_TAG "maincpu"
#define ISABUS_TAG "isa"

class luigisbc_state : public driver_device
{
	public:
	luigisbc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, M68K_TAG)
		, m_isa(*this, ISABUS_TAG)
	{ }

	void luigisbc(machine_config &config);
	void luigisbc_mem(address_map &map);
	
	DECLARE_WRITE_LINE_MEMBER(irq5_w);
	
private:
	virtual void machine_reset() override;

	required_device<cpu_device> m_maincpu;
	required_device<isa16_device> m_isa;
	
	DECLARE_WRITE8_MEMBER(bitmask_ttl_w);
	
	void irq5_update();
	bool m_irq5_isa;
};

class luigi010_state : public driver_device
{
	public:
	luigi010_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
	{ }

	void luigi010(machine_config &config);
	void luigi010_mem(address_map &map);
private:
	required_device<cpu_device> m_maincpu;
};

void luigisbc_state::luigisbc_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x000000, 0x0fffff).rom();
	map(0x100000, 0x1fffff).ram();
	map(0x600000, 0x60003f).rw("mfp", FUNC(mc68901_device::read), FUNC(mc68901_device::write)).umask16(0x00ff);
	
	// Schematiced, but not built.
	//map(0x800000, 0x9fffff).rw(m_isa, FUNC(isa8_device::mem_r), FUNC(isa8_device::mem_w)).umask16(0x00ff);
	//map(0xfa0000, 0xfbffff).rw(m_isa, FUNC(isa8_device::io_r), FUNC(isa8_device::io_w)).umask16(0x00ff);
	map(0x800000, 0x9fffff).rw(m_isa, FUNC(isa16_device::mem_r), FUNC(isa16_device::mem_w)).umask16(0xffff);
	map(0xfa0000, 0xfbffff).rw(m_isa, FUNC(isa16_device::io_r), FUNC(isa16_device::io_w)).umask16(0x00ff);
	
	map(0xf00000, 0xf00001).w(this, FUNC(luigisbc_state::bitmask_ttl_w));
	
}

WRITE8_MEMBER(luigisbc_state::bitmask_ttl_w){
	//printf("bitmask_ttl_w: %x\n", data);
	
	//TTL circuit masks off the high 4 bits, then passes the result in to the VGA bitmask register.
	
	m_isa->io_w(space, 0x3CE, 0x08, 0xFF);
	m_isa->io_w(space, 0x3CF, (0x80 >> (data & 0x07)), 0xFF);
}

void luigi010_state::luigi010_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x000000, 0x0fffff).rom();
	map(0x100000, 0x1fffff).ram();
	map(0x600000, 0x60003f).rw("mfp", FUNC(mc68901_device::read), FUNC(mc68901_device::write)).umask16(0x00ff);
}

void luigisbc_state::machine_reset()
{
	m_irq5_isa = CLEAR_LINE;
}

void luigisbc_state::irq5_update()
{
	if (m_irq5_isa)
	{
		m_maincpu->set_input_line(M68K_IRQ_5, ASSERT_LINE);
	}
	else
	{
		m_maincpu->set_input_line(M68K_IRQ_5, CLEAR_LINE);
	}
}

WRITE_LINE_MEMBER(luigisbc_state::irq5_w)
{
	m_irq5_isa = state;
	irq5_update();
}

/* Input ports */
static INPUT_PORTS_START( luigisbc )
INPUT_PORTS_END

// these are cards supported by the HUMBUG and Monk BIOSes
SLOT_INTERFACE_START( luigisbc_isa16_cards )
//	SLOT_INTERFACE("mda", ISA8_MDA)
//	SLOT_INTERFACE("cga", ISA8_CGA)
//	SLOT_INTERFACE("ega", ISA8_EGA)
	SLOT_INTERFACE("vga", ISA8_VGA)
	SLOT_INTERFACE("svga_et4k", ISA8_SVGA_ET4K)
	SLOT_INTERFACE("svga_et4k_16", ISA16_SVGA_ET4K)
SLOT_INTERFACE_END

MACHINE_CONFIG_START(luigisbc_state::luigisbc)
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, MASTER_CLOCK)
	MCFG_CPU_PROGRAM_MAP(luigisbc_mem)

	MCFG_DEVICE_ADD("mfp", MC68901, MASTER_CLOCK/2)
	MCFG_MC68901_TIMER_CLOCK(Y1)
	MCFG_MC68901_RX_CLOCK(9600)
	MCFG_MC68901_TX_CLOCK(9600)
	MCFG_MC68901_OUT_SO_CB(DEVWRITELINE("rs232", rs232_port_device, write_txd))
	
	MCFG_DEVICE_ADD(ISABUS_TAG, ISA16, 0)
	MCFG_ISA16_CPU(":" M68K_TAG)
	MCFG_ISA16_BUS_CUSTOM_SPACES()
	MCFG_ISA_OUT_IRQ5_CB(WRITELINE(luigisbc_state, irq5_w))
	MCFG_ISA16_SLOT_ADD(ISABUS_TAG, "isa1", luigisbc_isa16_cards, "svga_et4k_16", false)

	MCFG_RS232_PORT_ADD("rs232", default_rs232_devices, "terminal")
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("mfp", mc68901_device, write_rx))
	MCFG_RS232_DCD_HANDLER(DEVWRITELINE("mfp", mc68901_device, i1_w))
	MCFG_RS232_CTS_HANDLER(DEVWRITELINE("mfp", mc68901_device, i2_w))
	MCFG_RS232_RI_HANDLER(DEVWRITELINE("mfp", mc68901_device, i6_w))
MACHINE_CONFIG_END

MACHINE_CONFIG_START(luigi010_state::luigi010)
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, MASTER_CLOCK)
	MCFG_CPU_PROGRAM_MAP(luigi010_mem)

	MCFG_DEVICE_ADD("mfp", MC68901, MASTER_CLOCK/2)
	MCFG_MC68901_TIMER_CLOCK(Y1)
	MCFG_MC68901_RX_CLOCK(0)
	MCFG_MC68901_TX_CLOCK(0)
//	MCFG_MC68901_OUT_IRQ_CB(INPUTLINE(M68000_TAG, M68K_IRQ_6))
	MCFG_MC68901_OUT_SO_CB(DEVWRITELINE("rs232", rs232_port_device, write_txd))

	MCFG_RS232_PORT_ADD("rs232", default_rs232_devices, "terminal")
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("mfp", mc68901_device, write_rx))
	MCFG_RS232_DCD_HANDLER(DEVWRITELINE("mfp", mc68901_device, i1_w))
	MCFG_RS232_CTS_HANDLER(DEVWRITELINE("mfp", mc68901_device, i2_w))
	MCFG_RS232_RI_HANDLER(DEVWRITELINE("mfp", mc68901_device, i6_w))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( luigisbc )
	ROM_REGION(0x1000000, "maincpu", 0)
	ROM_LOAD( "68000.bin", 0x000000, 0x100000, CRC(00000000) SHA1(0) )
ROM_END

ROM_START( luigi010 )
	ROM_REGION(0x1000000, "maincpu", 0)
	ROM_LOAD( "68010.bin", 0x000000, 0x100000, CRC(00000000) SHA1(0) )
ROM_END

/* Driver */

/*    YEAR  NAME        PARENT  COMPAT MACHINE     INPUT     CLASS            INIT  COMPANY         FULLNAME                    FLAGS */
COMP( 2018, luigisbc,   0,       0,    luigisbc,   luigisbc, luigisbc_state,  0,    "Luigi Thirty", "Procyon 68000 (h/w 4/7/2018)", MACHINE_NO_SOUND_HW)
COMP( 2018, luigi010,   0,       0,    luigi010,   luigisbc, luigi010_state,  0,    "Luigi Thirty", "68010 SBC (idealized)", MACHINE_NO_SOUND_HW)