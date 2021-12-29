// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*
 * Motorola MVME320 VMEbus Disk Controller Module
 *
 * ST-506 and Shugart SA-400 disk controller, FM and MFM.
 * Supports 8" and 5.25" disk drives plus ST-506 hard drives.
 * 
 * Consists of a Signetics N8X305 running sequencer firmware and a bunch of TTL for controlling disks.
 * But that's all *way* too slow when emulated properly.
 *
 * There are three versions of the MVME320:
 * - MVME320: Has weird connectors and requires an MVME901 adapter board to connect to drives.
 * - MVME320/A: Simplified layout, standard connectors.
 * - MVME320/B: Adds support for 1.2M floppy drives.
 *  */

#include "emu.h"
#include "vme_mvme320.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define LOG_PRINTF  (1U << 1)
#define LOG_SETUP   (1U << 2)
#define LOG_GENERAL (1U << 3)

#define VERBOSE (LOG_GENERAL)

#include "logmacro.h"

#define LOGPRINTF(...)  LOGMASKED(LOG_PRINTF,   __VA_ARGS__)
#define LOGSETUP(...)   LOGMASKED(LOG_SETUP,    __VA_ARGS__)
#define LOGGENERAL(...) LOGMASKED(LOG_GENERAL,  __VA_ARGS__)

DEFINE_DEVICE_TYPE(VME_MVME320, vme_mvme320_card_device, "mvme320", "Motorola MVME320/B Disk Controller")

vme_mvme320_device::vme_mvme320_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock)
	, device_vme_card_interface(mconfig, *this)
    , device_execute_interface(mconfig, *this)
	, m_fd0(*this, "floppy0")
	, m_fd1(*this, "floppy1")
	, m_fd2(*this, "floppy2")
	, m_fd3(*this, "floppy3")
    , m_command_exec_timer(nullptr)
    , m_icount(0)
{
}


vme_mvme320_card_device::vme_mvme320_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme320_card_device(mconfig, VME_MVME320, tag, owner, clock)
{

}

void vme_mvme320_device::execute_run()
{
    // We only wake up in response to register reads or writes.
    // Otherwise, we're waiting on timers and triggers.
    do
    {
        switch(m_controller_state)
        {
            default:
            break;
        }
        
        m_icount--;
    }
    while(m_icount > 0);
}

void vme_mvme320_device::device_start()
{
    set_icountptr(m_icount);

    m_floppy[0] = m_fd0->get_device();
    m_floppy[1] = m_fd1->get_device();
    m_floppy[2] = m_fd2->get_device();
    m_floppy[3] = m_fd3->get_device();

    m_vme->install_device(vme_device::A16_SC, 0xFFB000, 0xFFB00F,
    read8sm_delegate(*this, FUNC(vme_mvme320_device::registers_r)), 
    write8sm_delegate(*this, FUNC(vme_mvme320_device::registers_w)), 
    0x00FF00FF);

    save_item(NAME(m_eca_pointer));
    save_item(NAME(m_interrupt_vector));
    save_item(NAME(m_interrupt_source));
    save_item(NAME(m_drive_status));

    m_command_exec_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(vme_mvme320_device::command_exec_timer_expired), this));
    m_ready_timeout = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(vme_mvme320_device::ready_timeout), this));
    m_step_time = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(vme_mvme320_device::step_time), this));

    t_gen = timer_alloc(TM_GEN);
}


void vme_mvme320_device::device_reset()
{
    m_eca_pointer = 0;
    m_interrupt_vector = 0x0F;
    m_interrupt_source = 0;
    m_drive_status = 0;
}

TIMER_CALLBACK_MEMBER(vme_mvme320_device::command_exec_timer_expired)
{
    uint32_t eca_table = (m_eca_pointer << 1);
    eca_table += (4*m_current_command_drive_number);

    uint32_t eca_address = 0;
    eca_address |= (read8(vme_device::A24_SC, eca_table+0) << 24);
    eca_address |= (read8(vme_device::A24_SC, eca_table+1) << 16);
    eca_address |= (read8(vme_device::A24_SC, eca_table+2) << 8);
    eca_address |= read8(vme_device::A24_SC, eca_table+3);

    execute_disk_command(m_current_command_drive_number);
    
    // Transmit the status back to the host system.
    write8(vme_device::A24_SC, eca_address+2, (m_current_eca.extended_status & 0xFF00) >> 8);
    write8(vme_device::A24_SC, eca_address+3, m_current_eca.extended_status & 0xFF);
    write8(vme_device::A24_SC, eca_address+1, m_current_eca.main_status);
}

TIMER_CALLBACK_MEMBER(vme_mvme320_device::ready_timeout)
{
    m_timeout = true;
}

TIMER_CALLBACK_MEMBER(vme_mvme320_device::step_time)
{
    trigger(TRIGGER_STEP);
}

uint8_t vme_mvme320_device::registers_r(offs_t offset)
{
    switch((vme_mvme320_device::Register)offset)
    {
        case ECAPointerLow:
            return m_eca_pointer & 0xFF;
        case ECAPointerMiddle:
            return (m_eca_pointer & 0xFF00) >> 8;
        case ECAPointerUpper:
            return (m_eca_pointer & 0xFF0000) >> 16;
        case ECAPointerNotUsed:
            return (m_eca_pointer & 0xFF000000 >> 24);
        case InterruptVector:
            return m_interrupt_vector & 0xFF;
        case InterruptSource:
            return m_interrupt_source & 0xF0;
        case DriveStatus:
            return m_drive_status & 0xF0;
        default:
            return 0xFF;
    }
}

void vme_mvme320_device::registers_w(offs_t offset, uint8_t data)
{
    LOGGENERAL("MVME320: register write %02X %02X\n", offset, data);    
    switch((vme_mvme320_device::Register)offset)
    {
        case ECAPointerLow:
            m_eca_pointer = (m_eca_pointer & 0xFFFFFF00) | data;
            break;
        case ECAPointerMiddle:
            m_eca_pointer = (m_eca_pointer & 0xFFFF00FF) | (((uint32_t)data) << 8);
            break;
        case ECAPointerUpper:
            m_eca_pointer = (m_eca_pointer & 0xFF00FFFF) | (((uint32_t)data) << 16);
            break;
        case ECAPointerNotUsed:
            m_eca_pointer = (m_eca_pointer & 0x00FFFFFF) | (((uint32_t)data) << 24);
            break;
        case InterruptVector:
            m_interrupt_vector = data;
            break;
        case InterruptSource:
            m_interrupt_source = data & 0xF0;
            break;
        case DriveStatus:
            update_drive_status(data);
            break;
    }
}

/////////////////////////////
void vme_mvme320_device::update_drive_status(uint8_t data)
{
    // The host system wrote Register $D.
    
    // Find any 0-to-1 transitions between m_drive_status and data.
    bool pending_operation[4];
    pending_operation[0] = (BIT(data, 4) & !BIT(m_drive_status, 4));
    pending_operation[1] = (BIT(data, 5) & !BIT(m_drive_status, 5));
    pending_operation[2] = (BIT(data, 6) & !BIT(m_drive_status, 6));
    pending_operation[3] = (BIT(data, 7) & !BIT(m_drive_status, 7));

    m_drive_status = data;

    // Find any 1-to-0 transitions between m_drive_status and data.
    // bool cancel[4];
    // cancel[0] = (!BIT(data, 4)) & BIT(m_drive_status, 4);
    // cancel[1] = (!BIT(data, 5)) & BIT(m_drive_status, 5);
    // cancel[2] = (!BIT(data, 6)) & BIT(m_drive_status, 6);
    // cancel[3] = (!BIT(data, 7)) & BIT(m_drive_status, 7);

    // If we have any pending operations, proceed and perform them.
    for(int i=0; i<4; i++)
    {
        if(pending_operation[i]) { begin_disk_command(i); break; }
    }
    
}

void vme_mvme320_device::begin_disk_command(int drive_index)
{ 
    LOGGENERAL("begin disk command: drive %d\n", drive_index);

    uint32_t eca_table = (m_eca_pointer << 1);
    eca_table += (4*drive_index);
    
    uint32_t eca_address = 0;
    eca_address |= (read8(vme_device::A24_SC, eca_table+0) << 24);
    eca_address |= (read8(vme_device::A24_SC, eca_table+1) << 16);
    eca_address |= (read8(vme_device::A24_SC, eca_table+2) << 8);
    eca_address |= read8(vme_device::A24_SC, eca_table+3);

    LOGGENERAL("ECA table is at %08X, ECA is at %08X\n", eca_table, eca_address);

    uint8_t *eca = (uint8_t *)&m_current_eca;
    for(int i=0; i<sizeof(ECA); i++)
    {
        *(eca+i) = read8(vme_device::A24_SC, eca_address+i);

        LOGGENERAL("Reading ECA: %08X %02X\n", eca_address+i, *(eca+i));
    }

    // Wait a bit before proceeding.
    m_current_command_drive_number = drive_index;
    m_command_exec_timer->adjust(attotime::from_usec(100));
    m_command_exec_timer->enable(true);
}

void vme_mvme320_device::floppy_ready_cb(floppy_image_device *floppy, int state)
{
    if(state == 0) trigger(TRIGGER_READY);
}

bool vme_mvme320_device::perform_recalibrate(int drive_index)
{
    // Move head to track 0. Return 1 on success, 0 on failure.

    // Preconditions:
    if(m_current_eca.drive_type > 5) return 0;
    int num_tracks = NUM_TRACKS[m_current_eca.drive_type];
    
    // Is the drive READY?
    if(m_floppy[drive_index]->ready_r() == 1)
    {
        // Wait for READY or fail.
        m_timeout = false;

        m_command_exec_timer->adjust(attotime::from_msec(500));
        m_command_exec_timer->enable(true);

        spin_until_trigger(TRIGGER_READY);

        // Either ready or timed out.
        if(m_floppy[drive_index]->ready_r() == 1) return 0;    
    }

    LOGGENERAL("Drive READY. Stepping to track 0.\n");

    // The ECA will specify the step interval which we will use for the timer.
    // Step rate is in 500usec units.
    uint32_t step_rate = m_current_eca.step_rate * 500;    
    if(m_floppy[drive_index]->trk00_r() != 0)
    {
        m_floppy[drive_index]->dir_w(1);
        for(int i=0; i<num_tracks; i++)
        {
            m_floppy[drive_index]->stp_w(1);
            m_floppy[drive_index]->stp_w(0);
            spin_until_time(attotime::from_usec(step_rate));

            if(m_floppy[drive_index]->trk00_r() == 0) return 1;
        }

        return m_floppy[drive_index]->trk00_r() == 0;
    }

    return 1;
}

/*******************
 * PLL
 */
void vme_mvme320_device::pll_reset(const attotime &when, int nsec)
{
	cur_pll.reset(when);
	cur_pll.set_clock(attotime::from_nsec(nsec));
}

void vme_mvme320_device::pll_commit(floppy_image_device *floppy, const attotime &tm)
{
	cur_pll.commit(floppy, tm);
}

void vme_mvme320_device::pll_stop_writing(floppy_image_device *floppy, const attotime &tm)
{
	cur_pll.stop_writing(floppy, tm);
}

void vme_mvme320_device::pll_save_checkpoint()
{
	checkpoint_pll = cur_pll;
}

void vme_mvme320_device::pll_retrieve_checkpoint()
{
	cur_pll = checkpoint_pll;
}

int vme_mvme320_device::pll_get_next_bit(attotime &tm, floppy_image_device *floppy, const attotime &limit)
{
	return cur_pll.get_next_bit(tm, floppy, limit);
}

bool vme_mvme320_device::pll_write_next_bit(bool bit, attotime &tm, floppy_image_device *floppy, const attotime &limit)
{
	return cur_pll.write_next_bit(bit, tm, floppy, limit);
}

bool vme_mvme320_device::read_one_bit(const attotime &limit)
{
	int bit = pll_get_next_bit(cur_live.tm, cur_live.curfloppy, limit);
	if(bit < 0)
		return true;
	cur_live.shift_reg = (cur_live.shift_reg << 1) | bit;
	cur_live.bit_counter++;
	if(cur_live.data_separator_phase) {
		cur_live.data_reg = (cur_live.data_reg << 1) | bit;
		if((cur_live.crc ^ (bit ? 0x8000 : 0x0000)) & 0x8000)
			cur_live.crc = (cur_live.crc << 1) ^ 0x1021;
		else
			cur_live.crc = cur_live.crc << 1;
	}
	cur_live.data_separator_phase = !cur_live.data_separator_phase;
	return false;
}

void vme_mvme320_device::live_start(int state, floppy_image_device *curfloppy)
{
    cur_live.curfloppy = curfloppy;
	cur_live.tm = machine().time();
	cur_live.state = state;
	cur_live.next_state = -1;
	cur_live.shift_reg = 0;
	cur_live.crc = 0xffff;
	cur_live.bit_counter = 0;
	cur_live.data_separator_phase = false;
	cur_live.data_reg = 0;

    // Track 0 is always assumed to be FM, so 8uS instead of 4uS.
	pll_reset(cur_live.tm, 4000); // set nsec according to drive type (8", 5.25", HDD)
	checkpoint_live = cur_live;
	pll_save_checkpoint();

	live_run();

    LOG("status is now %d\n", cur_live.state);
}

// Controller assumes sync byte is always $00.
#define sync 00

void vme_mvme320_device::checkpoint()
{
	pll_commit(cur_live.curfloppy, cur_live.tm);
	checkpoint_live = cur_live;
	pll_save_checkpoint();
}

void vme_mvme320_device::live_delay(int state)
{
	cur_live.next_state = state;
	t_gen->adjust(cur_live.tm - machine().time());
}

uint8_t vme_mvme320_device::read_byte_from_shift_register()
{
    return  (BIT(cur_live.shift_reg, 1)) | 
            (BIT(cur_live.shift_reg, 3))  << 1 |
            (BIT(cur_live.shift_reg, 5))  << 2 |
            (BIT(cur_live.shift_reg, 7))  << 3 |
            (BIT(cur_live.shift_reg, 9))  << 4 |
            (BIT(cur_live.shift_reg, 11)) << 5 |
            (BIT(cur_live.shift_reg, 13)) << 6 |
            (BIT(cur_live.shift_reg, 15)) << 7;
}

int bytes[512];
int byte_count = 0;

void vme_mvme320_device::live_run(attotime limit)
{
    byte_count = 0;

	if(cur_live.state == IDLE || cur_live.next_state != -1)
		return;

	if(limit == attotime::never) {
		if(cur_live.curfloppy)
			limit = cur_live.curfloppy->time_next_index();
		if(limit == attotime::never) {
			// Happens when there's no disk or if the wd is not
			// connected to a drive, hence no index pulse. Force a
			// sync from time to time in that case, so that the main
			// cpu timeout isn't too painful.  Avoids looping into
			// infinity looking for data too.

			limit = machine().time() + attotime::from_msec(1);
			t_gen->adjust(attotime::from_msec(1));
		}
	}

    // for now let's just worry about track 0, FM
    int count = 0;
    bool sync1 = false;
    bool sync2 = false;

    for(;;)
    {
        if (cur_live.tm > limit)
            return;

        if(read_one_bit(limit))
            return;

        if (cur_live.bit_counter == 16) {
            cur_live.bit_counter = 0;
        }

        // Step 1: find a $00 sync byte

        // This looks like a sync byte.
        if(!sync1 || !sync2)
        {
            if(read_byte_from_shift_register() == 0)
            {
                // How many consecutive?
                if(!sync1)
                {
                    // First sync byte.
                    sync1 = true;
                    continue;
                }
                else if(sync1 && !sync2)
                {
                    sync2 = true;
                    checkpoint();
                    cur_live.state = SYNC2;
                    continue;
                }
            }
            else
            {
                // This is not a sync byte, reset and try again.
                sync1 = false;
                sync2 = false;
            }   
        }
        else
        {
            // Synced? Maybe?
            if(cur_live.bit_counter == 0)
            {
                LOGGENERAL("byte %d %x\n", count++, read_byte_from_shift_register());
            }
        }
    }
}

bool vme_mvme320_device::perform_read_multiple(int drive_index)
{
    // 8" drive - 500KHz
    // 5.25" drive - 250KHz
    live_start(SYNC1, m_floppy[drive_index]);

    return 0;
}

void vme_mvme320_device::execute_disk_command(int drive_index)
{
    // Execute one of the supported commands or return an error.
    //floppy_image_device *drive[4] = { m_fd0->get_device(), m_fd1->get_device(), m_fd2->get_device(), m_fd3->get_device() };
    //floppy_image_device *floppy = drive[drive_index];

    LOGGENERAL("Executing disk command %d\n", m_current_eca.command_code);

    // Motor on for all drives no matter which one is in use.
    for(int i=0; i<4; i++) { m_floppy[i]->mon_w(0); }

    switch(m_current_eca.command_code)
    {
        case DriveCommand::Recalibrate:
            if(perform_recalibrate(drive_index))
            {
                LOGGENERAL("Recalibrate: SUCCESS\n");
                command_set_status(Success, 0x0000);
            }
            else
            {
                LOGGENERAL("Recalibrate: FAILURE\n");
                command_set_status(DriveNotReady, 0x0010);
            }
            break;
        case DriveCommand::WriteDeletedData:
            command_set_status(CommandRejected, 0xFFFF);
            break;
        case DriveCommand::Verify:
            command_set_status(CommandRejected, 0xFFFF);
            break;
        case DriveCommand::TransparentRead:
            command_set_status(CommandRejected, 0xFFFF);
            break;
        case DriveCommand::ReadIdentifier:
            command_set_status(CommandRejected, 0xFFFF);
            break;
        case DriveCommand::ReadMultiple:
            if(!perform_read_multiple(drive_index)) command_set_status(CommandRejected, 0xFFFF);
            else command_set_status(Success, 0x0000);
            break;
        case DriveCommand::WriteMultiple:
            command_set_status(CommandRejected, 0xFFFF);
            break;
        case DriveCommand::FormatTrack:
            command_set_status(CommandRejected, 0xFFFF);
            break;
        default:
            command_set_status(CommandRejected, 0xFFFF);
            break;
    }
}

void vme_mvme320_device::command_set_status(uint8_t main, uint16_t extended)
{
    LOGGENERAL("Updating ECA status to %02X %04X\n", main, extended);

    m_current_eca.main_status = main;
    m_current_eca.extended_status = extended;
}

/************************
 * Machine Configuration
 ************************/
void vme_mvme320_device::device_add_mconfig(machine_config &config)
{
	FLOPPY_CONNECTOR(config, m_fd0, 0);
	m_fd0->option_add("8ssdd", FLOPPY_8_SSDD);
	m_fd0->option_add("8dsdd", FLOPPY_8_DSDD);
	m_fd0->option_add("525dd", FLOPPY_525_DD);
	m_fd0->option_add("525qd", FLOPPY_525_QD);
	m_fd0->option_add("525hd", FLOPPY_525_HD);
	m_fd0->set_default_option("525qd");
	m_fd0->set_formats(floppy_image_device::default_mfm_floppy_formats);

	FLOPPY_CONNECTOR(config, m_fd1, 0);
	m_fd1->option_add("8ssdd", FLOPPY_8_SSDD);
	m_fd1->option_add("8dsdd", FLOPPY_8_DSDD);
	m_fd1->option_add("525dd", FLOPPY_525_DD);
	m_fd1->option_add("525qd", FLOPPY_525_QD);
	m_fd1->option_add("525hd", FLOPPY_525_HD);
	m_fd1->set_default_option("525qd");
	m_fd1->set_formats(floppy_image_device::default_mfm_floppy_formats);

	FLOPPY_CONNECTOR(config, m_fd2, 0);
	m_fd2->option_add("8ssdd", FLOPPY_8_SSDD);
	m_fd2->option_add("8dsdd", FLOPPY_8_DSDD);
	m_fd2->option_add("525dd", FLOPPY_525_DD);
	m_fd2->option_add("525qd", FLOPPY_525_QD);
	m_fd2->option_add("525hd", FLOPPY_525_HD);
	m_fd2->set_default_option("525qd");
	m_fd2->set_formats(floppy_image_device::default_mfm_floppy_formats);

	FLOPPY_CONNECTOR(config, m_fd3, 0);
	m_fd3->option_add("8ssdd", FLOPPY_8_SSDD);
	m_fd3->option_add("8dsdd", FLOPPY_8_DSDD);
	m_fd3->option_add("525dd", FLOPPY_525_DD);
	m_fd3->option_add("525qd", FLOPPY_525_QD);
	m_fd3->option_add("525hd", FLOPPY_525_HD);
	m_fd3->set_default_option("525qd");
	m_fd3->set_formats(floppy_image_device::default_mfm_floppy_formats);
}
