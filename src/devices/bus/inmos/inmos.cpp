// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

#include "emu.h"
#include "inmos.h"

DEFINE_DEVICE_TYPE(INMOS_SLOT, inmos_slot_device, "inmos_slot", "INMOS TRAM Slot")

#define VERBOSE	1

#include "logmacro.h"

/***********************************************************************
	SLOT DEVICE
***********************************************************************/
inmos_slot_device::inmos_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: inmos_slot_device(mconfig, INMOS_SLOT, tag, owner, clock)
{
}

inmos_slot_device::inmos_slot_device(machine_config const &mconfig, device_type type, char const *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, type, tag, owner, clock)
	, device_single_card_slot_interface<device_inmos_tram_interface>(mconfig, *this)
{
}

/*----------------------------------
device_t implementation
----------------------------------*/

/***********************************************************************
	CARD INTERFACE
***********************************************************************/

device_inmos_tram_interface::device_inmos_tram_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device, "inmos")
	, m_link_out_cb(*this)
	, m_error_cb(*this)
	, m_index(~unsigned(0))
{
}

void device_inmos_tram_interface::interface_pre_start()
{
}

/***********************************************************************
	BUS DEVICE
***********************************************************************/

#include "imsb411.h"

// must come after including the headers that declare these extern
template class device_finder<device_inmos_tram_interface, false>;
template class device_finder<device_inmos_tram_interface, true>;

void inmos_cards(device_slot_interface &device)
{
	device.option_add("ims_b411", IMS_B411);
}

/**********************************/
