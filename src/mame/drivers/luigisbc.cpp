#include "emu.h"
#include "bus/rs232/rs232.h"
#include "cpu/m68000/m68000.h"
#include "machine/mc68901.h"

#define MASTER_CLOCK      XTAL(8'000'000)
#define Y1      XTAL(2'457'600)

class luigisbc_state : public driver_device
{
	public:
	luigisbc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
	{ }

	void luigisbc(machine_config &config);
	void luigisbc_mem(address_map &map);
private:
	required_device<cpu_device> m_maincpu;
};

void luigisbc_state::luigisbc_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x000000, 0x0fffff).rom();
	map(0x100000, 0x1fffff).ram();
	map(0x600000, 0x60003f).rw("mfp", FUNC(mc68901_device::read), FUNC(mc68901_device::write)).umask16(0x00ff);
}

/* Input ports */
static INPUT_PORTS_START( luigisbc )
INPUT_PORTS_END

MACHINE_CONFIG_START(luigisbc_state::luigisbc)
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, MASTER_CLOCK)
	MCFG_CPU_PROGRAM_MAP(luigisbc_mem)

	MCFG_DEVICE_ADD("mfp", MC68901, MASTER_CLOCK/2)
	MCFG_MC68901_TIMER_CLOCK(Y1)
	MCFG_MC68901_RX_CLOCK(9600)
	MCFG_MC68901_TX_CLOCK(9600)
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
	ROM_LOAD( "68k.bin", 0x000000, 0x100000, CRC(00000000) SHA1(0) )
ROM_END

/* Driver */

/*    YEAR  NAME        PARENT  COMPAT MACHINE     INPUT     CLASS            INIT  COMPANY         FULLNAME                    FLAGS */
COMP( 2018, luigisbc,   0,       0,    luigisbc,   luigisbc, luigisbc_state,  0,    "Luigi Thirty", "68000 SBC (h/w 4/7/2018)", MACHINE_NO_SOUND_HW)