// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

#ifndef MAME_MACHINE_NABU_NABU_H
#define MAME_MACHINE_NABU_NABU_H

#pragma once

#include "bus/rs232/rs232.h"
#include "bus/rs232/keyboard.h"
#include "cpu/z80/z80.h"
#include "machine/ay31015.h"
#include "machine/clock.h"
#include "machine/i8251.h"
#include "machine/latch8.h"
#include "sound/ay8910.h"
#include "video/tms9928a.h"
#include "screen.h"
#include "speaker.h"
#include "nabu_kbd.h"

#include "bus/centronics/ctronics.h"
#include "bus/centronics/comxpl80.h"
#include "bus/centronics/epson_ex800.h"
#include "bus/centronics/epson_lx800.h"
#include "bus/centronics/epson_lx810l.h"
#include "bus/centronics/printer.h"

class nabu_pc_state : public driver_device
{
	static constexpr XTAL VDP_CLOCK     = 10.738635_MHz_XTAL;
	static constexpr XTAL CPU_CLOCK     = VDP_CLOCK / 3;
	static constexpr XTAL HCCA_CLOCK    = CPU_CLOCK / 2;
    static constexpr XTAL UART_CLOCK    = CPU_CLOCK / 2;
    static constexpr XTAL KBD_CLOCK     = UART_CLOCK / 16;


public:
	nabu_pc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
        , m_adaptor(*this, "adaptor")
        , m_usart(*this, "usart")
        , m_vdp(*this, "vdp")
        , m_psg(*this, "psg")
        , m_parallel(*this, "parallel")
        , m_hcca_clock(*this, "hcca_clock")
        , m_kbd_clock(*this, "kbd_clock")
	{ }

	void nabu_pc(machine_config &config);

    DECLARE_WRITE_LINE_MEMBER(nabu_vdp_interrupt);

    template <unsigned N> DECLARE_WRITE_LINE_MEMBER(irq) { irq_encoder(N, state); }

protected:
	int m_last_nmi_state = 0;

private:
	required_device<cpu_device> m_maincpu;
    required_device<ay31015_device> m_adaptor;  // ADAPTOR port, connected to a UART
    required_device<i8251_device> m_usart;      // USART for the parallel port
    required_device<tms9918a_device> m_vdp;
    required_device<ay8910_device> m_psg;
    required_device<centronics_device> m_parallel;
    required_device<clock_device> m_hcca_clock;
    required_device<clock_device> m_kbd_clock;

	virtual void machine_start() override;
	virtual void machine_reset() override;

	void nabu_pc_map_4k_bios(address_map &map);
	void nabu_pc_map_8k_bios(address_map &map);

    void nabu_pc_io_map(address_map &map);
    
    void hardware_control_w(uint8_t value);
    void interrupt_control_w(uint8_t value);

    void parallel_w(uint8_t value);

    uint8_t m_hardware_control;
    uint8_t m_interrupt_control;

    uint8_t porta_r();
    uint8_t portb_r();
    void porta_w(uint8_t);
    void portb_w(uint8_t);

    // uint8_t m_interrupt_state;

    DECLARE_WRITE_LINE_MEMBER(parallel_busy_changed);
    bool m_parallel_busy;

    // uint8_t m_irq_priority;
    // DECLARE_WRITE_LINE_MEMBER(irq_ack_w);

    // void update_irq_priority();

	u8 m_int_mask = 0U, m_int_active = 0U;

    void irq_encoder(int pin, int state);

    void kbd_put(u8 data);
};

#endif
