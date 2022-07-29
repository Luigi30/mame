
// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    Apollo DN300-series display controller.

    1024x1024 physical framebuffer
    1 bit per pixel
    1024x800 visible area

    Contains a copy/fill blitter.

 **********************************************************************/

#ifndef MAME_VIDEO_DN300_H
#define MAME_VIDEO_DN300_H

#include "emupal.h"
#include "screen.h"
#include "machine/ram.h"

class dn300_display_device : public device_t, public device_video_interface
{
public:
	// construction/destruction
	dn300_display_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

    virtual uint32_t screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

    uint8_t     vram_r(offs_t offset);
    void        vram_w(offs_t offset, uint8_t data);

    uint16_t    regs_r(offs_t offset);
    void        regs_w(offs_t offset, uint16_t data);

    typedef enum
    {
        DISPLAY_CONTROL,
        DEB,
        WSSY,
        WSSX,
        DCY,
        DCX,
        WSDY,
        WSDX,
        BLT_DCY = (0x14 >> 1),
        BLT_DCX = (0x16 >> 1),
        BLT_END_BIT = (0x1C >> 1)
    } Regs;

protected:
    virtual void device_add_mconfig(machine_config &config) override;
    virtual void device_start() override;
    virtual void device_reset() override;

    required_device<ram_device>     m_vram;
    required_device<screen_device>  m_screen;
    required_device<palette_device> m_palette;

private:
    void        update_display_control(uint16_t data);
    void        do_copy_blit(uint16_t wssx, uint16_t wssy, uint16_t wsdx, uint16_t wsdy, uint16_t dcx, uint16_t dcy);
    void        do_fill_blit(uint16_t wssx, uint16_t wssy, uint16_t wsdx, uint16_t wsdy, uint16_t dcx, uint16_t dcy);

    uint16_t    m_display_control;
    uint16_t    m_display_status;
    uint16_t    m_deb;  // destination end bit
    uint16_t    m_wssy; // source Y
    uint16_t    m_wssx; // source X
    int16_t     m_dcy;  // count Y
    int16_t     m_dcx;  // count X
    uint16_t    m_wsdy; // dest Y
    uint16_t    m_wsdx; // dest X

    TIMER_CALLBACK_MEMBER(blitter_operation_timer_callback);
    emu_timer *m_blitter_operation_timer = nullptr;

    bool        get_vram_pixel(uint16_t x, uint16_t y);
    void        set_vram_pixel(uint16_t x, uint16_t y, bool value);
};

DECLARE_DEVICE_TYPE(DN300_DISPLAY, dn300_display_device)

#endif