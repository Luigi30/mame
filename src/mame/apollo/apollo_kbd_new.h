// license:BSD-3-Clause
// copyright-holders:Barry Rodewald
// Convergent NGEN keyboard

#ifndef MAME_MACHINE_APOLLO_KB_NEW_H
#define MAME_MACHINE_APOLLO_KB_NEW_H

#pragma once

#include "bus/rs232/keyboard.h"

class apollo_keyboard_new_device : public serial_keyboard_device
{
public:
	apollo_keyboard_new_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
	virtual ioport_constructor device_input_ports() const override;

protected:
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void rcv_complete() override;
	virtual void key_make(uint8_t row, uint8_t column) override;
	virtual void key_break(uint8_t row, uint8_t column) override;

private:
	void write(uint8_t data);

	uint8_t m_keys_down;
	uint8_t m_last_reset;

    struct code_entry { uint16_t down, up, unshifted, shifted, control, caps_lock, up_trans, auto_repeat; };
	static code_entry const s_code_table[];
};

DECLARE_DEVICE_TYPE(APOLLO_KEYBOARD_NEW, apollo_keyboard_new_device)

#endif // MAME_MACHINE_NGEN_KB_H
