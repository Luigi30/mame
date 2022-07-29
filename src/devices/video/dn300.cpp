// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    Apollo DN300-series display controller.

    1024x1024 physical framebuffer
    1 bit per pixel
    1024x800 visible area

    Contains a copy/fill blitter.

 **********************************************************************/

#include "emu.h"
#include "dn300.h"

#define LOG_GENERAL 0x01
#define LOG_SETUP   0x02

#define VERBOSE (LOG_SETUP | LOG_GENERAL)

#include "logmacro.h"

#define LOG(...)      LOGMASKED(LOG_GENERAL, __VA_ARGS__)
#define LOGSETUP(...) LOGMASKED(LOG_SETUP,   __VA_ARGS__)

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

DEFINE_DEVICE_TYPE(DN300_DISPLAY, dn300_display_device, "dn300_display", "DN300 Display Controller")

dn300_display_device::dn300_display_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, DN300_DISPLAY, tag, owner, clock)
	, device_video_interface(mconfig, *this)
    , m_vram(*this, "vram")
    , m_screen(*this, "screen")
    , m_palette(*this, "palette")
{
}

void dn300_display_device::device_start() 
{
	m_blitter_operation_timer = timer_alloc(FUNC(dn300_display_device::blitter_operation_timer_callback), this);
}

void dn300_display_device::device_reset() 
{
    m_display_control = 0;
    m_display_status = 0;

    memset(m_vram->pointer(), 0, 0x20000);

    m_blitter_operation_timer->adjust(attotime::never);
}

void dn300_display_device::device_add_mconfig(machine_config &config)
{
	RAM(config, m_vram).set_default_size("128K").set_default_value(0);

    // TODO: Figure out timings.
    SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_raw(53389000, 1104, 0, 1024, 806, 0, 800);
	m_screen->set_screen_update(FUNC(dn300_display_device::screen_update));
	m_screen->set_palette(m_palette);

    PALETTE(config, "palette", palette_device::MONOCHROME);
}

uint8_t dn300_display_device::vram_r(offs_t offset)
{
    return m_vram->read(offset);
}

void dn300_display_device::vram_w(offs_t offset, uint8_t data)
{
    m_vram->write(offset, data);
}

uint32_t dn300_display_device::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
    uint8_t *vram_base = m_vram->pointer();

    for(int row=0; row<800; row++)
    {
        for(int col=0; col<1024/8; col++)
        {
            uint8_t byte = vram_base[(row * 128) + col];
            
            bitmap.pix(row, col*8+0) = BIT(byte, 7) ? 0xFFFFFFFF : 0;
            bitmap.pix(row, col*8+1) = BIT(byte, 6) ? 0xFFFFFFFF : 0;
            bitmap.pix(row, col*8+2) = BIT(byte, 5) ? 0xFFFFFFFF : 0;
            bitmap.pix(row, col*8+3) = BIT(byte, 4) ? 0xFFFFFFFF : 0;
            bitmap.pix(row, col*8+4) = BIT(byte, 3) ? 0xFFFFFFFF : 0;
            bitmap.pix(row, col*8+5) = BIT(byte, 2) ? 0xFFFFFFFF : 0;
            bitmap.pix(row, col*8+6) = BIT(byte, 1) ? 0xFFFFFFFF : 0;
            bitmap.pix(row, col*8+7) = BIT(byte, 0) ? 0xFFFFFFFF : 0;
        }
    }

    return 0;
}

uint16_t dn300_display_device::regs_r(offs_t offset)
{
	/* 
	 * When read: Display Status (page 3-4 in Rev.1 manual)
	 *
	 * b15: blit in progress
	 * b7: end of frame interrupt
	 * b1: reserved
	 * b0: reserved
	 */

    switch(offset)
    {
        case Regs::DISPLAY_CONTROL: return m_display_status;
        case Regs::BLT_DCY:         return (-1 - (abs(m_wssy - m_wsdy)));
        case Regs::BLT_DCX:         return (-1 - (abs(m_wssx/16 - m_wsdx/16)));
        case Regs::BLT_END_BIT:     return m_wsdx % 16;

        // debugging
        case Regs::DEB:     return m_deb;
        case Regs::WSSY:    return m_wssy;
        case Regs::WSSX:    return m_wssx;
        case Regs::DCY:     return m_dcy;
        case Regs::DCX:     return m_dcx;
        case Regs::WSDY:    return m_wsdy;
        case Regs::WSDX:    return m_wsdx;

        default: return 0x0000;
    }
}

void dn300_display_device::regs_w(offs_t offset, uint16_t data)
{
    switch(offset)
    {
        case Regs::DISPLAY_CONTROL: update_display_control(data); break;
        case Regs::DEB:     m_deb = data;   break;
        case Regs::WSSY:    m_wssy = data;  break;
        case Regs::WSSX:    m_wssx = data;  break;
        case Regs::DCY:     m_dcy = data;   break;
        case Regs::DCX:     m_dcx = data;   break;
        case Regs::WSDY:    m_wsdy = data;  break;
        case Regs::WSDX:    m_wsdx = data;  break;
    }
}


TIMER_CALLBACK_MEMBER(dn300_display_device::blitter_operation_timer_callback)
{
    LOG("%s\n", FUNCNAME);
    // In a blit operation?
    if(BIT(m_display_control, 15))
    {
        // Interrupt at end of blit?
        if(BIT(m_display_control, 4))
        {
            LOG("%s: trigger an IRQ, end of blit!\n", FUNCNAME);
        }
        update_display_control(m_display_control & 0x7FFF);
    }

    LOG("%s: operation complete\n", FUNCNAME);
	
}


void dn300_display_device::update_display_control(uint16_t data)
{
	/* 
	 * When written: Display Control (page 3-4 in Rev.1 manual)
	 *
	 * b15: GO (start blit operation)
	 * b5: interrupt at end of frame?
	 * b4: interrupt at end of blit?
	 * b3: increment Y coord?
	 * b2: increment X coord?
	 * b1: fill-mode blit?
	 * b0: display enabled?
	 */

	LOG("%s: D:%04X\n", FUNCNAME, data);

	uint16_t old = m_display_control;
    m_display_control = data;

	if(BIT(data, 15) && !BIT(old, 15))
	{
		// DCX is in words(?), DCY is in scanlines.
		LOG("%s: CPU set blit GO bit\n", FUNCNAME);
		LOG("%s: CTRL: %04X DEB: %04X WSSY: %04X WSSX: %04X DCY: %04X DCX: %04X WSDY: %04X WSDX: %04X\n", FUNCNAME,
			data, m_deb, m_wssy, m_wssx, m_dcy, m_dcx, m_wsdy, m_wsdx);
		if(BIT(data, 1))
		{
            do_fill_blit(m_wssx, m_wssy, m_wsdx, m_wsdy, abs(m_dcx), abs(m_dcy));
		}
		else
		{
            do_copy_blit(m_wssx, m_wssy, m_wsdx, m_wsdy, abs(m_dcx), abs(m_dcy));
		}		

        m_blitter_operation_timer->adjust(attotime::from_nsec(200)); // TODO: arbitrary
	}

	if(BIT(data, 0) && !BIT(old, 0))
	{
		LOG("%s: Display now enabled\n", FUNCNAME);
	}
}

void dn300_display_device::do_fill_blit(uint16_t wssx, uint16_t wssy, uint16_t wsdx, uint16_t wsdy, uint16_t dcx, uint16_t dcy)
{
    // TODO: What determines whether it's a white fill or black fill?

    LOG("%s: doing fill blit from %d,%d to %d,%d of rect size %d,%d\n", FUNCNAME, wssx, wssy, wsdx, wsdy, dcx, dcy);

    bool increment_x = BIT(m_display_control, 2);
    bool increment_y = BIT(m_display_control, 3);
    // LOG("%s: increment X %d increment Y %d\n", FUNCNAME, increment_x, increment_y);

    // uint16_t end_bit_mask = (0xFFFF >> m_deb) << m_deb;
    // LOG("%s: end word mask is %04X. m_deb %d. start at bit %d end at bit %d\n", FUNCNAME, 
    //     end_bit_mask, m_deb, 15-(wsdx % 16), 15-m_deb);

    uint16_t cur_src_x = wssx;
    uint16_t cur_src_y = wssy;

    uint16_t cur_dst_x = wsdx;
    uint16_t cur_dst_y = wsdy;

    uint8_t start_bit = 15 - (cur_dst_x % 16);

    for(int scanline=0; scanline<abs(m_dcy); scanline++)
    {
        cur_src_x = wssx;
        cur_dst_x = wsdx;

        if(dcx > 1)
        {
            uint16_t words_to_go = dcx/2;

            for(int byte=0; byte<dcx-1; byte++)
            {
                // LOG("%s: (%d,%d) (b%d) <= (%d,%d) (b%d)\n", FUNCNAME,
                //     cur_dst_x, cur_dst_y, 15 - (cur_dst_x % 16),
                //     cur_src_x, cur_src_y, 15 - (cur_src_x % 16));
                for(int bit=0; bit<16; bit++)
                {
                    set_vram_pixel(cur_dst_x, cur_dst_y, 0);
                    if(increment_x) cur_src_x++;
                    cur_dst_x++;

                    if((cur_dst_x % 16) == 0) break;
                } 

                if((cur_dst_x % 16) == 0)
                {
                    // Rolled through a word. Do we have more than 16 bits to go?
                    words_to_go--;
                    if(words_to_go == 0)
                    {
                        // LOG("out of words\n");
                        start_bit = 15;
                        break;  
                    }
                }
            }
        }

        for(int bit=start_bit; (bit >= 0) && (bit != 15-m_deb-1); bit--)
        {
            // LOG("%s: (%d,%d) (b%d) <= (%d,%d) (b%d) [last word]\n", FUNCNAME, 
            //     cur_dst_x, cur_dst_y, 15 - (cur_dst_x % 16),
            //     cur_src_x, cur_src_y, 15 - (cur_src_x % 16));
            set_vram_pixel(cur_dst_x, cur_dst_y, 0);

            if(increment_x) cur_src_x++;
            cur_dst_x++;
        }

        if(increment_y) cur_src_y++;
        cur_dst_y++;
    }
}

void dn300_display_device::do_copy_blit(uint16_t wssx, uint16_t wssy, uint16_t wsdx, uint16_t wsdy, uint16_t dcx, uint16_t dcy)
{
    LOG("%s: doing copy blit from %d,%d to %d,%d of rect size %d,%d\n", FUNCNAME, wssx, wssy, wsdx, wsdy, dcx, dcy);

    bool increment_x = BIT(m_display_control, 2);
    bool increment_y = BIT(m_display_control, 3);
    // LOG("%s: increment X %d increment Y %d\n", FUNCNAME, increment_x, increment_y);

    // uint16_t end_bit_mask = (0xFFFF >> m_deb) << m_deb;
    // LOG("%s: end word mask is %04X. m_deb %d. start at bit %d end at bit %d\n", FUNCNAME, 
    //     end_bit_mask, m_deb, 15-(wsdx % 16), 15-m_deb);

    uint16_t cur_src_x = wssx;
    uint16_t cur_src_y = wssy;

    uint16_t cur_dst_x = wsdx;
    uint16_t cur_dst_y = wsdy;

    uint8_t start_bit = 15 - (cur_dst_x % 16);

    for(int scanline=0; scanline<abs(m_dcy); scanline++)
    {
        cur_src_x = wssx;
        cur_dst_x = wsdx;

        if(dcx > 1)
        {
            uint16_t words_to_go = dcx/2;

            for(int byte=0; byte<dcx-1; byte++)
            {
                // LOG("%s: (%d,%d) (b%d) <= (%d,%d) (b%d)\n", FUNCNAME,
                //     cur_dst_x, cur_dst_y, 15 - (cur_dst_x % 16),
                //     cur_src_x, cur_src_y, 15 - (cur_src_x % 16));
                for(int bit=0; bit<16; bit++)
                {
                    set_vram_pixel(cur_dst_x, cur_dst_y, get_vram_pixel(cur_src_x, cur_src_y));
                    if(increment_x) cur_src_x++;
                    cur_dst_x++;

                    if((cur_dst_x % 16) == 0) break;
                } 

                if((cur_dst_x % 16) == 0)
                {
                    // Rolled through a word. Do we have more than 16 bits to go?
                    words_to_go--;
                    if(words_to_go == 0)
                    {
                        // LOG("out of words\n");
                        start_bit = 15;
                        break;  
                    }
                }
            }
        }

        for(int bit=start_bit; (bit >= 0) && (bit != 15-m_deb-1); bit--)
        {
            // LOG("%s: (%d,%d) (b%d) <= (%d,%d) (b%d) [last word]\n", FUNCNAME, 
            //     cur_dst_x, cur_dst_y, 15 - (cur_dst_x % 16),
            //     cur_src_x, cur_src_y, 15 - (cur_src_x % 16));
            set_vram_pixel(cur_dst_x, cur_dst_y, get_vram_pixel(cur_src_x, cur_src_y));

            if(increment_x) cur_src_x++;
            cur_dst_x++;
        }

        if(increment_y) cur_src_y++;
        cur_dst_y++;
    }
}

bool dn300_display_device::get_vram_pixel(uint16_t x, uint16_t y)
{
    uint32_t offset;
    offset = (128*y) + (x/8);

    uint8_t vram_byte = m_vram->pointer()[offset];

    return BIT(vram_byte, 7-(x % 8));
}

void dn300_display_device::set_vram_pixel(uint16_t x, uint16_t y, bool value)
{
    uint32_t offset;
    offset = (128*y) + (x/8);

    uint8_t vram_byte = m_vram->pointer()[offset];
    uint8_t mask = ~(1 << (7-(x % 8)));
    
    vram_byte = (vram_byte & mask);
    if(value)
    {
        vram_byte |= (1 << (7-(x % 8)));
    }

    m_vram->pointer()[offset] = vram_byte;
}