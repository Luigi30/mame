// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    Coleco Adam Multipurpose Interface Board 238
    https://github.com/epearsoe/MIB238

**********************************************************************/

#ifndef MAME_BUS_ADAM_MIB238_H
#define MAME_BUS_ADAM_MIB238_H

#pragma once

#include "exp.h"
#include "bus/centronics/ctronics.h"
#include "bus/rs232/rs232.h"
#include "machine/mc68681.h"
#include "machine/output_latch.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> mib238_device

class mib238_device :  public device_t,
							public device_adam_expansion_slot_card_interface
{
public:
	// construction/destruction
	mib238_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// device-level overrides
	virtual void device_start() override;
    virtual void device_add_mconfig(machine_config &config) override;

    virtual const tiny_rom_entry *device_rom_region() const override;

	// device_adam_expansion_slot_card_interface overrides
	virtual uint8_t adam_bd_r(offs_t offset, uint8_t data, int bmreq, int biorq, int aux_rom_cs, int cas1, int cas2) override;
	virtual void adam_bd_w(offs_t offset, uint8_t data, int bmreq, int biorq, int aux_rom_cs, int cas1, int cas2) override;

    void duart_out_w(uint8_t value);

private:
    required_device<scn2681_device>         m_duart;
    required_device<rs232_port_device>      m_serial1;
    required_device<rs232_port_device>      m_serial2;
    required_device<centronics_device>      m_lpt;
    required_device<output_latch_device>    m_lpt_data_out;
};


// device type definition
DECLARE_DEVICE_TYPE(MIB238, mib238_device)

#endif // MAME_BUS_ADAM_MIB238_H
