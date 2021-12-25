// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*****************************************************************************

    Signetics 8X371 8-Bit Latched Bidirectional I/O Port

*****************************************************************************/

#include "emu.h"
#include "8x371.h"

#define LOG_PRINTF  (1U << 1)
#define LOG_SETUP   (1U << 2)
#define LOG_GENERAL (1U << 3)

//#define VERBOSE (LOG_PRINTF | LOG_SETUP | LOG_GENERAL)

#include "logmacro.h"

#define LOGPRINTF(...)  LOGMASKED(LOG_PRINTF,   __VA_ARGS__)
#define LOGSETUP(...)   LOGMASKED(LOG_SETUP,    __VA_ARGS__)
#define LOGGENERAL(...) LOGMASKED(LOG_GENERAL,  __VA_ARGS__)

DEFINE_DEVICE_TYPE(N8X371, n8x371_device, "N8X371", "N8X371 I/O Port")

n8x371_device::n8x371_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, N8X371, tag, owner, clock)
	, m_write_cb(*this)
	, m_read_cb(*this)
	, m_value(0)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void n8x371_device::device_start()
{
	save_item(NAME(m_value));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void n8x371_device::device_reset()
{
	m_value = 0;
}

// UD bus
// void n8x371_device::ud_w(uint8_t bit, bool state)
// {
//     if(m_uic_state == 0)
//     {
//         m_ud_incoming[bit] = state;
//     }
// }

// WRITE_LINE_MEMBER( n8x371_device::ud0_w ) { ud_w(7, state); }
// WRITE_LINE_MEMBER( n8x371_device::ud1_w ) { ud_w(6, state); }
// WRITE_LINE_MEMBER( n8x371_device::ud2_w ) { ud_w(5, state); }
// WRITE_LINE_MEMBER( n8x371_device::ud3_w ) { ud_w(4, state); }
// WRITE_LINE_MEMBER( n8x371_device::ud4_w ) { ud_w(3, state); }
// WRITE_LINE_MEMBER( n8x371_device::ud5_w ) { ud_w(2, state); }
// WRITE_LINE_MEMBER( n8x371_device::ud6_w ) { ud_w(1, state); }
// WRITE_LINE_MEMBER( n8x371_device::ud7_w ) { ud_w(0, state); }

// IV bus, active low
// void n8x371_device::iv_w(uint8_t bit, bool state)
// {
//     if(m_wc_state)
//     {
//         m_iv_incoming[bit] = state;
//     }
// }

// WRITE_LINE_MEMBER( n8x371_device::iv0_w ) { iv_w(7, state); }
// WRITE_LINE_MEMBER( n8x371_device::iv1_w ) { iv_w(6, state); }
// WRITE_LINE_MEMBER( n8x371_device::iv2_w ) { iv_w(5, state); }
// WRITE_LINE_MEMBER( n8x371_device::iv3_w ) { iv_w(4, state); }
// WRITE_LINE_MEMBER( n8x371_device::iv4_w ) { iv_w(3, state); }
// WRITE_LINE_MEMBER( n8x371_device::iv5_w ) { iv_w(2, state); }
// WRITE_LINE_MEMBER( n8x371_device::iv6_w ) { iv_w(1, state); }
// WRITE_LINE_MEMBER( n8x371_device::iv7_w ) { iv_w(0, state); }

void n8x371_device::update()
{
    // IV bus -> m_value
    if(m_wc_state && !m_me_state)
    {
        LOG("8X371: updating m_value from IV bus: %02X\n", m_iv_incoming);
        m_value = 0x00;
        for(int i=0; i<8; i++)
        {
            m_value |= (m_iv_incoming & (1 << i));
            m_value_came_from_iv = true;
        }
    }
    else if(m_uic_state == 0 && !m_me_state)
    {
        LOG("8X371: updating m_value from UD bus\n");
        m_value = 0x00;
        for(int i=0; i<8; i++)
        {
            m_value |= (m_iv_incoming & (1 << i));
            m_value_came_from_iv = false;
        }
    }
}

/* Write lines */
void n8x371_device::write_iv(uint8_t val)
{
	m_iv_incoming = val;
}

void n8x371_device::write_ud(uint8_t val)
{
	m_ud_incoming = val;
}