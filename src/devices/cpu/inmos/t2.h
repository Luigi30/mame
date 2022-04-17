// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

    t2.h
    T2-series CPU implementation.

    The embedded transputer.
    - 16-bit word
    - no FPU

***************************************************************************/

#ifndef MAME_CPU_INMOS_T4_H
#define MAME_CPU_INMOS_T4_H

#pragma once

#include "transputer.h"
#include "machine/ram.h"

DECLARE_DEVICE_TYPE(T212, t212_cpu_device);

class t212_cpu_device : 
    public transputer_cpu_device
{
    public:
    t212_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

    protected:
    // virtual uint8_t sram_read8(offs_t addr) override;
    // virtual void sram_write8(offs_t addr, uint8_t data) override;
    
    private:
    // subdevices
};

#endif
