// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

  Procyon 68000 homebrew single-board computer.
 
  Driver by Katherine Rohl.
  
  Homebrew 68000-based computer that I designed.
  
  This driver is to document its build and to act as an emulator platform for
  BIOS and software development for the system.

***************************************************************************

  CPU:
  8MHz MC68000

  RAM:
  2x AS6C1008-55

  Flash ROM:
  2x SST39SF040
  
  Support:
  1x MC68901 MFP
  1x HD63450 4-channel DMA controller
  
  Slot devices:
  2x 16-bit ISA slot

  Memory Map:
  0x000000-0x0fffff - ROM
  0x100000-0x1fffff - RAM
  0x600000-0x60003f - MFP registers
  0x800000-0x9fffff - ISA memory address space
  0xf00000-0xf01fff - DMA controller
  0xfa0000-0xfbffff - ISA I/O address space
    
  I/O:
  RS-232 terminal, 9600 bps, 8/N/1
  
  Video:
  Tseng ET4000-compatible device on the ISA bus.

  Sound:
  None (yet)
  
  Usage:
  Doesn't do very much yet. 
  Type X in the monitor to run an ELF program built into the BIOS.

***************************************************************************/

#include "emu.h"
#include "bus/rs232/rs232.h"
#include "cpu/m68000/m68000.h"
#include "machine/mc68901.h"
#include "machine/hd63450.h"

#include "bus/isa/com.h"
#include "bus/isa/cga.h"
#include "bus/isa/ega.h"
#include "bus/isa/fdc.h"
#include "bus/isa/isa.h"
#include "bus/isa/isa_cards.h"
#include "bus/isa/lpt.h"
#include "bus/isa/mda.h"
#include "bus/isa/vga.h"
#include "bus/isa/svga_tseng.h"

#define MASTER_CLOCK	XTAL(24'000'000)
#define Y1      		XTAL(2'457'600)
#define Y2				XTAL(1'024'000)

#define M68K_TAG "maincpu"
#define ISABUS_TAG "isa"
#define MFP_TAG "mfp"
#define RS232_TAG "rs232"

class luigisbc_state : public driver_device
{
	public:
	luigisbc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, M68K_TAG)
		, m_mfp(*this, MFP_TAG)
		, m_isa(*this, ISABUS_TAG)
		, m_hd63450(*this, "hd63450")
	{ }

	void luigisbc(machine_config &config);
	void luigisbc_mem(address_map &map);
	
	DECLARE_WRITE_LINE_MEMBER(irq2_w);
	DECLARE_WRITE_LINE_MEMBER(irq4_w);
	
	// DMA
	void dma_irq(int channel);
	DECLARE_WRITE8_MEMBER(dma_end);
	DECLARE_WRITE8_MEMBER(dma_error);
	
	IRQ_CALLBACK_MEMBER(luigisbc_int_ack);
	
	int m_current_vector[8];
	uint8_t m_current_irq_line;
	
private:
	virtual void machine_reset() override;

	required_device<cpu_device> m_maincpu;
	required_device<mc68901_device> m_mfp;
	required_device<isa16_device> m_isa;
	required_device<hd63450_device> m_hd63450;
	
	DECLARE_WRITE16_MEMBER(isa1_dma_w);
	DECLARE_READ16_MEMBER(isa1_dma_r);
	
	void irq2_update();
	bool m_irq2_isa;
	
	void irq4_update();
	bool m_irq4_isa;
	
	void irq6_update();
	bool m_irq6_isa;
};

/*
class luigi012_state : public luigisbc_device
{
	luigi012_state(const machine_config &mconfig, device_type type, const char *tag)
		: luigisbc_state(mconfig, type, tag)
	{ }

	
	void luigi012(machine_config &config);
	void luigi012_map(address_map &map);
};
*/

void luigisbc_state::luigisbc_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x000000, 0x0fffff).rom();
	map(0x100000, 0x1fffff).ram();
	map(0x600000, 0x60003f).rw(MFP_TAG, FUNC(mc68901_device::read), FUNC(mc68901_device::write)).umask16(0x00ff);
	
	// Schematiced, but not built.
	map(0x800000, 0x9fffff).rw(m_isa, FUNC(isa16_device::mem_r), FUNC(isa16_device::mem_w));
	map(0xfa0000, 0xfbffff).rw(m_isa, FUNC(isa16_device::io_r), FUNC(isa16_device::io_w)).umask16(0x00ff);
	
	map(0xf00000, 0xf01fff).rw(m_hd63450, FUNC(hd63450_device::read), FUNC(hd63450_device::write));
	//map(0xf00000, 0xf00001).w(this, FUNC(luigisbc_state::bitmask_ttl_w));
}

/*
void luigi012_state::luigi012_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x00000000, 0x000fffff).rom();
	map(0x00100000, 0x001fffff).ram();
	map(0x00600000, 0x0060003f).rw(MFP_TAG, FUNC(mc68901_device::read), FUNC(mc68901_device::write)).umask16(0x00ff);
	
	// Schematiced, but not built.
	map(0x00800000, 0x009fffff).rw(m_isa, FUNC(isa16_device::mem_r), FUNC(isa16_device::mem_w));
	map(0x00fa0000, 0x00fbffff).rw(m_isa, FUNC(isa16_device::io_r), FUNC(isa16_device::io_w)).umask16(0x00ff);
	
	map(0x00f00000, 0x00f01fff).rw(m_hd63450, FUNC(hd63450_device::read), FUNC(hd63450_device::write));
	//map(0x00f00000, 0x00f00001).w(this, FUNC(luigisbc_state::bitmask_ttl_w));
}
*/

void luigisbc_state::machine_reset()
{
	m_irq2_isa = CLEAR_LINE;
	m_irq4_isa = CLEAR_LINE;
}

void luigisbc_state::irq2_update()
{
	if (m_irq2_isa)
	{
		m_maincpu->set_input_line(M68K_IRQ_6, ASSERT_LINE);
	}
	else
	{
		m_maincpu->set_input_line(M68K_IRQ_6, CLEAR_LINE);
	}
}

void luigisbc_state::irq4_update()
{
	if (m_irq4_isa)
	{
		m_maincpu->set_input_line(M68K_IRQ_4, ASSERT_LINE);
	}
	else
	{
		m_maincpu->set_input_line(M68K_IRQ_4, CLEAR_LINE);
	}
}

WRITE_LINE_MEMBER(luigisbc_state::irq2_w)
{
	m_irq2_isa = state;
	irq2_update();
}

WRITE_LINE_MEMBER(luigisbc_state::irq4_w)
{
	m_irq4_isa = state;
	irq4_update();
}

/* Input ports */
static INPUT_PORTS_START( luigisbc )
INPUT_PORTS_END

void luigisbc_isa16_cards(device_slot_interface &device)
{
	device.option_add("vga", ISA8_VGA);
	device.option_add("svga_et4k", ISA8_SVGA_ET4K);
	device.option_add("svga_et4k_16", ISA16_SVGA_ET4K);
	device.option_add("com", ISA8_COM);
}

MACHINE_CONFIG_START(luigisbc_state::luigisbc)
	/* basic machine hardware */
	MCFG_DEVICE_ADD(M68K_TAG, M68000, MASTER_CLOCK/2)
	MCFG_DEVICE_PROGRAM_MAP(luigisbc_mem)
	MCFG_DEVICE_IRQ_ACKNOWLEDGE_DRIVER(luigisbc_state,luigisbc_int_ack)

	MCFG_DEVICE_ADD(MFP_TAG, MC68901, MASTER_CLOCK/4)
	MCFG_MC68901_TIMER_CLOCK(Y2 / 16)
	MCFG_MC68901_RX_CLOCK(9600)
	MCFG_MC68901_TX_CLOCK(9600)
	MCFG_MC68901_OUT_SO_CB(WRITELINE(RS232_TAG, rs232_port_device, write_txd))
	MCFG_MC68901_OUT_IRQ_CB(INPUTLINE(M68K_TAG, M68K_IRQ_6))
	
	MCFG_DEVICE_ADD(ISABUS_TAG, ISA16, MASTER_CLOCK/4)
	MCFG_ISA16_CPU(":" M68K_TAG)
	MCFG_ISA16_BUS_CUSTOM_SPACES()
	MCFG_ISA_OUT_IRQ2_CB(WRITELINE(*this, luigisbc_state, irq2_w))
	MCFG_ISA_OUT_IRQ4_CB(WRITELINE(*this, luigisbc_state, irq4_w))
	MCFG_DEVICE_ADD("isa1", ISA16_SLOT, 0, ISABUS_TAG, luigisbc_isa16_cards, "svga_et4k_16", false)
	MCFG_DEVICE_ADD("isa2", ISA16_SLOT, 0, ISABUS_TAG, luigisbc_isa16_cards, "com", false)

	MCFG_DEVICE_ADD(RS232_TAG, RS232_PORT, default_rs232_devices, "terminal")
	MCFG_RS232_RXD_HANDLER(WRITELINE(MFP_TAG, mc68901_device, write_rx))
	MCFG_RS232_DCD_HANDLER(WRITELINE(MFP_TAG, mc68901_device, i1_w))
	MCFG_RS232_CTS_HANDLER(WRITELINE(MFP_TAG, mc68901_device, i2_w))
	MCFG_RS232_RI_HANDLER(WRITELINE(MFP_TAG, mc68901_device, i6_w))
	
	MCFG_DEVICE_ADD("hd63450", HD63450, MASTER_CLOCK/2)
	MCFG_HD63450_CPU(M68K_TAG) // CPU - 68000
	MCFG_HD63450_CLOCKS(attotime::from_usec(2), attotime::from_nsec(450), attotime::from_usec(4), attotime::from_hz(15625/2))
	MCFG_HD63450_BURST_CLOCKS(attotime::from_usec(2), attotime::from_nsec(450), attotime::from_nsec(50), attotime::from_nsec(50))
	MCFG_HD63450_DMA_END_CB(WRITE8(*this, luigisbc_state, dma_end))
	MCFG_HD63450_DMA_ERROR_CB(WRITE8(*this, luigisbc_state, dma_error))
	MCFG_HD63450_DMA_READ_0_CB(READ16(*this, luigisbc_state, isa1_dma_r))
	MCFG_HD63450_DMA_WRITE_0_CB(WRITE16(*this, luigisbc_state, isa1_dma_w))
MACHINE_CONFIG_END

/*
MACHINE_CONFIG_START(luigisbc_state::luigi012)
	MCFG_DEVICE_ADD(M68K_TAG, M68012, MASTER_CLOCK/2)
	MCFG_DEVICE_PROGRAM_MAP(luigisbc_mem)
	MCFG_DEVICE_IRQ_ACKNOWLEDGE_DRIVER(luigisbc_state,luigisbc_int_ack)

	MCFG_DEVICE_ADD(MFP_TAG, MC68901, MASTER_CLOCK/4)
	MCFG_MC68901_TIMER_CLOCK(Y2)
	MCFG_MC68901_RX_CLOCK(9600)
	MCFG_MC68901_TX_CLOCK(9600)
	MCFG_MC68901_OUT_SO_CB(WRITELINE(RS232_TAG, rs232_port_device, write_txd))
	MCFG_MC68901_OUT_IRQ_CB(INPUTLINE(M68K_TAG, M68K_IRQ_6))
	
	MCFG_DEVICE_ADD(ISABUS_TAG, ISA16, MASTER_CLOCK/4)
	MCFG_ISA16_CPU(":" M68K_TAG)
	MCFG_ISA16_BUS_CUSTOM_SPACES()
	MCFG_ISA_OUT_IRQ2_CB(WRITELINE(*this, luigisbc_state, irq2_w))
	MCFG_ISA_OUT_IRQ4_CB(WRITELINE(*this, luigisbc_state, irq4_w))
	MCFG_DEVICE_ADD("isa1", ISA16_SLOT, 0, ISABUS_TAG, luigisbc_isa16_cards, "svga_et4k_16", false)
	MCFG_DEVICE_ADD("isa2", ISA16_SLOT, 0, ISABUS_TAG, luigisbc_isa16_cards, "com", false)

	MCFG_DEVICE_ADD(RS232_TAG, RS232_PORT, default_rs232_devices, "terminal")
	MCFG_RS232_RXD_HANDLER(WRITELINE(MFP_TAG, mc68901_device, write_rx))
	MCFG_RS232_DCD_HANDLER(WRITELINE(MFP_TAG, mc68901_device, i1_w))
	MCFG_RS232_CTS_HANDLER(WRITELINE(MFP_TAG, mc68901_device, i2_w))
	MCFG_RS232_RI_HANDLER(WRITELINE(MFP_TAG, mc68901_device, i6_w))
	
	MCFG_DEVICE_ADD("hd63450", HD63450, MASTER_CLOCK/2)
	MCFG_HD63450_CPU(M68K_TAG) // CPU - 68012
	MCFG_HD63450_CLOCKS(attotime::from_usec(2), attotime::from_nsec(450), attotime::from_usec(4), attotime::from_hz(15625/2))
	MCFG_HD63450_BURST_CLOCKS(attotime::from_usec(2), attotime::from_nsec(450), attotime::from_nsec(50), attotime::from_nsec(50))
	MCFG_HD63450_DMA_END_CB(WRITE8(*this, luigisbc_state, dma_end))
	MCFG_HD63450_DMA_ERROR_CB(WRITE8(*this, luigisbc_state, dma_error))
	MCFG_HD63450_DMA_READ_0_CB(READ16(*this, luigisbc_state, isa1_dma_r))
	MCFG_HD63450_DMA_WRITE_0_CB(WRITE16(*this, luigisbc_state, isa1_dma_w))
MACHINE_CONFIG_END
*/

IRQ_CALLBACK_MEMBER(luigisbc_state::luigisbc_int_ack)
{
	if (irqline == M68K_IRQ_6)
	{
		return m_mfp->get_vector();
	}

	return M68K_INT_ACK_AUTOVECTOR;
}

READ16_MEMBER(luigisbc_state::isa1_dma_r)
{   
	return m_isa->mem_r(space, offset, 0xFF);
}

WRITE16_MEMBER(luigisbc_state::isa1_dma_w)
{
	//TODO: this right? lol
	logerror("DMA: offset %x data %x\n", offset, data);
	m_isa->mem_w(space, offset, data, 0xFF);
}

void luigisbc_state::dma_irq(int channel)
{
	m_current_vector[3] = m_hd63450->get_vector(channel);
	m_current_irq_line = 3;
	logerror("DMA#%i: DMA End (vector 0x%02x)\n",channel,m_current_vector[3]);
	m_maincpu->set_input_line_and_vector(3,ASSERT_LINE,m_current_vector[3]);
}

WRITE8_MEMBER(luigisbc_state::dma_end)
{
	if(data != 0)
	{
		dma_irq(offset);
	}
	if(offset == 0)
	{
		//m_fdc_tc->adjust(attotime::from_usec(1), 0, attotime::never);
	}
}

WRITE8_MEMBER(luigisbc_state::dma_error)
{
	if(data != 0)
	{
		m_current_vector[3] = m_hd63450->get_error_vector(offset);
		m_current_irq_line = 3;
		logerror("DMA#%i: DMA Error (vector 0x%02x)\n",offset,m_current_vector[3]);
		m_maincpu->set_input_line_and_vector(3,ASSERT_LINE,m_current_vector[3]);
	}
}

/* ROM definition */
ROM_START( luigisbc )
	ROM_REGION(0x1000000, M68K_TAG, 0)
	ROM_LOAD( "68000.bin", 0x000000, 0x100000, CRC(00000000) SHA1(0) )
ROM_END

/* ROM definition */
/*
ROM_START( luigi012 )
	ROM_REGION(0x1000000, M68K_TAG, 0)
	ROM_LOAD( "68012.bin", 0x00000000, 0x100000, CRC(00000000) SHA1(0) )
ROM_END
*/

/* Driver */

/*    YEAR  NAME        PARENT  COMPAT MACHINE     INPUT     CLASS            INIT  COMPANY         FULLNAME                    FLAGS */
COMP( 2018, luigisbc,   0,       0,    luigisbc,   luigisbc, luigisbc_state,  empty_init,    "Luigi Thirty", "Procyon 68000", MACHINE_NO_SOUND_HW)

/*
COMP( 2018, luigi012,   0,       0,    luigi012,   luigisbc, luigi012_state,  empty_init,    "Luigi Thirty", "Procyon 68012", MACHINE_NO_SOUND_HW)
*/
