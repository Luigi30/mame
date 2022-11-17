// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

#include "emu.h"
#include "nabu_kbd.h"

#define VERBOSE 1
#include "logmacro.h"

namespace {

ROM_START(nabu_kbd)
	ROM_REGION(0x800, "mbcpu", 0)
	ROM_LOAD("keyboard_rev_a.bin", 0x0000, 0x0800, CRC(eead3abc) SHA1(2f6ff63ca2f2ac90f3e03ef4f2b79883205e8a4e))
ROM_END

class nabu_pc_keyboard_device : public device_t, public device_rs232_port_interface
{
public:
	nabu_pc_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	virtual DECLARE_WRITE_LINE_MEMBER( input_txd ) override;

	required_device<m6803_cpu_device> m_cpu;

protected:
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_add_mconfig(machine_config &config) override;
	virtual const tiny_rom_entry *device_rom_region() const override;

    TIMER_CALLBACK_MEMBER(watchdog_expired);

private:
    emu_timer *m_watchdog_timer;
	void ser_tx_w(int state) { output_rxd(state); }
    void watchdog_reset_w(u8 state) { reset_timer(); }

	void m6803_mem(address_map &map);
	int m_rx_state;

    void reset_timer();

	uint8_t joyport1_r();
	uint8_t joyport2_r();
	uint8_t joyport3_r();
	uint8_t joyport4_r();
};

nabu_pc_keyboard_device::nabu_pc_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, NABU_PC_KEYBOARD, tag, owner, clock)
	, device_rs232_port_interface(mconfig, *this)
	, m_cpu(*this, "mbdcpu")
{
}

void nabu_pc_keyboard_device::device_add_mconfig(machine_config &config)
{
	M6803(config, m_cpu, XTAL(3'579'545));
	m_cpu->set_addrmap(AS_PROGRAM, &nabu_pc_keyboard_device::m6803_mem);
	m_cpu->out_ser_tx_cb().set(FUNC(nabu_pc_keyboard_device::ser_tx_w));
}

const tiny_rom_entry *nabu_pc_keyboard_device::device_rom_region() const
{
	return ROM_NAME(nabu_kbd);
}

void nabu_pc_keyboard_device::device_start()
{
  	m_watchdog_timer = timer_alloc(FUNC(nabu_pc_keyboard_device::watchdog_expired), this);
}

TIMER_CALLBACK_MEMBER(nabu_pc_keyboard_device::watchdog_expired)
{
    LOG("keyboard watchdog reset\n");
	m_cpu->pulse_input_line(INPUT_LINE_NMI, attotime::zero);
}

void nabu_pc_keyboard_device::device_reset()
{
	output_dcd(0);
	output_dsr(0);
	output_cts(0);

    reset_timer();
}

void nabu_pc_keyboard_device::reset_timer()
{
	m_watchdog_timer->adjust( attotime::from_seconds( 5 ) );
}

uint8_t nabu_pc_keyboard_device::joyport1_r()
{
	return 0;
}
uint8_t nabu_pc_keyboard_device::joyport2_r()
{
	return 0;
}
uint8_t nabu_pc_keyboard_device::joyport3_r()
{
	return 0;
}
uint8_t nabu_pc_keyboard_device::joyport4_r()
{
	return 0;
}

void nabu_pc_keyboard_device::m6803_mem(address_map &map)
{
	// TODO: this is definitely wrong

    // Somewhere there's a 555 watchdog timer.
    // The MCU writes to $7000, $8000, $9000. It's one of those, presumably.

	// $50/$51/$52/$5300 - 4 joystick ports? 2 are populated, 2 are unpopulated.
	map(0x5000, 0x5000).r(FUNC(nabu_pc_keyboard_device::joyport1_r));
	map(0x5100, 0x5100).r(FUNC(nabu_pc_keyboard_device::joyport2_r));
	map(0x5200, 0x5200).r(FUNC(nabu_pc_keyboard_device::joyport3_r));
	map(0x5300, 0x5300).r(FUNC(nabu_pc_keyboard_device::joyport4_r));

    map(0x7000, 0x7000).w(FUNC(nabu_pc_keyboard_device::watchdog_reset_w));
	map(0xf800, 0xffff).rom().region("mbcpu", 0);
}

WRITE_LINE_MEMBER(nabu_pc_keyboard_device::input_txd)
{
	m_rx_state = (state & 1);
}

} // anonymous namespace

DEFINE_DEVICE_TYPE_PRIVATE(NABU_PC_KEYBOARD, device_rs232_port_interface, nabu_pc_keyboard_device, "nabu_kbd", "NABU PC Keyboard")
