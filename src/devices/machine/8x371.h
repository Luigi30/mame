// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*****************************************************************************

    Signetics 8X371 8-Bit Latched Bidirectional I/O Port

*****************************************************************************

    Connection Diagram:

            SN5404 J Package
            SN54LS04, SN54S04 J or W Package
            SN7404 D, DB, N or NS Package
            SN74S04 D or N Package
                 ___ ___
        UD7  1  |*  u   | 24 Vcc
        UD6  2  |       | 23  /IV7
        UD5  3  |       | 22  /IV6
        UD4  4  |       | 21  /IV5
        UD3  5  |       | 20  /IV4
        UD2  6  |       | 19  /IV3
        UD1  7  |       | 18  /IV2
        UD0  8  |       | 17  /IV1
       /UOC  9  |       | 16  /IV0
       /UIC  10 |       | 15  WC
        /ME  11 |       | 14  RC
        Gnd  12 |_______| 13  MCLK


*****************************************************************************/

#ifndef MAME_MACHINE_8X371_H
#define MAME_MACHINE_8X371_H

#pragma once

class n8x371_device : public device_t
{
public:
	// construction/destruction
	n8x371_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock = 0);

	// write & read full byte. reading from the opposite bus of where the data came from inverts it.
	uint8_t read_iv() { if(m_value_came_from_iv) return m_value; else return ~m_value; }
	void write_iv(uint8_t data);

    uint8_t read_ud() { if(m_value_came_from_iv) return ~m_value; else return m_value; }
    void write_ud(uint8_t data);

	// public interfaces
    DECLARE_WRITE_LINE_MEMBER( rc_w )   { m_rc_state = state; }         // active low
    DECLARE_WRITE_LINE_MEMBER( wc_w )   { m_wc_state = state; }
    DECLARE_WRITE_LINE_MEMBER( uoc_w )  { m_uoc_state = state; }        // active low
    DECLARE_WRITE_LINE_MEMBER( uic_w )  { m_uic_state = state; }        // active low
    DECLARE_WRITE_LINE_MEMBER( me_w )   { m_me_state = state; }         // active low
    DECLARE_WRITE_LINE_MEMBER( mclk_w ) { if(state == 1 && m_me_state == 1) update(); }

    // IV bus (active low)
    DECLARE_WRITE_LINE_MEMBER( iv0_w );
	DECLARE_WRITE_LINE_MEMBER( iv1_w );
	DECLARE_WRITE_LINE_MEMBER( iv2_w );
	DECLARE_WRITE_LINE_MEMBER( iv3_w );
	DECLARE_WRITE_LINE_MEMBER( iv4_w );
	DECLARE_WRITE_LINE_MEMBER( iv5_w );
	DECLARE_WRITE_LINE_MEMBER( iv6_w );
    DECLARE_WRITE_LINE_MEMBER( iv7_w );

    // really it's three-stated
	DECLARE_READ_LINE_MEMBER( iv0_r )   { if(m_rc_state == 0) return BIT(m_value, 7); else return 0; };
	DECLARE_READ_LINE_MEMBER( iv1_r )   { if(m_rc_state == 0) return BIT(m_value, 6); else return 0; };
	DECLARE_READ_LINE_MEMBER( iv2_r )   { if(m_rc_state == 0) return BIT(m_value, 5); else return 0; };
	DECLARE_READ_LINE_MEMBER( iv3_r )   { if(m_rc_state == 0) return BIT(m_value, 4); else return 0; };
	DECLARE_READ_LINE_MEMBER( iv4_r )   { if(m_rc_state == 0) return BIT(m_value, 3); else return 0; };
	DECLARE_READ_LINE_MEMBER( iv5_r )   { if(m_rc_state == 0) return BIT(m_value, 2); else return 0; };
    DECLARE_READ_LINE_MEMBER( iv6_r )   { if(m_rc_state == 0) return BIT(m_value, 1); else return 0; };
    DECLARE_READ_LINE_MEMBER( iv7_r )   { if(m_rc_state == 0) return BIT(m_value, 0); else return 0; };

    // UD bus
	DECLARE_WRITE_LINE_MEMBER( ud0_w );
	DECLARE_WRITE_LINE_MEMBER( ud1_w );
	DECLARE_WRITE_LINE_MEMBER( ud2_w );
	DECLARE_WRITE_LINE_MEMBER( ud3_w );
	DECLARE_WRITE_LINE_MEMBER( ud4_w );
	DECLARE_WRITE_LINE_MEMBER( ud5_w );
	DECLARE_WRITE_LINE_MEMBER( ud6_w );
    DECLARE_WRITE_LINE_MEMBER( ud7_w );

    // really it's three-stated
	DECLARE_READ_LINE_MEMBER( ud0_r )   { if(m_uoc_state == 0) return !BIT(m_value, 7); else return 0; };
	DECLARE_READ_LINE_MEMBER( ud1_r )   { if(m_uoc_state == 0) return !BIT(m_value, 6); else return 0; };
	DECLARE_READ_LINE_MEMBER( ud2_r )   { if(m_uoc_state == 0) return !BIT(m_value, 5); else return 0; };
	DECLARE_READ_LINE_MEMBER( ud3_r )   { if(m_uoc_state == 0) return !BIT(m_value, 4); else return 0; };
	DECLARE_READ_LINE_MEMBER( ud4_r )   { if(m_uoc_state == 0) return !BIT(m_value, 3); else return 0; };
	DECLARE_READ_LINE_MEMBER( ud5_r )   { if(m_uoc_state == 0) return !BIT(m_value, 2); else return 0; };
    DECLARE_READ_LINE_MEMBER( ud6_r )   { if(m_uoc_state == 0) return !BIT(m_value, 1); else return 0; };
    DECLARE_READ_LINE_MEMBER( ud7_r )   { if(m_uoc_state == 0) return !BIT(m_value, 0); else return 0; };

protected:
	// void ud_w(uint8_t line, bool state);
	// uint8_t ud_r(uint8_t line);

	// void iv_w(uint8_t line, bool state);
	// uint8_t iv_r(uint8_t line);

	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

private:
	void update();
    bool m_value_came_from_iv;

    devcb_write_line::array<8> m_write_cb;
	devcb_read_line::array<8> m_read_cb;

	// internal state
    uint8_t m_ud_incoming;
    uint8_t m_iv_incoming;

	uint8_t m_value;
    bool m_rc_state;
    bool m_wc_state;
    bool m_uoc_state;
    bool m_uic_state;
    bool m_me_state;
};


// device type definition
DECLARE_DEVICE_TYPE(N8X371, n8x371_device)

#endif // MAME_MACHINE_8X371_H
