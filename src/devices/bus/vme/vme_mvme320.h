// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
#ifndef MAME_BUS_VME_VME_MVME320_H
#define MAME_BUS_VME_VME_MVME320_H

#pragma once

#include "cpu/8x300/8x300.h"
#include "imagedev/floppy.h"
#include "bus/vme/vme.h"
#include "bus/rs232/rs232.h"
#include "machine/clock.h"
#include "machine/ram.h"
#include "machine/latch8.h"
#include "machine/fdc_pll.h"

DECLARE_DEVICE_TYPE(VME_MVME320, vme_mvme320_card_device)

class vme_mvme320_device : public device_t, public device_vme_card_interface, public device_execute_interface
{
public:
	vme_mvme320_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

protected:
    virtual void device_start() override;
	virtual void device_reset() override;
    virtual void execute_run() override;

	required_device<floppy_connector> m_fd0, m_fd1, m_fd2, m_fd3;

    void device_add_mconfig(machine_config &config) override;

    uint32_t m_eca_pointer;
    uint8_t m_interrupt_vector;
    uint8_t m_interrupt_source;
    uint8_t m_drive_status;

	struct live_info {
		enum { PT_NONE, PT_CRC_1, PT_CRC_2 };
        
        floppy_image_device *curfloppy;
		attotime tm;
		int state, next_state;
		uint16_t shift_reg;
		uint16_t crc;
		int bit_counter;
		bool data_separator_phase;
		uint8_t data_reg;
	};

	live_info cur_live, checkpoint_live;
	fdc_pll_t cur_pll, checkpoint_pll;

	void pll_reset(const attotime &when, int usec);
	//void pll_start_writing(const attotime &tm);
	void pll_commit(floppy_image_device *floppy, const attotime &tm);
	void pll_stop_writing(floppy_image_device *floppy, const attotime &tm);
	int pll_get_next_bit(attotime &tm, floppy_image_device *floppy, const attotime &limit);
	bool pll_write_next_bit(bool bit, attotime &tm, floppy_image_device *floppy, const attotime &limit);
	void pll_save_checkpoint();
	void pll_retrieve_checkpoint();

	void live_start(int live_state, floppy_image_device *curfloppy);
	void live_abort();
	void checkpoint();
	void rollback();
	void live_delay(int state);
	void live_sync();
	void live_run(attotime limit = attotime::never);
	bool read_one_bit(const attotime &limit);
	bool write_one_bit(const attotime &limit);

    void fdc_index_callback(floppy_image_device *floppy, int state);

    typedef enum
    {
        ECAPointerLow,      // $FFB001
        ECAPointerMiddle,   // $FFB003
        ECAPointerUpper,    // $FFB005
        ECAPointerNotUsed,  // $FFB007
        InterruptVector,    // $FFB009
        InterruptSource,    // $FFB00B
        DriveStatus         // $FFB00D
    } Register;

    typedef enum
    {
        Recalibrate,
        WriteDeletedData,
        Verify,
        TransparentRead,
        ReadIdentifier,
        ReadMultiple,
        WriteMultiple,
        FormatTrack
    } DriveCommand;

    typedef enum
    {
        Success,
        HardError,
        DriveNotReady,
        Reserved,
        SectorOutOfRange,
        ThroughputError,
        CommandRejected,
        Busy,
        DriveNotAvailable,
        DMAFailed,
        UserAbort
    } MainStatus;

    uint8_t registers_r(offs_t offset);
	void registers_w(offs_t offset, uint8_t data);

    void update_drive_status(uint8_t data);
    void begin_disk_command(int drive_index);
    void execute_disk_command(int drive_index);
    void command_set_status(uint8_t main, uint16_t extended);

    constexpr static int NUM_TRACKS[] = { 77, 77, -1, -1, 40, 40 }; // Number of tracks per side for each drive type

    typedef struct
    {
        uint8_t command_code;
        uint8_t main_status;
        uint16_t extended_status;
        uint8_t max_retries;
        uint8_t actual_retries;
        uint8_t dma_type;
        uint8_t command_options;
        uint16_t buffer_msw;
        uint16_t buffer_lsw;
        uint16_t buffer_length;
        uint16_t bytes_transferred;
        uint16_t cylinder_number;
        uint8_t head_number;
        uint8_t sector_number;
        uint16_t current_cylinder;
        uint8_t reserved10[10];
        uint8_t preindex_gap;
        uint8_t postindex_gap;
        uint8_t sync_byte_count;
        uint8_t postid_gap;
        uint8_t postdata_gap;
        uint8_t address_mark_count;
        uint8_t sector_length_code;
        uint8_t fill_byte;
        uint8_t reserved6[6];
        uint8_t drive_type;
        uint8_t num_surfaces;
        uint8_t sectors_per_track;
        uint8_t step_rate;
        uint8_t head_settling_time;
        uint8_t head_load_time;
        uint8_t seek_type;
        uint8_t phase_count;
        uint16_t low_write_current_cylinder;
        uint16_t precomp_cylinder;
        uint8_t ecc_remainder[6];
        uint8_t ecc_remainder_from_disk[6];
        uint8_t reserved4[4];
        uint8_t working_area[12];
        uint16_t reserved;
    } ECA;
    ECA m_current_eca;

	enum { TM_HEAD_LOAD, TM_TIMEOUT, TM_GEN };

    floppy_image_device *m_floppy[4];
    emu_timer *t_gen;
    emu_timer *m_command_exec_timer;
    emu_timer *m_ready_timeout;
    emu_timer *m_step_time;
	TIMER_CALLBACK_MEMBER(command_exec_timer_expired);
    TIMER_CALLBACK_MEMBER(ready_timeout);
    TIMER_CALLBACK_MEMBER(step_time);

    int m_current_command_drive_number;
    bool m_timeout;

    bool perform_recalibrate(int drive_index);
    bool perform_read_multiple(int drive_index);

    typedef enum
    {
        CMD_IDLE,
        CMD_0_WAITING_FOR_READY,
        CMD_0_STEPPING,
    } ControllerState;
    ControllerState m_controller_state;

    enum
    {
        IDLE,
		//
		SYNC1,
		SYNC_BYTE1,
		SYNC2,
		SYNC_BYTE2,
		READ,
		READ_BYTE,
		WRITE,
		WRITE_BITS,
    };

    enum
    {
        TRIGGER_READY,
        TRIGGER_STEP
    };

    void floppy_ready_cb(floppy_image_device *floppy, int state);

    int m_icount;

    void start_read();

    uint16_t m_shift_register;
    uint8_t read_byte_from_shift_register();

    u8 m_crc;
	u8 m_last_crc;
};

class vme_mvme320_card_device : public vme_mvme320_device
{
public:
	vme_mvme320_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	vme_mvme320_card_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme320_device(mconfig, type, tag, owner, clock)
	{ }
};

#endif // MAME_BUS_VME_VME_MVME320_H