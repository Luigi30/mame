// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*
 *	Rastergraf RG-750 VMEbus graphics card
 *
 *  Features:
 *	- TMS34010 CPU
 *	- 1 or 4MB of DRAM
 *	- 1MB of VRAM
 *	- RS-232 mouse and AT keyboard interface
 *  - SVGA output up to 1024x768
 *
 */

#include "emu.h"
#include "vme_rg750.h"

#define LOG_PRINTF  (1U << 1)
#define LOG_SETUP 	(1U << 2)
#define LOG_GENERAL (1U << 3)

#define VERBOSE (LOG_PRINTF | LOG_SETUP | LOG_GENERAL)
 
#include "logmacro.h"

#define LOGPRINTF(...) 	LOGMASKED(LOG_PRINTF, 	__VA_ARGS__)
#define LOGSETUP(...) 	LOGMASKED(LOG_SETUP, 	__VA_ARGS__)
#define LOGGENERAL(...) LOGMASKED(LOG_GENERAL, 	__VA_ARGS__)

#define MASTER_XTAL	( 40_MHz_XTAL )

DEFINE_DEVICE_TYPE(VME_RG750,   vme_rg750_card_device,   "rg750",   "Rastergraf RG-750")

static INPUT_PORTS_START(rg750)

INPUT_PORTS_END

ioport_constructor vme_rg750_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(rg750);
}

vme_rg750_device::vme_rg750_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock)
	, device_vme_card_interface(mconfig, *this)
	, m_maincpu (*this, "maincpu")
	, m_uart (*this, "uart")
	, m_screen (*this, "screen")
	, m_vdac (*this, "bt478")
	, m_vram(*this, "vram")
	, m_dram(*this, "dram")
{

}

vme_rg750_card_device::vme_rg750_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
		: vme_rg750_card_device(mconfig, VME_RG750, tag, owner, clock)
{

}

void vme_rg750_device::rg750_mem(address_map &map)
{
	map(0x02000000, 0x02ffffff).ram().share("vram");
	map(0x03000000, 0x037fffff).ram().share("dram");
	
	map(0x05000000, 0x0500006f).rw(FUNC(vme_rg750_device::uart_r), FUNC(vme_rg750_device::uart_w)).umask32(0x000000FF);
	map(0x05400000, 0x0540000f).rw(FUNC(vme_rg750_device::ramdac_address_read), FUNC(vme_rg750_device::ramdac_address_write)).umask32(0x000000FF);
	map(0x05410000, 0x0541000f).rw(FUNC(vme_rg750_device::ramdac_palette_read), FUNC(vme_rg750_device::ramdac_palette_write)).umask32(0x000000FF);
	map(0x05800000, 0x05bfffff).rw(FUNC(vme_rg750_device::ctrlreg_r), FUNC(vme_rg750_device::ctrlreg_w));		// TODO: mirroring
	map(0x05c00000, 0x05ffffff).rw(FUNC(vme_rg750_device::statusreg_r), FUNC(vme_rg750_device::statusreg_w));	// TODO: mirroring
	map(0x06c00000, 0x06ffffff).rom().region("maincpu", 0x0000);
	map(0xffc00000, 0xffffffff).rom().region("maincpu", 0x0000); // can be replaced by DRAM by ROMDIS register
	
}

uint8_t vme_rg750_device::uart_r(offs_t offset)
{
	return m_uart->read(offset);
}

void vme_rg750_device::uart_w(offs_t offset, uint8_t data)
{
	m_uart->write(offset, data);
}

void vme_rg750_device::device_start()
{
	LOG("%s\n", FUNCNAME);

}

void vme_rg750_device::device_reset()
{
	LOG("%s\n", FUNCNAME);
}

/* 
 *	Control register (per manual)
 *	D15 - Red LED
 *	D14 - Debug Enable
 * 	D13 - ROMDIS
 *	D12 - VSYNC polarity
 *	D11 - HSYNC polarity
 *	D10 - Pixel clock select (0 = internal, 1 = auxiliary)
 *	D09 - VRAM mapping bit 0
 *	D08 - Green LED
 *	D07 - Sync on green (active low)
 *	D06 - Keyboard serial data
 *	D05 - Keyboard clock
 *	D04 - Memory installed (0 = 1MB, 1 = 4MB)
 *	D03 - Video bit depth (0 = 4bpp, 1 = 8bpp)
 *	D02 - VRAM mapping bit 1
 * 	D01 - Pixel clock divisor (0 = divide by 1, 1 = divide by 2)
 *	D00 - Shift clock enable
 */
uint16_t vme_rg750_device::ctrlreg_r(offs_t offset)
{
	return m_ctrlreg;
}

void vme_rg750_device::ctrlreg_w(offs_t offset, uint16_t data)
{
	LOG("%s: control register set to %X $%02X\n", FUNCNAME, offset, data);
	m_ctrlreg = data;

	if(BIT(m_ctrlreg, 1)) m_maincpu->set_pixels_per_clock(2); else m_maincpu->set_pixels_per_clock(1);
}

/* 	
 *	Status register (per manual)
 *	D11 - 0 = Jumper installed at J19 3-4
 *	D10 - 0 = Jumper installed at J19 1-2
 *	D09 - Reserved
 *	D08 - VSYNC
 *	D07 - HSYNC
 *	D06 - Keyboard data
 *	D05 - Keyboard clock
 *	D04 - CSYNC
 *	D03 - Config
 *	D02 - Config
 * 	D01 - Config
 *	D00 - Config
 */
uint16_t vme_rg750_device::statusreg_r(offs_t offset)
{
	m_statusreg = 0;
	return m_statusreg;
}

void vme_rg750_device::statusreg_w(offs_t offset, uint16_t data) { }

/*
 * Machine configuration
 */

void vme_rg750_device::device_add_mconfig(machine_config &config)
{
	TMS34010(config, m_maincpu, MASTER_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &vme_rg750_device::rg750_mem);
	m_maincpu->set_halt_on_reset(false);     /* halt on reset */
	m_maincpu->set_pixel_clock(25175000); 	/* pixel clock */
	m_maincpu->set_pixels_per_clock(1);
	m_maincpu->set_scanline_ind16_callback(*this, FUNC(vme_rg750_device::scanline_update));  /* scanline updater (indexed16) */
	m_maincpu->set_shiftreg_in_callback(*this, FUNC(vme_rg750_device::to_shiftreg));         /* write to shiftreg function */
	m_maincpu->set_shiftreg_out_callback(*this, FUNC(vme_rg750_device::from_shiftreg));      /* read from shiftreg function */

	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_raw(25175000, 800, 0, 640, 525, 0, 480);	// VGA timings for now, but supports 800 and 1024 SVGA
	m_screen->set_size(640, 480);
	m_screen->set_visarea(0, 640-1, 0, 480-1);
	m_screen->set_screen_update(FUNC(vme_rg750_device::screen_update));
	m_maincpu->set_screen(m_screen);

	SCN2681(config, "uart", 3.6864_MHz_XTAL);

	/*
	TIMER(config, m_scantimer, 0);
	m_scantimer->configure_scanline(FUNC(kn01_state::scanline_timer), "screen", 0, 1);
	*/

	BT478(config, m_vdac, 25175000);
	
	VME(config, "vme", 0);
}

uint32_t vme_rg750_device::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, rectangle const &cliprect)
{
	// VRAM is 1024x1024 in 8bpp mode and 1024x2048 in 4bpp mode.
	/*
	// TODO: 8bpp modes
	for (unsigned y = 0; y < 2048; y++)
	{
		u32 *scanline = &bitmap.pix(y);
		for (unsigned x = 0; x < 640; x++)
		{
			u8 pixel = m_vram[(y * 1024) + x];
			*scanline++ = m_vdac->palette_lookup(pixel);
			// u8 const pixels = m_vram->read((y * 1024) + BYTE4_XOR_BE(x));
			// *scanline++ = m_vdac->palette_lookup(pixels);
		}
	}
	*/

	return 0;
}

TMS340X0_SCANLINE_IND16_CB_MEMBER(vme_rg750_device::scanline_update)
{
	// TODO
}

TMS340X0_TO_SHIFTREG_CB_MEMBER(vme_rg750_device::to_shiftreg)
{
	// TODO
}

TMS340X0_FROM_SHIFTREG_CB_MEMBER(vme_rg750_device::from_shiftreg)
{
	// TODO
}


// ROM definitions
ROM_START (rg750)
	ROM_REGION16_LE(0x80000, "maincpu", 0)
	ROM_LOAD16_BYTE("afgis-v3.11a-u18.bin", 0x00000, 0x40000, CRC(71c67430) SHA1(52872291b160abd11b720a6949c2246bd32c80c4))
	ROM_LOAD16_BYTE("afgis-v3.11a-u17.bin", 0x00001, 0x40000, CRC(8cd81681) SHA1(0b19f06bef5cf43ccfe2e24bbc8c7067b2eb3cfa))
ROM_END

const tiny_rom_entry *vme_rg750_device::device_rom_region() const
{
	return ROM_NAME(rg750);
}
