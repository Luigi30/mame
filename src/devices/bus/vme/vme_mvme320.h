// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
#ifndef MAME_BUS_VME_VME_MVME320_H
#define MAME_BUS_VME_VME_MVME320_H

#pragma once

#include "cpu/8x300/8x300.h"
#include "bus/vme/vme.h"
#include "bus/rs232/rs232.h"
#include "machine/clock.h"
#include "machine/mc68901.h"

DECLARE_DEVICE_TYPE(VME_MVME320, vme_mvme320_card_device)

//**************************************************************************
//  Base Device declaration
//**************************************************************************
class vme_mvme320_device :  public device_t, public device_vme_card_interface
{
public:
	vme_mvme320_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

protected:
    required_device<cpu_device> m_maincpu;

	virtual void device_start() override;
	virtual void device_reset() override;

    virtual const tiny_rom_entry *device_rom_region() const override;

    void device_add_mconfig(machine_config &config) override;
    void mvme320_program_map(address_map &map);
    void mvme320_io_map(address_map &map);

    void unscramble_proms();
};

//**************************************************************************
//  Board Device declarations
//**************************************************************************

class vme_mvme320_card_device : public vme_mvme320_device
{
public:
	vme_mvme320_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	vme_mvme320_card_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme320_device(mconfig, type, tag, owner, clock)
	{ }
};

#endif // MAME_BUS_VME_VME_MVME320_H