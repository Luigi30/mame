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
#include "nabupc.h"

#define VERBOSE 1
#include "logmacro.h"

void nabu_pc_centronics_devices(device_slot_interface &device)
{
	device.option_add("pl80", COMX_PL80);
	device.option_add("ex800", EPSON_EX800);
	device.option_add("lx800", EPSON_LX800);
	device.option_add("lx810l", EPSON_LX810L);
	device.option_add("ap2000", EPSON_AP2000);
	device.option_add("printer", CENTRONICS_PRINTER);
}

void nabu_pc_keyboard_devices(device_slot_interface &device)
{
    device.option_add("nabu_kbd", NABU_PC_KEYBOARD);
}

/**************************************
*             Memory Map              *
**************************************/

// TODO: /ROMSEL line

void nabu_pc_state::nabu_pc_map_4k_bios(address_map &map)
{
    map(0x0000, 0x0fff).rom();
    map(0x1000, 0xffff).ram();
}

void nabu_pc_state::nabu_pc_map_8k_bios(address_map &map)
{
    map(0x0000, 0x1fff).rom();
    map(0x2000, 0xffff).ram();
}

void nabu_pc_state::nabu_pc_io_map(address_map &map)
{
	map.global_mask(0xff);
	map.unmap_value_high();

    // I/O map
    //  $00      W  - Hardware Control Register
    //  $40     RW  - PSG data port
    //  $41      W  - PSG address strobe
    //  $80     RW  - Adaptor data port
    //  $90     R   - Keyboard UART data port 
    //  $91     RW  - Keyboard UART control port
    //  $A0     RW  - VDP
    //  $A1     R   - VDP
    //  $B0      W  - Parallel data out
    //  $C0-$CF RW  - slot 0
    //  $D0-$DF RW  - slot 1
    //  $E0-$EF RW  - slot 2
    //  $F0-$FF RW  - slot 3 

    map(0x00, 0x00).w(FUNC(nabu_pc_state::hardware_control_w)).mirror(0x3f);

	map(0x40, 0x40).rw(m_psg, FUNC(ay8910_device::data_r), FUNC(ay8910_device::data_w));
    map(0x41, 0x41).w(m_psg, FUNC(ay8910_device::address_w));

    map(0x80, 0x80).rw(m_adaptor, FUNC(ay31015_device::receive), FUNC(ay31015_device::transmit));

    map(0x90, 0x90).rw(m_usart, FUNC(i8251_device::data_r), FUNC(i8251_device::data_w));
	map(0x91, 0x91).rw(m_usart, FUNC(i8251_device::status_r), FUNC(i8251_device::control_w));

    map(0xa0, 0xa1).rw(m_vdp, FUNC(tms9918a_device::read), FUNC(tms9918a_device::write));

    map(0xb0, 0xb0).w(FUNC(nabu_pc_state::parallel_w));
}

void nabu_pc_state::parallel_w(uint8_t data)
{
    m_parallel->write_data0(BIT(data,0));
    m_parallel->write_data1(BIT(data,1));
    m_parallel->write_data2(BIT(data,2));
    m_parallel->write_data3(BIT(data,3));
    m_parallel->write_data4(BIT(data,4));
    m_parallel->write_data5(BIT(data,5));
    m_parallel->write_data6(BIT(data,6));
    m_parallel->write_data7(BIT(data,7));
}

WRITE_LINE_MEMBER( nabu_pc_state::parallel_busy_changed )
{
	m_parallel_busy = state;
}

void nabu_pc_state::interrupt_control_w(uint8_t value)
{
    // Interrupt Control is a write-only register.
    // Masks the bits of the IRQ latch.

    // b0: Slot 4 interrupt
    // b1: Slot 3 interrupt
    // b2: Slot 2 interrupt
    // b3: Slot 1 interrupt
    // b4: Clock interrupt
    // b5: Keyboard interrupt
    // b6: Adaptor Tx interrupt
    // b7: Adaptor Rx interrupt

    m_int_mask = value;
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

    m_parallel->write_strobe(BIT(value, 2));
}

// uint8_t nabu_pc_state::porta_r()
// {
//     //LOG("PSG port A read\n");
//     return m_interrupt_control;
// }

uint8_t nabu_pc_state::portb_r()
{
    //LOG("PSG port B read\n");

    // Interrupt status byte.
    // d0 = interrupt request
    // d1-d3 are the IRQ number
    // d4 - printer busy
    // d5 and d6 are HCCA UART framing and overrun errors

    uint8_t value = 0;

    value |= (m_int_active > 0) ? 1 : 0;
    value |= (m_parallel_busy   ? 0x10 : 0x00);

    if(m_int_active)
    {
        uint8_t highest_priority = 7;

        // Propagate the highest-priority interrupt.
        switch(m_int_active)
        {
            case 0x80:
                highest_priority = 7;
                break;
            case 0x40:
                highest_priority = 6;
                break;
            case 0x20:
                highest_priority = 5;
                break;
            case 0x10:
                highest_priority = 4;
                break;
            case 0x08:
                highest_priority = 3;
                break;
            case 0x04:
                highest_priority = 2;
                break;
            case 0x02:
                highest_priority = 1;
                break;
            case 0x01:
                highest_priority = 0;
                break;
        }

        // Set the interrupt value in PORTB starting at b1.
        value |= (0 << 1);

        // The highest-priority interrupt is acknowledged.
        irq_encoder(highest_priority, false);        
        LOG("Acknowledged IRQ%d\n", highest_priority);
    }

    return value;
}

void nabu_pc_state::porta_w(uint8_t value)
{
    if(value != m_interrupt_control) LOG("PSG port A: interrupt mask now %02X\n", value);
    m_interrupt_control = value;

}

// void nabu_pc_state::portb_w(uint8_t value)
// {
//     //LOG("PSG port B write %02X\n", value);
// }

void nabu_pc_state::irq_encoder(int pin, int state)
{
    // Only enable interrupt if it's not masked.

	if (state == ASSERT_LINE)
    {
        if(BIT(m_interrupt_control, pin))
        {
		    m_int_active |= (1 << pin);
            LOG("asserting interrupt %d\n", pin);
        }
        else
        {
            m_int_active &= ~(1 << pin);
            LOG("interrupt %d is masked\n", pin);
        }

    }
	else
    {
		m_int_active &= ~(1 << pin);
        LOG("clearing interrupt %d\n", pin);
    }

	m_maincpu->set_input_line(INPUT_LINE_IRQ0, (m_int_active != 0));
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
    // keyboard transmits 0x95 on power-up to signal self-tests passed

    m_parallel_busy = false;

    m_int_mask = 0;
    m_int_active = 0;
}

/**************************************
*           Machine Config            *
**************************************/

void nabu_pc_state::nabu_pc(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, CPU_CLOCK);      // 3.579545 MHz
	m_maincpu->set_addrmap(AS_PROGRAM, &nabu_pc_state::nabu_pc_map_4k_bios);
    m_maincpu->set_addrmap(AS_IO, &nabu_pc_state::nabu_pc_io_map);

	/* video hardware */
	TMS9918A(config, m_vdp, VDP_CLOCK);       // X1 connected directly to VDP
	m_vdp->set_screen("screen");
	m_vdp->set_vram_size(0x4000);
	m_vdp->int_callback().set(FUNC(nabu_pc_state::irq<4>));
	SCREEN(config, "screen", SCREEN_TYPE_RASTER);

    // 8251 Tx lines are not connected
    I8251(config, m_usart, KBD_CLOCK);    // 1.79MHz    
    m_usart->rxrdy_handler().set(FUNC(nabu_pc_state::irq<5>));

    // Cable modem "adaptor" port
	AY31015(config, m_adaptor);

    SPEAKER(config, "mono").front_center();
    AY8910(config, m_psg, 10.738635_MHz_XTAL / 10).add_route(ALL_OUTPUTS, "mono", 1.0);
    //m_psg->port_a_read_callback().set(FUNC(nabu_pc_state::porta_r));
	m_psg->port_b_read_callback().set(FUNC(nabu_pc_state::portb_r));
	m_psg->port_a_write_callback().set(FUNC(nabu_pc_state::porta_w));
    //m_psg->port_b_write_callback().set(FUNC(nabu_pc_state::portb_w));

	CENTRONICS(config, m_parallel, nabu_pc_centronics_devices, "printer");
	m_parallel->busy_handler().set(FUNC(nabu_pc_state::parallel_busy_changed));

    CLOCK(config, m_hcca_clock, HCCA_CLOCK);
    m_hcca_clock->signal_handler().set(m_adaptor, FUNC(ay31015_device::write_tcp));
    m_hcca_clock->signal_handler().append(m_adaptor, FUNC(ay31015_device::write_rcp));

    CLOCK(config, m_kbd_clock, KBD_CLOCK);
    m_kbd_clock->signal_handler().set(m_usart, FUNC(i8251_device::write_rxc));
    // TxC is not connected

    rs232_port_device &rs232(RS232_PORT(config, "rs232", nabu_pc_keyboard_devices, "nabu_kbd"));
	rs232.rxd_handler().set(m_usart, FUNC(i8251_device::write_rxd));
	rs232.dsr_handler().set(m_usart, FUNC(i8251_device::write_dsr));
}

/**************************************
*              ROM Load               *
**************************************/
ROM_START( nabu_pc )
	ROM_REGION( 0x2000, "maincpu", 0 )  // main BIOS
	ROM_DEFAULT_BIOS("rev_b")
	//ROM_SYSTEM_BIOS(0, "rev_a", "BIOS Rev.A")
	//ROMX_LOAD("pc_rev_a.bin", 0x0000, 0x1000, CRC(8110bde0) SHA1(57e5f34645df06d7cb6c202a6d35a442776af2cb), ROM_BIOS(0))
	ROM_SYSTEM_BIOS(0, "rev_b", "BIOS Rev.B")
	ROMX_LOAD("pc_rev_b.bin", 0x0000, 0x2000, CRC(5a5db110) SHA1(14f3e14ed809f9ec30b8189e5506ed911127de34), ROM_BIOS(0))
ROM_END

/**************************************
*           Game Driver(s)            *
**************************************/

/*    YEAR  NAME      PARENT  MACHINE  INPUT    STATE          INIT        ROT   COMPANY            FULLNAME    FLAGS  */
GAME( 1982, nabu_pc,  0,      nabu_pc, nabu_pc, nabu_pc_state, empty_init, ROT0, "NABU",            "NABU PC",  MACHINE_IS_SKELETON )
