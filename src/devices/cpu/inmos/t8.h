// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

    t8.h
    T8-series CPU implementation.

    The top-of-the-line transputer.
    - 32-bit word
    - FPU
    - Some additional instructions

***************************************************************************/

#ifndef MAME_CPU_INMOS_T800_H
#define MAME_CPU_INMOS_T800_H

#pragma once

#include "transputer.h"
#include "machine/ram.h"

DECLARE_DEVICE_TYPE(T800, t800_cpu_device);

class t800_cpu_device : 
    public transputer_cpu_device
{
    public:
    t800_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

    virtual offs_t get_memstart() override { return 0x80000070; }

    protected:
    // virtual uint8_t sram_read8(offs_t addr) override;
    // virtual void sram_write8(offs_t addr, uint8_t data) override;

    private:
    // subdevices
};

#endif
