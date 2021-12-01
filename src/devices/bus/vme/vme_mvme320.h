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

DECLARE_DEVICE_TYPE(VME_MVME120,   vme_mvme120_card_device)
DECLARE_DEVICE_TYPE(VME_MVME121,   vme_mvme121_card_device)
DECLARE_DEVICE_TYPE(VME_MVME122,   vme_mvme122_card_device)
DECLARE_DEVICE_TYPE(VME_MVME123,   vme_mvme123_card_device)

//**************************************************************************
//  Base Device declaration
//**************************************************************************
class vme_mvme320_device :  public device_t, public device_vme_card_interface
{
	vme_mvme320_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

    required_device<cpu_device> m_maincpu;
};

#endif // MAME_BUS_VME_VME_MVME320_H