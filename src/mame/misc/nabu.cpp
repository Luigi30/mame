// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*****************************************************************

    NABU PC
    Preliminary driver by Katherine Rohl

******************************************************************

  Hardware Notes
  --------------

  System Hardware:

  Z80 @ 3.75MHz
  TMS9918A VDP
  AY-3-8910
  8251 USART
  TR1865 UART

******************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/ay31015.h"
#include "machine/i8251.h"
#include "sound/ay8910.h"
#include "video/tms9928a.h"
#include "screen.h"
#include "speaker.h"

#define VERBOSE 1
#include "logmacro.h"

class nabu_pc_state : public driver_device
{
public:
	nabu_pc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
        m_adaptor(*this, "adaptor"),
        m_usart(*this, "usart"),
        m_vdp(*this, "vdp"),
        m_psg(*this, "psg")
	{ }

	void nabu_pc(machine_config &config);

    DECLARE_WRITE_LINE_MEMBER(nabu_vdp_interrupt);

protected:
	int m_last_nmi_state = 0;

private:
	required_device<cpu_device> m_maincpu;
    required_device<ay31015_device> m_adaptor;  // ADAPTOR port, connected to a UART
    required_device<i8251_device> m_usart;      // USART for the parallel port
    required_device<tms9918a_device> m_vdp;
    required_device<ay8910_device> m_psg;

	virtual void machine_start() override;
	virtual void machine_reset() override;
	virtual void video_start() override;
	void nabu_pc_map(address_map &map);
    void nabu_pc_io_map(address_map &map);
    
    void hardware_control_w(uint8_t value);
    void interrupt_control_w(uint8_t value);

    uint8_t m_hardware_control;
    uint8_t m_interrupt_control;

    uint8_t porta_r();
    uint8_t portb_r();
    void porta_w(uint8_t);
    void portb_w(uint8_t);
};

void nabu_pc_state::video_start()
{
}

/**************************************
*             Memory Map              *
**************************************/

void nabu_pc_state::nabu_pc_map(address_map &map)
{
    map(0x0000, 0x0fff).rom();
    map(0x1000, 0xffff).ram();
}

void nabu_pc_state::nabu_pc_io_map(address_map &map)
{
	map.global_mask(0xff);
	map.unmap_value_high();

    // I/O map
    //  $00      W  - Hardware Control Register
    //  $40     RW  - PSG
    //  $41      W  - PSG
    //  $80     RW
    //  $90     R   - keyboard buffer?
    //  $91     RW
    //  $A0     RW  - VDP
    //  $A1     R   - VDP

    // Somewhere there's an interrupt control register (page 3-26)
    
    // The keyboard test writes $20 to PORTA and reads I/O 90h.
    // The adaptor test writes $80 to PORTA and reads I/O 80h.

    //map(0x40, 0x40).rw(m_usart, FUNC(i8251_device::data_r), FUNC(i8251_device::data_w));      
	//map(0x41, 0x41).rw(m_usart, FUNC(i8251_device::status_r), FUNC(i8251_device::control_w)); 

    // TODO: work out the I/O map
    map(0x00, 0x00).w(FUNC(nabu_pc_state::hardware_control_w));                                 // OK

	map(0x40, 0x40).rw(m_psg, FUNC(ay8910_device::data_r), FUNC(ay8910_device::data_w));        // OK
    map(0x41, 0x41).w(m_psg, FUNC(ay8910_device::address_w));                                   // OK

    map(0x80, 0x80).rw(m_adaptor, FUNC(ay31015_device::receive), FUNC(ay31015_device::transmit));

    map(0xa0, 0xa1).rw(m_vdp, FUNC(tms9918a_device::read), FUNC(tms9918a_device::write));       // OK
}

void nabu_pc_state::interrupt_control_w(uint8_t value)
{
    // Interrupt Control is a write-only register.
    // b0: Slot 4 interrupt
    // b1: Slot 3 interrupt
    // b2: Slot 2 interrupt
    // b3: Slot 1 interrupt
    // b4: Clock interrupt
    // b5: Keyboard interrupt
    // b6: Adaptor Tx interrupt
    // b7: Adaptor Rx interrupt

    LOG("write to interrupt control %02X\n", value);

    m_interrupt_control = value;
}

void nabu_pc_state::hardware_control_w(uint8_t value)
{
    // Hardware Control is a write-only register.
    // b0: /ROM select
    // b1: video switch (1 = VDP, 0 = pass-through cable)
    // b2: parallel data strobe
    // b3: green LED
    // b4: red LED
    // b5: amber LED
    // b6: unused
    // b7: unused

    LOG("*** Hardware Control: /ROMSEL %d LEDs %c%c%c\n",
        BIT(value, 0),
        BIT(value, 3) ? 'G' : '-',
        BIT(value, 4) ? 'R' : '-',
        BIT(value, 5) ? 'Y' : '-');

    m_hardware_control = value;
}

uint8_t nabu_pc_state::porta_r()
{
    LOG("PSG port A read\n");
    return 0xFF;
}

uint8_t nabu_pc_state::portb_r()
{
    LOG("PSG port B read\n");
    return 0xFF;
}

void nabu_pc_state::porta_w(uint8_t value)
{
    LOG("PSG port A write %02X\n", value);
}

void nabu_pc_state::portb_w(uint8_t value)
{
    LOG("PSG port B write %02X\n", value);
}

/**************************************
*            Input Ports              *
**************************************/

static INPUT_PORTS_START( nabu_pc )
INPUT_PORTS_END


/**************************************
*        Machine Start/Reset          *
**************************************/

void nabu_pc_state::machine_start()
{
}

void nabu_pc_state::machine_reset()
{
}


/**************************************
*           Machine Config            *
**************************************/

void nabu_pc_state::nabu_pc(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, 10.738635_MHz_XTAL / 3); // 3.579545 MHz
	m_maincpu->set_addrmap(AS_PROGRAM, &nabu_pc_state::nabu_pc_map);
    m_maincpu->set_addrmap(AS_IO, &nabu_pc_state::nabu_pc_io_map);

	/* video hardware */
	TMS9918A(config, m_vdp, 10.738635_MHz_XTAL);
	m_vdp->set_screen("screen");
	m_vdp->set_vram_size(0x4000);
	m_vdp->int_callback().set(FUNC(nabu_pc_state::nabu_vdp_interrupt));
	SCREEN(config, "screen", SCREEN_TYPE_RASTER);

    I8251(config, m_usart, 0);

	AY31015(config, m_adaptor);

    SPEAKER(config, "mono").front_center();
    AY8910(config, m_psg, 10.738635_MHz_XTAL / 10).add_route(ALL_OUTPUTS, "mono", 1.0);
    m_psg->port_a_read_callback().set(FUNC(nabu_pc_state::porta_r));
	m_psg->port_b_read_callback().set(FUNC(nabu_pc_state::portb_r));
	m_psg->port_a_write_callback().set(FUNC(nabu_pc_state::porta_w));
    m_psg->port_b_write_callback().set(FUNC(nabu_pc_state::portb_w));
}

WRITE_LINE_MEMBER(nabu_pc_state::nabu_vdp_interrupt)
{
	// // NMI on rising edge
	// if (state && !m_last_nmi_state)
	// 	m_maincpu->pulse_input_line(INPUT_LINE_NMI, attotime::zero);

	// m_last_nmi_state = state;
}



/**************************************
*              ROM Load               *
**************************************/
ROM_START( nabu_pc )

//  ODYSSEY_BIOS

	ROM_REGION( 0x80000, "maincpu", 0 )  // main BIOS
	ROM_LOAD( "rev_a.bin", 0x0000, 0x1000, CRC(00000000) SHA1(29281d25aaf2051e0794dece8be146bb63d5c488) )

ROM_END


/**************************************
*           Game Driver(s)            *
**************************************/

/*    YEAR  NAME      PARENT  MACHINE  INPUT    STATE          INIT        ROT   COMPANY            FULLNAME    FLAGS  */
GAME( 1982, nabu_pc,  0,      nabu_pc, nabu_pc, nabu_pc_state, empty_init, ROT0, "NABU",            "NABU PC",  MACHINE_IS_SKELETON )
