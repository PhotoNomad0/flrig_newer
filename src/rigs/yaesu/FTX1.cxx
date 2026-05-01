// ----------------------------------------------------------------------------
// Copyright (C) 2023
//              David Freese, W1HKJ
//
// This file is part of flrig.
//
// flrig is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// flrig is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

//////////////////////////
// based on FT-710 driver

// comment out for distribution
//#define TESTING 1

#include <iostream>
#include <sstream>
#include <cstring>

#include "yaesu/FTX1.h"
#include "debug.h"
#include "support.h"
#include "trace.h"

// use like this to trace data: `TRACE_STREAM(1, "execute_setPower()-spnrPOWER, progStatus.power_level=" << progStatus.power_level);`
#define TRACE_STREAM(level, streamExpr)                           \
    do {                                                          \
        std::ostringstream _trace_os_;                             \
        _trace_os_ << streamExpr;                                  \
        const std::string _trace_s_ = _trace_os_.str();            \
        trace((level), _trace_s_.c_str());                         \
    } while (0)


enum mFTX1 {
   mLSB, mUSB, mCW_U, mFM, mAM, mRTTY_L, mCW_L, mDATA_L, mRTTY_U, mDATA_FM, mFM_N, mDATA_U, mAM_N, mPSK, mDATA_FMN,  m_NA_G, mC4FM_N, mC4FM_VW };
//  0,    1,    2,    3,    4,    5,       6,     7,      8,       9,        10,    11 ,     12,    13,      14,      15	  16,      17   // mode index
//  1,    2,    3,    4,    5,    6,       7,     8,      9,       A,        B,     C,       D,      E		 F,       G,      H,       I    // actual value

static const char FTX1name_[] = "FTX-1";

#undef  NUM_MODES
#define NUM_MODES  18

static int defBW_narrow[NUM_MODES] = {
//  mLSB, mUSB, mCW_U, mFM, mAM, mRTTY_L, mCW_L, mDATA_L, mRTTY_U, mDATA_FM, mFM_N, mDATA_U, mAM_N, mPSK, mDATA_FMN,  m_NA_G, mC4FM_N, mC4FM_VW };
//  0,    1,    2,    3,    4,    5,       6,     7,      8,       9,        10,    11 ,     12,    13,      14,      15	  16,      17   // mode index
//  1,    2,    3,    4,    5,    6,       7,     8,      9,       A,        B,     C,       D,      E		 F,       G,      H,       I    // actual value
	6,    6,    9,    0,    0,   10,       9,     6,     10,       0,         0,     6,       0,     5,      0,       0,      0,       0,
};
static int defBW_wide[NUM_MODES] = {
//     mLSB, mUSB, mCW_U, mFM, mAM, mRTTY_L, mCW_L, mDATA_L, mRTTY_U, mDATA_FM, mFM_N, mDATA_U, mAM_N, mPSK, mDATA_FMN,  m_NA_G, mC4FM_N, mC4FM_VW };
//  0,    1,    2,    3,    4,    5,       6,     7,      8,       9,        10,    11 ,     12,    13,      14,      15	  16,      17   // mode index
//  1,    2,    3,    4,    5,    6,       7,     8,      9,       A,        B,     C,       D,      E		 F,       G,      H,       I    // actual value
	13,  13,   16,    0,    0,   10,      16,    17,     10,       0,         0,    17,       0,     9,      0,       0,      0,       0,
};

static int mode_bwA[NUM_MODES] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static int mode_bwB[NUM_MODES] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

static std::vector<std::string>FTX1modes_;
static const char *vmd[] = {
  "LSB", "USB", "CW-U", "FM", "AM",
  "RTTY-L", "CW-L", "DATA-L", "RTTY-U", "DATA-FM",
  "FM-N", "DATA-U", "AM-N", "PSK", "DATA-FMN", "-",
  "C4FM_N", "C4FM_VW" };

static const char FTX1_mode_chr[] =  { '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I' };
static const char FTX1_mode_type[] = { 'L', 'U', 'U', 'U', 'U', 'L', 'L', 'L', 'U', 'U', 'U', 'U', 'U', 'U', 'U', 'U', 'U', 'U' }; // upper or lower type

static std::vector<std::string>FTX1_widths_SSB;
static const char *vssb[] = {
 "300",  "400",  "600",  "850", "1100", 	//  1 ... 5
"1200", "1500", "1650", "1800", "1950",		//  6 ... 10
"2100", "2250", "2400", "2450", "2500",		// 11 ... 15
"2600", "2700", "2800", "2900", "3000",		// 16 ... 20
"3200", "3500", "4000" };				    // 21 ... 23

static int FTX1_wvals_SSB[] = {
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23, WVALS_LIMIT};

static std::vector<std::string>FTX1_widths_CW;
static const char *vcww[] = {
  "50",  "100",  "150",  "200",  "250",		//  1 ... 5
 "300",  "350",  "400",  "450",  "500",		//  6 ... 10
 "600",  "800", "1200", "1400", "1700",		// 11 ... 15
"2000", "2400", "3000", "3200", "3500",		// 16 .. 20
"4000" };								    // 21

static int FTX1_wvals_CW[] = {
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18, 19, 20, 21, WVALS_LIMIT };

static std::vector<std::string>FTX1_widths_RTTY;
static const char *vrtty[] = {
  "50",  "100",  "150",  "200",  "250",		//  1 ... 5
 "300",  "350",  "400",  "450",  "500",		//  6 ... 10
 "600",  "800", "1200", "1400", "1700",		// 11 ... 15
"2000", "2400", "3000", "3200", "3500",		// 16 .. 20
"4000" };								    // 21

static int FTX1_wvals_RTTY[] = {
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18, 19, 20, 21, WVALS_LIMIT };

static std::vector<std::string>FTX1_widths_DATA;
static const char *vdata[] = {
  "50",  "100",  "150",  "200",  "250",		//  1 ... 5
 "300",  "350",  "400",  "450",  "500",		//  6 ... 10
 "600",  "800", "1200", "1400", "1700",		// 11 ... 15
"2000", "2400", "3000", "3200", "3500",		// 16 .. 20
"4000" };								    // 21

static int FTX1_wvals_PSK[] = {
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18, 19, 20, 21, WVALS_LIMIT };

static const int FTX1_wvals_AMFM[] = { 0, WVALS_LIMIT }; // used for settings with only one acceptable value

static std::vector<std::string>FTX1_widths_AMwide;
static const char *vamw[] = { "9000" };
static std::vector<std::string>FTX1_widths_AMnar;
static const char *vamn[] = { "6000" };
static std::vector<std::string>FTX1_widths_FMnar;
static const char *vfmn[] = { "9000" };
static std::vector<std::string>FTX1_widths_FMwide;
static const char *vfmw[] = { "16000" };
static std::vector<std::string>FTX1_widths_DATA_FM;
static const char *vfmd[]  = { "16000" };
static std::vector<std::string>FTX1_widths_DATA_FMN;
static const char *vfmdn[] = { "9000" };

// US has 5 60M presets. Using dummy numbers for all.
// First "" means skip 60m sets in get_band_selection().
// Maybe someone can do a cat command MC; on all 5 presets and add returned numbers above.
// To send cat commands in flrig goto menu Config->Xcvr select->Send Cmd.
//
// UK has 7 60M presets. Using dummy numbers for all.  If you want support,
// Maybe someone can do a cat command MC; on all 7 presets and add returned numbers below.
// static const char *FTX1_UK_60m[] = {"", "126", "127", "128", "130", "131", "132"};

static std::vector<std::string>FTX1_US_60m;
static const char *v60m[] = {"50011", "50012", "50013", "50014", "50015"};

static std::vector<std::string>& Channels_60m = FTX1_US_60m;

//----------------------------------------------------------------------
static std::vector<std::string>FTX1_att_labels;
static const char *vFTX1_att_labels[] = { "ATT", "ATT on"};

static std::vector<std::string>FTX1_pre_labels;
static const char *vFTX1_pre_labels[] = { "IPO", "Amp 1", "Amp 2" };

static std::vector<std::string>FTX1_nb_labels;
static const char *vFTX1_nb_labels[] = { "NB off", "NB 1", "NB 2", "NB 3", "NB 4", "NB 5", "NB 6", "NB 7", "NB 8", "NB 8", "NB 10" }; // all possible values of NB
//----------------------------------------------------------------------

static GUI rig_widgets[]= {
	{ (Fl_Widget *)btnVol,        2, 125,  50 }, // 0
	{ (Fl_Widget *)sldrVOLUME,   54, 125, 368 }, // 1
	{ (Fl_Widget *)sldrRFGAIN,   54, 145, 156 }, // 2
	{ (Fl_Widget *)sldrSQUELCH, 266, 145, 156 }, // 3

	{ (Fl_Widget *)sldrMICGAIN,  54, 165, 156 }, // 4
	{ (Fl_Widget *)btnNotch,    214, 165,  50 }, // 5
	{ (Fl_Widget *)sldrNOTCH,   266, 165, 156 }, // 6

	{ (Fl_Widget *)btnNR,         2, 185,  50 }, // 7
	{ (Fl_Widget *)sldrNR,       54, 185, 156 }, // 8
	{ (Fl_Widget *)btnIFsh,     214, 185,  50 }, // 9
	{ (Fl_Widget *)sldrIFSHIFT, 266, 185, 156 }, // 10

	{ (Fl_Widget *)sldrPOWER,    54, 205, 368 }, // 11

	{ (Fl_Widget *)NULL,          0,   0,   0 }
};

void RIG_FTX1::initialize()
{
	name_ = FTX1name_;

	VECTOR(FTX1modes_, vmd);
	VECTOR(FTX1_widths_SSB, vssb);
	VECTOR(FTX1_widths_CW, vcww);
	VECTOR(FTX1_widths_RTTY, vrtty);
	VECTOR(FTX1_widths_DATA, vdata);
	VECTOR(FTX1_widths_AMwide, vamw);
	VECTOR(FTX1_widths_AMnar, vamn);
	VECTOR(FTX1_widths_FMnar, vfmn);
	VECTOR(FTX1_widths_FMwide, vfmw);
	VECTOR(FTX1_widths_DATA_FM, vfmd);
	VECTOR(FTX1_widths_DATA_FMN, vfmdn);
	VECTOR(FTX1_US_60m, v60m);

	VECTOR (FTX1_att_labels, vFTX1_att_labels);
	att_labels_ = FTX1_att_labels;

	VECTOR (FTX1_pre_labels, vFTX1_pre_labels);
	pre_labels_ = FTX1_pre_labels;

	VECTOR (FTX1_nb_labels, vFTX1_nb_labels);
	nb_labels_ = FTX1_nb_labels;

	modes_ = FTX1modes_;
	bandwidths_ = FTX1_widths_SSB;
	bw_vals_ = FTX1_wvals_SSB;

	rig_widgets[0].W = btnVol;
	rig_widgets[1].W = sldrVOLUME;
	rig_widgets[2].W = sldrRFGAIN;
	rig_widgets[3].W = sldrSQUELCH;
	rig_widgets[4].W = sldrMICGAIN;
	rig_widgets[5].W = btnNotch;
	rig_widgets[6].W = sldrNOTCH;
	rig_widgets[7].W = btnNR;
	rig_widgets[8].W = sldrNR;
	rig_widgets[9].W = btnIFsh;
	rig_widgets[10].W = sldrIFSHIFT;
	rig_widgets[11].W = sldrPOWER;


	cmd = "AI0;";
	sendCommand(cmd);
	showresp(WARN, ASC, "Auto Info OFF", cmd, replystr);
	sett("Auto Info OFF");

	set_cw_spot();

	get_vfoAorB();
}

RIG_FTX1::RIG_FTX1() {
// base class values
	IDstr = "ID";
	name_ = FTX1name_;
	modes_ = FTX1modes_;
	bandwidths_ = FTX1_widths_SSB;
	bw_vals_ = FTX1_wvals_SSB;

	widgets = rig_widgets;

	serial_baudrate = BR38400;
	stopbits = 1;
	serial_retries = 2;

	serial_write_delay = 0;
	serial_post_write_delay = 0;

	serial_timeout = 50;
	serial_rtscts = true;
	serial_rtsplus = false;
	serial_dtrplus = false;
	serial_catptt = true;
	serial_rtsptt = false;
	serial_dtrptt = false;

	A.imode = B.imode = modeB = modeA = def_mode = 1;
	A.iBW = B.iBW = bwA = bwB = def_bw = 0;
	A.freq = B.freq = freqA = freqB = def_freq = 14070000ULL;

	notch_on = false;

	has_band_selection =
	has_extras =
	has_vox_onoff =
	has_vox_gain =
	has_vox_anti =
	has_vox_hang =
	has_vox_on_dataport =

	has_cw_wpm =
	has_cw_keyer =
//	has_cw_vol =
	has_cw_spot =
//	has_cw_spot_tone = // does not exist???
	has_cw_qsk =
	has_cw_weight =
	has_cw_break_in =
	has_split =
	can_change_alt_vfo =
	has_smeter =
	has_swr_control =
	has_alc_control =

	has_idd_control =
	has_voltmeter =

	has_power_out =
	has_power_control =
	has_volume_control =
	has_rf_control =
	has_sql_control =
	has_agc_control =
	has_micgain_control =
	has_mode_control =
	has_noise_control =
	has_noise_reduction =
	has_nb_level =
	has_noise_reduction_control =
	has_bandwidth_control =
	has_notch_control =
	has_auto_notch =
	has_attenuator_control =
	has_preamp_control =
	has_ifshift_control =
	has_ptt_control =
	has_tune_control =
	has_xcvr_auto_on_off =
    has_vfo_mem =
    has_clarifier = true;

// derived specific
	atten_state = 0;
	agcval = 0;
	preamp_state = 0;
	notch_on = false;
	m_60m_indx = 0;
	m_noise_reduction_on = false;
	m_tX_output = '1'; // default to '1' for field only

	inuse = onA;

	can_synch_clock = true;

	precision = 1;
	ndigits = 9; // expand to support higher frequencies in UHF and VHF bands

}

void RIG_FTX1::set_xcvr_auto_on()
{
	cmd = "ID;";
	wait_char(';', 7 , 100, "check", ASC);
	if (replystr.find("ID") != std::string::npos)
		return;

// wait 1.2 seconds
	for (int i = 0; i < 12; i++) {
		MilliSleep(100);
		update_progress(i * 10);
		Fl::awake();
	}

	sendCommand("PS1;");
	sett("set xcvr auto ON");

	update_progress(0);

// wait up to 10 seconds for normal response
	cmd = "PS;";
	for (int i = 0; i < 100; i++) {
		wait_char(';', 4, 100, "Test for xcvr ON", ASC);
		if (replystr.find("PS1;") != std::string::npos) {
			update_progress(100);
			break;
		}
		update_progress(i);
		Fl::awake();
	}
}

void RIG_FTX1::set_xcvr_auto_off()
{
	sendCommand("PS0;");
	sett("set_xcvr_auto_off");

// transceiver does not respond after a power OFF

	for (int i = 0; i < 100; i++) {
		cmd = "PS;";
		wait_char(';', 4, 100, "Test for xcvr OFF", ASC);
		if (replystr.empty()) break;
		Fl::awake();
	}
}

/**
 * Toggles between VFO mode and Memory mode.
 *
 * This function sends the VM (VFO/Memory) command to the transceiver to toggle
 * between VFO mode and Memory mode. The command "VM;" is sent without parameters,
 * which causes the transceiver to switch between the two modes.
 *
 * In VFO mode, the transceiver operates on a variable frequency that can be tuned
 * freely within the band. In Memory mode, the transceiver recalls a stored channel
 * with preset frequency, mode, and other settings.
 *
 * @note This function does not verify if the command was successful
 * @note The actual mode after toggle depends on the current state of the transceiver
 * @note Use is_in_memory_mode() to verify the current operating mode after toggling
 */
void RIG_FTX1::vfo_mem_toggle()
{
	sendCommand("VM;");
	sett("vfo_mem_toggle");
}

/**
 * Checks if the transceiver is in memory mode.
 *
 * @return true if the transceiver is operating in memory mode (recalling a memory channel),
 *         false if in VFO mode
 *
 * This function queries the transceiver to determine its current operating mode
 * (VFO or Memory) by sending the VM (VFO/Memory) command. The command format is:
 * - "VM0;" for VFO A (when inuse == onA)
 * - "VM1;" for VFO B (when inuse == onB)
 *
 * The transceiver responds with "VMx00;" for VFO mode or "VMxnn;" for memory mode,
 * where 'x' is the VFO selector (0/1) and 'nn' is a non-zero value indicating memory mode.
 *
 * The function:
 * 1. Sends the appropriate VM command based on the active VFO
 * 2. Waits for a response in the format "VMxnn;"
 * 3. Extracts the mode indicator (2 characters starting at position p+3)
 * 4. Returns true if the mode string is not "00" (indicating memory mode)
 *
 * @note The function waits up to 100ms for a response with maximum 6 characters
 * @note Logs trace information at level 1 including the reply string, mode string, and result
 * @note There is a syntax error in the original code: extra closing parenthesis in the comparison
 */
bool RIG_FTX1::is_in_memory_mode()
{
	if (inuse == onA)
		cmd = "VM0";
	else // onB
		cmd = "VM1";

	rsp = cmd;
	cmd += ";";
	wait_char(';', 6, 100, "is_in_memory_mode()", ASC);
	size_t p = replystr.rfind(rsp);
	bool memory_mode = false;
	std::string mode_str = "";
    if (p != std::string::npos && p + 5 < replystr.length()) {
        mode_str = replystr.substr(p+3, 2);
		memory_mode = (mode_str != "00"); // Fixed: removed extra closing parenthesis
    }
    TRACE_STREAM(1, "is_in_memory_mode() replystr='" << replystr << "', mode_str='" << mode_str  << "', memory_mode=" << memory_mode);
	return memory_mode;
}

/**
 * Changes the memory channel up or down.
 *
 * @param channel_up true to increment to the next channel, false to decrement to the previous channel
 *
 * This function sends a channel change command (CH) to the transceiver to step through
 * memory channels sequentially. The command format is:
 * - "CH0;" to increment to the next higher channel
 * - "CH1;" to decrement to the next lower channel
 *
 * The function constructs the appropriate command based on the channel_up parameter,
 * sends it to the transceiver, and logs the operation using sett() with either
 * "change_channel UP" or "change_channel DOWN" for tracing purposes.
 *
 * @note This function does not verify if the command was successful
 * @note The actual channel number after the operation is not returned by this function
 */
void RIG_FTX1::change_channel(bool channel_up)
{
	cmd = channel_up ? "CH0;" : "CH1;";
	sendCommand(cmd);
	if (channel_up) {
		sett("change_channel UP");
	} else {
		sett("change_channel DOWN");
	}
}

/**
 * Starts or stops the scan operation on the transceiver.
 *
 * @param start true to start scanning, false to stop scanning
 *
 * This function controls the scan operation by sending the SC (Scan) command to the transceiver.
 * The command format varies based on which VFO is currently active:
 * - "SC0x;" for VFO A (when inuse == onA)
 * - "SC1x;" for VFO B (when inuse == onB)
 *
 * where 'x' is:
 * - '1' to start scanning
 * - '0' to stop scanning
 *
 * The function constructs the appropriate command based on the active VFO and the start parameter,
 * sends it to the transceiver, and logs the operation for tracing purposes.
 *
 * @note This function does not verify if the command was successful
 * @note The scan operation behavior depends on the transceiver's current configuration
 */
void RIG_FTX1::scan_operation(bool start)
{
	if (inuse == onA)
		cmd = "SC0";
	else // onB
		cmd = "SC1";

	const char * operation = start ? "1" : "0";

	cmd = cmd + operation + ";";
	sendCommand(cmd);
	if (start) {
		sett("scan_operation START");
	} else {
		sett("scan_operation STOP");
	}
}

/**
 * Controls the power state of the transceiver.
 *
 * @param on true to power on the transceiver, false to power off
 *
 * This function sends the PS (Power Switch) command to the transceiver to
 * control its power state. The command format is:
 * - "PS1;" to power on the transceiver
 * - "PS0;" to power off the transceiver
 *
 * The function constructs the appropriate command based on the on parameter,
 * sends it to the transceiver, and logs the operation using sett() for
 * tracing purposes with either "power on" or "power off" message.
 *
 * @note When powering on, the transceiver may take several seconds to become
 *       fully operational and respond to commands
 * @note When powering off, the transceiver will not respond to subsequent
 *       commands until powered back on
 * @note This function does not verify if the command was successful or wait
 *       for the power state transition to complete
 */
void RIG_FTX1::power(bool on)
{
	cmd = on ? "PS1;" : "PS0;";
	sendCommand(cmd);

	if (on) {
		sett("power on");
	} else {
		sett("power off");
	}
}

/**
 * Reads the dual receive (RX) mode status from the transceiver.
 *
 * @return true if dual receive mode is enabled (both VFO A and B receiving),
 *         false if single receive mode (only active VFO receiving)
 *
 * This function queries the transceiver using the FR (Function Receive) command
 * to determine if dual receive mode is active. The transceiver responds with:
 * - "FR00;" when dual receive is enabled (both VFOs receiving)
 * - "FR01;" when single receive mode (only active VFO receiving)
 *
 * The function sends "FR;" command and waits for a response in the format "FRx;"
 * where x indicates the receive mode. It parses the response to extract the mode
 * character at position 3 and returns true if it equals '0' (dual mode).
 *
 * @note The function uses ASC (ASCII) format for communication
 * @note Waits up to 100ms for a response with maximum 5 characters
 * @note Logs trace information at level 1 including the reply string and result
 */
bool RIG_FTX1::read_rx_dual()
{
	cmd = rsp = "FR";
	cmd += ";";
	wait_char(';', 5, 100, "read_rx_dual()", ASC);
	size_t p = replystr.rfind(rsp);
	bool dual = false;
    if (p != std::string::npos && p + 4 < replystr.length()) {
		dual = (replystr[p + 3] == '0');
    }
    TRACE_STREAM(1, "read_rx_dual() replystr=" << replystr << ", dual=" << dual);
	return dual;
}

/**
 * Sets the dual receive (RX) mode on the transceiver.
 *
 * @param dual true to enable dual receive mode (both VFO A and B receiving),
 *             false to enable single receive mode (only active VFO receiving)
 *
 * This function configures the transceiver's receive mode using the FR (Function Receive) command.
 * The command format is "FR0x;" where x indicates the desired mode:
 * - '0' enables dual receive mode (both VFOs receive simultaneously)
 * - '1' enables single receive mode (only the active VFO receives)
 *
 * The function constructs the appropriate command string and sends it to the transceiver
 * without waiting for a response.
 *
 * @note This function does not verify if the command was successful
 * @note The commented trace line can be uncommented for debugging purposes
 */
void RIG_FTX1::set_rx_dual(bool dual)
{
    const char rxChar = dual ? '0' : '1';
	cmd = std::string("FR0") + rxChar + ";";
	sendCommand(cmd);
	sett("set_rx_dual()");
//     TRACE_STREAM(1, "set_rx_dual() dual=" << dual << ", cmd=''" << cmd << "'', replystr=''" << replystr << "'");
}

/**
 * Reads the transmit (TX) destination setting from the transceiver.
 *
 * @return true if main-side is the TX destination,
 *         false if sub-side is the TX destination
 *
 * The function sends "FT;" command and waits for a response in the format "FTx;"
 * where x indicates the TX destination. It parses the response to extract the
 * destination character at position 2 and returns true if it equals '0' (main VFO).
 *
 * @note The function uses ASC (ASCII) format for communication
 * @note Waits up to 100ms for a response with maximum 4 characters
 * @note Logs trace information at level 1 including the reply string and result
 */
bool RIG_FTX1::read_tx_destination()
{
	cmd = rsp = "FT";
	cmd += ";";
	wait_char(';', 4, 100, "read_tx_destination()", ASC);
	size_t p = replystr.rfind(rsp);
	bool main_side = false;
    if (p != std::string::npos && p + 3 < replystr.length()) {
		main_side = (replystr[p + 2] == '0');
    }
    TRACE_STREAM(1, "read_tx_destination() replystr=" << replystr << ", main_side=" << main_side);
	return main_side;
}

/**
 * Sets the transmit (TX) destination VFO on the transceiver.
 *
 * @param main_side true to set main VFO as TX destination,
 *                  false to set sub VFO as TX destination
 *
 * This function configures which VFO (main or sub) will be used for transmission
 * using the FT (Function Transmit) command. The command format is "FTx;" where:
 * - '0' sets the main VFO as TX destination
 * - '1' sets the sub VFO as TX destination
 *
 * The function constructs the appropriate command string and sends it to the transceiver
 * without waiting for a response.
 *
 * @note This function does not verify if the command was successful
 * @note The commented trace line can be uncommented for debugging purposes
 */
void RIG_FTX1::set_tx_destination(bool main_side)
{
    const char mainChar = main_side ? '0' : '1';
    cmd = std::string("FT") + mainChar + ";";
	sendCommand(cmd);
	sett("set_tx_destination()");
//     TRACE_STREAM(1, "set_tx_destination() main_side=" << main_side << ", cmd=''" << cmd << "'', replystr=''" << replystr << "'");
}
/**
 * Removes leading and trailing whitespace from a string.
 *
 * @param str The input string to be trimmed
 * @return A new string with all leading and trailing whitespace characters removed
 *
 * This function removes spaces, tabs, newlines, and carriage returns from both
 * the beginning and end of the input string. The original string is copied and
 * modified, leaving the input unchanged.
 */
std::string trim_whitespace(const std::string& str)
{
	std::string result = str;
	result.erase(0, result.find_first_not_of(" \t\n\r"));
	result.erase(result.find_last_not_of(" \t\n\r") + 1);
	return result;
}

static bool in_memory_mode = false;
static int memory_channel = 0;
static std::string memory_channel_id_str;
static std::string memory_channel_tag;

/**
 * Converts a string to an integer with error handling.
 *
 * @param str The string to convert to an integer
 * @param deflt_value The default value to return if conversion fails (default: 0)
 * @return The converted integer value, or deflt_value if conversion fails
 *
 * This function safely converts a string to an integer using std::stoi.
 * If the conversion throws an exception (e.g., invalid format, out of range),
 * the function catches it, logs a trace message, and returns the default value
 * instead of propagating the exception.
 */
int strToI(const std::string& str, int deflt_value = 0) {
	int value = deflt_value;
	try {
		value = std::stoi(str);
	} catch (const std::exception& e) {
		TRACE_STREAM(1, "strToI() exception converting str='" << str << "', exception=" << e.what());
	} catch (...) {
		TRACE_STREAM(1, "strToI() unknown exception converting str='" << str << "'");
    }
	return value;
}

/**
 * Converts a string to an long integer with error handling.
 *
 * @param str The string to convert to an integer
 * @param deflt_value The default value to return if conversion fails (default: 0)
 * @return The converted long integer value, or deflt_value if conversion fails
 *
 * This function safely converts a string to an integer using std::stoi.
 * If the conversion throws an exception (e.g., invalid format, out of range),
 * the function catches it, logs a trace message, and returns the default value
 * instead of propagating the exception.
 */
long long strToL(const std::string& str, long long deflt_value = 0) {
	long long value = deflt_value;
	try {
		value = std::stoll(str);
	} catch (const std::exception& e) {
		TRACE_STREAM(1, "strToL() exception converting str='" << str << "', exception=" << e.what());
	} catch (...) {
		TRACE_STREAM(1, "strToL() unknown exception converting str='" << str << "'");
    }
	return value;
}

/**
 * Formats a numeric value with comma separators for thousands.
 *
 * @param value The numeric value to format
 * @return A string representation of the value with commas inserted every three digits
 *         from right to left (e.g., 1234567 becomes "1,234,567")
 *
 * This function converts a long integer to a string and inserts commas as
 * thousand separators. The commas are inserted from right to left, starting three
 * positions from the end of the string and continuing every three digits until
 * the beginning is reached.
 */
std::string format_with_commas(long long value)
{
  try {
    std::string s = std::to_string(value);
    int insertPos = static_cast<int>(s.length()) - 3;

    while (insertPos > 0) {
        s.insert(static_cast<std::string::size_type>(insertPos), ",");
        insertPos -= 3;
    }

    return s;
  } catch (...) {
//     TRACE_STREAM(1, "format_with_commas() unknown exception getting str");
  }

  // on error fall back to simple conversion
  try {
      std::string s = std::to_string(value);
      return s;
  } catch (...) {
   //     TRACE_STREAM(1, "format_with_commas() unknown exception getting str");
  }
  return "0";
}

/**
 * Retrieves the memory tag (label/description) for a given memory channel.
 *
 * @param memory_channel_id_str_ The memory channel ID as a string (e.g., "00001")
 * @return The memory tag/label for the specified channel, trimmed of whitespace.
 *         If the tag is empty, returns the memory_channel_id_str_ as a fallback.
 *
 * This function sends the MT (Memory Tag) command to the transceiver with the
 * specified channel number and parses the response to extract the 12-character
 * tag field. The tag is trimmed of leading and trailing whitespace before being
 * returned.
 */
std::string RIG_FTX1::get_memory_tag(const std::string memory_channel_id_str_, long long frequency = 0)
{
  memory_channel_tag = "";
  try {
	cmd = rsp = "MT";
	cmd = cmd + memory_channel_id_str_ + ';'; // add the memory channel number to the MT command to get the memory channel tag
	wait_char(';', 20, 100, "get_current_memory_tag", ASC);
	size_t p = replystr.rfind(rsp);
    if (p != std::string::npos && p + 19 <= replystr.length()) {
		memory_channel_tag = replystr.substr(p + 7, 12);
	//			TRACE_STREAM(1, "get_current_memory_tag() replystr=" << replystr << ", memory_channel_tag=" << memory_channel_tag << ", memory_channel_id_str='" << memory_channel_id_str << "'");
    	memory_channel_tag = trim_whitespace(memory_channel_tag);
	}

	if (
	    (frequency >  54000000 && frequency < 144000000) ||
    	(frequency > 148000000 && frequency < 420000000) ||
    	(frequency > 45000000)
     )
	{
        memory_channel_tag = format_with_commas(frequency) + " - " + memory_channel_tag;
//     	TRACE_STREAM(1, "get_current_memory_tag() undocumented frequency, memory_channel_tag='" << memory_channel_tag << "'");
	}

	//			TRACE_STREAM(1, "get_current_memory_tag() trimmed memory_channel_tag='" << memory_channel_tag << "'");
	if (memory_channel_tag.empty()) {
		std::string tag = memory_channel_id_str; // default

		long long channel_number = strToL(memory_channel_id_str_);
		if (channel_number >= 50001 && channel_number <= 50005) {
			tag = "60m ch" + std::to_string(channel_number - 50000) + " (USB)";
		} else if (channel_number >= 50006 && channel_number <= 50010) {
			tag = "60m ch" + std::to_string(channel_number - 50005) + " (CW-U)";
		} else if (channel_number >= 50011 && channel_number <= 50015) {
			tag = "60m ch" + std::to_string(channel_number - 50010) + " (DATA-U)";
		}

		memory_channel_tag = tag;
		//				TRACE_STREAM(1, "get_current_memory_tag() fall back to using memory_channel_id_str=" << memory_channel_id_str);
	}
    return memory_channel_tag;

  } catch(const std::exception& e) {
    TRACE_STREAM(1, "get_memory_tag() exception getting tag, exception=" << e.what());
  } catch (...) {
    TRACE_STREAM(1, "get_memory_tag() unknown exception getting tag");
  }
  if (!memory_channel_id_str_.empty()) {
      memory_channel_tag = memory_channel_id_str_;
  }
  return memory_channel_tag;
}

/**
 * Parses a memory response string from the transceiver.
 *
 * @param replystr The complete response string from the transceiver
 * @param offset The starting position in replystr where the memory data begins
 * @param parsedResponse Reference to MemoryResponse structure to be populated with parsed data
 * @return true if parsing was successful, false otherwise
 *
 * This function extracts memory channel configuration from the transceiver's response,
 * including channel number, frequency, clarifier settings, mode, VFO/memory status,
 * repeater mode, and shift settings. The function expects a specific format in the
 * response string with fixed-position fields.
 */
bool RIG_FTX1::parse_memory_response(const std::string replystr, const size_t offset, MemoryResponse &parsedResponse)
{
	size_t p = offset;
    if (p == std::string::npos) {
        // not valid response
        return false;
    }

    // Fixed-field parsing needs the reply to be long enough.
    // The last field read is at p + 28, length 1.
    if (p + 29 > replystr.length()) {
        // not valid response
        return false;
    }

    // get channel number
    parsedResponse.ChannelNum = replystr.substr(p + 2, 5); // P1 = 5 bytes representing current memory channel. NOTE - the numbers get strange on Emergency channels - seeing semicolons in channel #
    parsedResponse.Frequency = replystr.substr(p + 7, 9); // P1 = 5 bytes representing frequency
    parsedResponse.Clarifier = replystr.substr(p + 16, 5); // P3 = clarifier
    parsedResponse.RxClarifier = replystr.substr(p + 21, 1); // P4 - RX clarifier
    parsedResponse.TxClarifier = replystr.substr(p + 22, 1); // P5 - TX clarifier
    parsedResponse.Mode = replystr.substr(p + 23, 1); // P6 - mode

    parsedResponse.VfoMem = replystr.substr(p + 24, 1); // P7 = 0 means VFO mode, otherwise assume memory mode
    parsedResponse.RepeaterMode = replystr.substr(p + 25, 1); // P8 = repeater mode
    parsedResponse.Shift = replystr.substr(p + 28, 1); // P10 = shift
    return true;
}

/**
 * Retrieves the memory configuration for a given memory channel.
 *
 * @param memory_channel_id_str_ The memory channel ID as a string (e.g., "00001")
 * @param parsedResponse Reference to MemoryResponse structure to be populated with parsed data
 * @return true if parsing was successful, false otherwise
 *
 * This function sends the MR (Memory Read) command to the transceiver with the
 * specified channel number and parses the response to extract the memory channel
 * configuration including frequency, mode, clarifier settings, and repeater information.
 */
bool RIG_FTX1::get_memory_config(const std::string memory_channel_id_str_, MemoryResponse &parsedResponse)
{
	cmd = rsp = "MR";
	cmd = cmd + memory_channel_id_str_ + ';'; // add the memory channel number to the MR command to get the memory channel config
	wait_char(';', 30, 100, "get_memory_config", ASC);
	size_t p = replystr.rfind(rsp);
	const bool parsed = parse_memory_response(replystr, p, parsedResponse);
	return parsed;
}

/**
 * Load a range of memory channels into a list.
 *
 * Each entry contains the parsed memory configuration plus the memory tag.
 */
std::vector<MemoryResponse> RIG_FTX1::get_memory_range(int start_channel, int end_channel)
{
    std::vector<MemoryResponse> memories;
    int emptyCount = 0;

    if (start_channel > end_channel) {
        std::swap(start_channel, end_channel);
    }

    for (long long ch = start_channel; ch <= end_channel; ++ch) {
        try {
            char ch_buf[6] = {0};
            std::snprintf(ch_buf, sizeof(ch_buf), "%05lld", ch);

            MemoryResponse memory;
            if (!get_memory_config(ch_buf, memory)) {
                TRACE_STREAM(1, "get_memory_range() ch=" << ch
                     << " is empty");
                if (++emptyCount > 3) {
                    TRACE_STREAM(1, "get_memory_range() too many empty channels in a row, quitting");
                    break;
                }
                continue;
            } else {
                emptyCount = 0;
            }

            // If MemoryResponse does not already have a tag field,
            // add one in the header or store it separately.
            memory.Tag = get_memory_tag(ch_buf, ch);


            TRACE_STREAM(1, "get_memory_range() ch=" << ch
                 << ", ChannelNum=" << memory.ChannelNum
                 << ", Frequency=" << memory.Frequency
//                  << ", Clarifier=" << memory.Clarifier
//                  << ", RxClarifier=" << memory.RxClarifier
//                  << ", TxClarifier=" << memory.TxClarifier
//                  << ", Mode=" << memory.Mode
//                  << ", VfoMem=" << memory.VfoMem
//                  << ", RepeaterMode=" << memory.RepeaterMode
//                  << ", Shift=" << memory.Shift
                 << ", Tag=" << memory.Tag);


            memories.push_back(memory);
        } catch (const std::exception& e) {
            TRACE_STREAM(1, "get_memory_range() exception for ch=" << ch << ": " << e.what());
        } catch (...) {
            TRACE_STREAM(1, "get_memory_range() unknown exception for ch=" << ch);
        }
    }

    return memories;
}

/**
 * Retrieves all memory channels from the transceiver.
 *
 * @return A vector of MemoryResponse structures containing all memory channels
 *
 * This function retrieves memory channels from two ranges:
 * - Regular memory channels (1-9999)
 * - 60m channels (50000-50020)
 *
 * The function combines both ranges into a single vector and returns all
 * available memory channels with their configuration and tags.
 */
std::vector<MemoryResponse> RIG_FTX1::get_memory_channels() {
    std::vector<MemoryResponse> channels = get_memory_range(1, 9999);
    std::vector<MemoryResponse> channels2 = get_memory_range(50000, 50020);
    channels.insert(channels.end(), channels2.begin(), channels2.end());
    return channels;
}

/**
 * Retrieves the current memory channel and memory mode status.
 *
 * @param memory_channel_ Reference to store the current memory channel number (0 if in VFO mode)
 * @param memory_channel_tag_ Reference to store the memory channel tag/label
 * @return true if the transceiver is in memory mode, false if in VFO mode
 *
 * This function queries the transceiver to determine if it is operating in memory mode
 * or VFO mode. For VFO A, it uses the IF command; for VFO B, it uses the OI command.
 * If in memory mode (VfoMem != '0'), it retrieves the memory channel number and its
 * associated tag/label. The function updates both the local state variables and the
 * output parameters with the current memory configuration.
 */
bool RIG_FTX1::get_current_memory(long long &memory_channel_, std::string &memory_channel_tag_)
{
	bool in_memory_mode_result = false;
	in_memory_mode = false;
	memory_channel_ = 0;
    memory_channel_tag_.clear();

	if (inuse == onA)
		cmd = rsp = "IF";
	else // onB
		cmd = rsp = "OI";

	cmd += ';';
	const int nread = wait_char(';', 30, 100, "get_current_memory", ASC);
	if (nread <= 0 || replystr.empty()) {
		return false;
	}

// 	sett("get_current_memory");

	size_t p = replystr.rfind(rsp);
    if (p == std::string::npos) {
        return false;
    }

    MemoryResponse parsedResponse;
    if (!parse_memory_response(replystr, p, parsedResponse)) {
        return false;
    }

	if (parsedResponse.VfoMem.empty()) {
		return false;
	}

    memory_channel_id_str = parsedResponse.ChannelNum;
    memory_channel = strToL(memory_channel_id_str);
    char vfoMem = parsedResponse.VfoMem[0];
    long long freq = strToL(parsedResponse.Frequency);

//         TRACE_STREAM(1, "get_current_memory() replystr=" << replystr << ", memory_channel_id_str='" << memory_channel_id_str << "', vfoMem=" << vfoMem);

    if (vfoMem != '0') {
        in_memory_mode = true;
        memory_channel_tag = get_memory_tag(parsedResponse.ChannelNum, freq);
    }

	in_memory_mode_result = in_memory_mode;
    memory_channel_ = memory_channel;
    memory_channel_tag_ = memory_channel_tag;

	return in_memory_mode_result;
}

/**
 * Selects a specific memory channel on the transceiver.
 *
 * @param channel The memory channel number to select (1-9999 for regular channels,
 *                50000-50020 for 60m channels)
 *
 * This function sends the MC (Memory Channel) command to the transceiver to recall
 * a specific memory channel. The command format differs based on which VFO is active:
 * - MC0 for VFO A (when inuse == onA)
 * - MC1 for VFO B (when inuse == onB)
 *
 * The channel number is formatted as a 5-digit zero-padded string and appended to
 * the MC command before transmission.
 */
void RIG_FTX1::select_channel(int channel)
{
	sett("select_channel");
	if (inuse == onB)
		cmd = "MC1";
	else // onA
		cmd = "MC0";

	char ch_buf[6] = {0};
    std::snprintf(ch_buf, sizeof(ch_buf), "%05d", channel);
	cmd = cmd + ch_buf + ';';
	sendCommand(cmd);
	TRACE_STREAM(1, "select_channel() cmd=" << cmd);
}

void RIG_FTX1::get_band_selection(int v)
{
	long long memory_channel = 0;
	std::string memory_channel_tag;
	bool inc_60m = get_current_memory(memory_channel, memory_channel_tag);
	sett("get band");

	if (v == 12) {	// 5MHz 60m presets, each time it is called toggle to next channel
		if (Channels_60m[0].empty()) return;	// no 60m Channels so skip
		if (inc_60m) {
			if (++m_60m_indx >= (int)Channels_60m.size()) m_60m_indx = 0;
		}
		if (inuse == onB)
			cmd = "MC1";
		else
			cmd = "MC0";
		cmd.append(Channels_60m[m_60m_indx]).append(";");
	} else {		// v == 1..11 band selection OR return to vfo mode == 0
		if (inc_60m) {
			cmd = "VM;"; // first switch back to VFO
			sendCommand(cmd);
		}

		if (v < 3) {
			v = v - 1;
		}
		cmd.assign("BS0").append(to_decimal(v, 2)).append(";");
	}

	sendCommand(cmd);
	showresp(WARN, ASC, "Select Band Stacks", cmd, replystr);
}

//static std::string Avfo = "FA014070000;";
//static std::string Bvfo = "FB007070000;";

bool RIG_FTX1::check ()
{
#ifdef TESTING
return true;
#endif
	cmd = "ID;";
	wait_char(';', 7 , 500, "check", ASC);
//std::cout << "check: " << replystr << std::endl;

	if (replystr.find("ID") == std::string::npos)
		return false;
	return true;
}

unsigned long long RIG_FTX1::get_vfoA ()
{
	cmd = "FA";
	cmd += ';';
	wait_char(';', 12, 100, "get vfo A", ASC);
	gett("get_vfoA()");

//replystr = "XXXFA014025500;";
	unsigned long long f = 0;
	sscanf(replystr.c_str(), "FA%lld", &f);
	if (f)
		freqA = f;
	return freqA;
}

void RIG_FTX1::set_vfoA (unsigned long long freq)
{
	freqA = freq;
	cmd = "FA000000000;";
	for (int i = 10; i > 1; i--) {
		cmd[i] += freq % 10;
		freq /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vfo A", cmd, replystr);
	sett("SET vfo A");
}

unsigned long long RIG_FTX1::get_vfoB ()
{
	cmd = rsp = "FB";
	cmd += ';';
	wait_char(';', 12, 100, "get vfo B", ASC);
	gett("get_vfoB()");

//	replystr = "YYYYYFB7300000;";
	unsigned long long f = 0;
	sscanf(replystr.c_str(), "FB%lld", &f);
	if (f)
		freqB = f;
	return freqB;
}

void RIG_FTX1::set_vfoB (unsigned long long freq)
{
	freqB = freq;
	cmd = "FB000000000;";
	for (int i = 10; i > 1; i--) {
		cmd[i] += freq % 10;
		freq /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vfo B", cmd, replystr);
	sett("SET vfo B");
}


bool RIG_FTX1::twovfos()
{
	return true;
}

int RIG_FTX1::get_vfoAorB()
{
	cmd = "VS;";
	rsp = "VS";
	wait_char(';', 4, 100, "get vfoAorB()", ASC);
	gett("get vfoAorB()");
	size_t p = replystr.rfind(rsp);
    if (p != std::string::npos && p + 3 < replystr.length())
		inuse = (replystr[p + 2] == '1') ? onB : onA;
	return inuse;
}


void RIG_FTX1::selectA()
{
	cmd = "VS0;";
	sendCommand(cmd);
	showresp(WARN, ASC, "select A", cmd, replystr);
	sett("selectA()");
	inuse = onA;
}

void RIG_FTX1::selectB()
{
	cmd = "VS1;";
	sendCommand(cmd);
	showresp(WARN, ASC, "select B", cmd, replystr);
	sett("selectB()");
	inuse = onB;
}

void RIG_FTX1::A2B()
{
	cmd = "AB;";
	sendCommand(cmd);
	showresp(WARN, ASC, "vfo A --> B", cmd, replystr);
	sett("A2B()");
}

bool RIG_FTX1::can_split()
{
	return true;
}

void RIG_FTX1::set_split(bool val)
{
	split = val;
	if (val) {
		cmd = "ST1;";
		sendCommand(cmd);
		sett("Split ON");
	} else {
		cmd = "ST0;";
		sendCommand(cmd);
		sett("Split OFF");
	}
}

int RIG_FTX1::get_split()
{
	cmd = rsp = "FT";
	cmd += ";";
	wait_char(';', 4, 100, "Get split", ASC);
	gett("get split()");
	size_t p = replystr.rfind(rsp);
    if (p == std::string::npos || p + 3 >= replystr.length()) return 0;
	int split = replystr[p+2] - '0';

	return (split > 0);
}

void RIG_FTX1::swapAB()
{
	cmd = "SV;";
	sendCommand(cmd);
	sett("swapAB()");
}


int RIG_FTX1::get_smeter()
{
	if (inuse == onA)
		cmd = rsp = "SM0";
	else // onB
		cmd = rsp = "SM1";

	cmd += ';';
	wait_char(';', 7, 100, "get smeter", ASC);

	gett("get_smeter()");

	int mtr = 0;
	size_t p = replystr.rfind(rsp);
    if (p == std::string::npos || p + 6 >= replystr.length()) return 0;
    std::string searchStr = rsp + "%d";
	sscanf(replystr.c_str(), searchStr.c_str(), &mtr);
	mtr = mtr * 100.0 / 256.0;
	return mtr;
}

int RIG_FTX1::get_swr()
{
	cmd = rsp = "RM6";
	cmd += ';';
	wait_char(';', 10, 100, "get swr", ASC);

	gett("get_swr()");

	int mtr = 0, dmy = 0;
	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos || p + 9 >= replystr.length()) return 0;
	std::string searchStr = rsp + "%3d%3d";
	sscanf(&replystr[p], searchStr.c_str(), &mtr, &dmy);

	return mtr / 2.56;
}

double RIG_FTX1::get_idd()
{
	static meterpair iddtbl[] = {
		{ 52, 5.0 },
		{ 70, 7.0 },
		{ 96, 10.0 },
		{ 116, 12.0 },
		{ 125, 13.0 },
		{ 134, 14.0 },
		{ 143, 15.0 },
		{ 152, 16.0 },
		{ 161, 17.0 },
		{ 171, 18.0 },
		{ 191, 20.0 }
	};

	cmd = rsp = "RM7";
	cmd += ';';
	wait_char(';',10, 100, "get alc", ASC);
	gett("get_idd");

	int mtr = 0, dmy = 0;
	double idd = 0;
	size_t p = replystr.rfind("RM7");
	if (p != std::string::npos && p + 9 < replystr.length()) {
		sscanf(&replystr[p], "RM7%3d%3d", &mtr, &dmy);
		size_t i = 0;
		for (i = 0; i < sizeof(iddtbl) / sizeof(meterpair) - 1; i++)
			if (mtr >= iddtbl[i].mtr && mtr < iddtbl[i+1].mtr)
				break;
		if (mtr < 0) mtr = 0;
		if (mtr > 191) mtr = 191;
		idd = iddtbl[i].val +
			  (iddtbl[i+1].val - iddtbl[i].val)*(mtr - iddtbl[i].mtr) / (iddtbl[i+1].mtr - iddtbl[i].mtr);
		if (idd > 25) idd = 25;
	}
	return idd;
}

double RIG_FTX1::get_voltmeter()
{
	cmd = "RM8;";
	std::string resp = "RM";

	get_trace(1, "get_voltmeter()");
	wait_char(';',10, 100, "get vdd", ASC);
	gett("get_voltmeter");

	int mtr = 0, dmy = 0;
	double val = 0;

	size_t p = replystr.rfind("RM8");
	if (p != std::string::npos && p + 9 < replystr.length()) {
		sscanf(&replystr[p], "RM8%3d%3d", &mtr, &dmy);
		// previously: val = 13.8 * mtr / 190;
		val = 0.028 * mtr + 7.46; // determined through direct measurement
		return val;
	}

	return -1;
}


int RIG_FTX1::get_power_out()
{
	static meterpair pwrtbl[] = {
		{ 35,  5.0 },
		{ 94, 25.0 },
		{147, 50.0 },
		{176, 75.0 },
		{205,100.0 }
	};

	cmd = rsp = "RM5";
	sendCommand(cmd.append(";"));
	wait_char(';', 10, 100, "get pout", ASC);
	gett("get_power_out()");

	int mtr = 0, dmy = 0;
	size_t p = replystr.rfind("RM5");
	if (p == std::string::npos || p + 9 >= replystr.length()) return 0;

	sscanf(&replystr[p], "RM5%3d%3d", &mtr, &dmy);

	size_t i = 0;
	for (i = 0; i < sizeof(pwrtbl) / sizeof(meterpair) - 1; i++)
		if (mtr >= pwrtbl[i].mtr && mtr < pwrtbl[i+1].mtr)
			break;
	if (mtr < 0) mtr = 0;
	if (mtr > 205) mtr = 205;
	double pwr = (int)ceil(pwrtbl[i].val +
			  (pwrtbl[i+1].val - pwrtbl[i].val)*(mtr - pwrtbl[i].mtr) / (pwrtbl[i+1].mtr - pwrtbl[i].mtr));

	if (pwr > 100) pwr = 100;

	return pwr;
}

int RIG_FTX1::get_alc()
{
	cmd = rsp = "RM4";
	cmd += ';';
	wait_char(';',10, 100, "get alc", ASC);
	gett("get_alc");

	int mtr = 0, dmy = 0;
	size_t p = replystr.rfind("RM4");
	if (p == std::string::npos || p + 9 >= replystr.length()) return 0;

	sscanf(&replystr[p], "RM4%3d%3d", &mtr, &dmy);

	return (int)ceil(mtr / 2.56);
}

// Transceiver power level
double RIG_FTX1::get_power_control()
{
	cmd = rsp = "PC";
	cmd += ';';
	wait_char(';', 7, 100, "get power", ASC);

	gett("get_power_control()");

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.power_level;
	if (p + 6 >= replystr.length()) return progStatus.power_level;
	m_tX_output = replystr[p+2]; // detect if SPA-1 is attached

	int mtr = atoi(&replystr[p+3]);
	return mtr;
}

void RIG_FTX1::set_power_control(double val)
{
	int ival = (int)val;
	cmd = "PC";
    cmd += m_tX_output;   // append the output selector
    cmd += "000;";
	for (int i = 5; i > 2; i--) {
		cmd[i] += ival % 10;
		ival /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET power", cmd, replystr);
}

// Volume control return 0 ... 100
int RIG_FTX1::get_volume_control()
{
	cmd = rsp = "AG0";
	cmd += ';';
	wait_char(';', 7, 100, "get vol", ASC);

	gett("get_volume_control()");

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.volume;
	if (p + 6 >= replystr.length()) return progStatus.volume;
	int val = 0;
	sscanf(replystr.c_str(), "AG0%d", &val);
	val *= 100;
	val /= 255;
	if (val > 100) val = 100;
	return val;
}

void RIG_FTX1::set_volume_control(int val)
{
	int ivol = (int)(val * 250 / 100);
	cmd = "AG0000;";
	for (int i = 5; i > 2; i--) {
		cmd[i] += ivol % 10;
		ivol /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vol", cmd, replystr);
}

// Tranceiver PTT on/off
void RIG_FTX1::set_PTT_control(int val)
{
	cmd = val ? "TX1;" : "TX0;";
	sendCommand(cmd);
	showresp(WARN, ASC, "SET PTT", cmd, replystr);
	ptt_ = val;
}

int RIG_FTX1::get_PTT()
{
	cmd = "TX;";
	rsp = "TX";
	wait_char(';', 4, 100, "get PTT", ASC);

	gett("get_PTT()");

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return ptt_;
	if (p + 3 > replystr.length()) return ptt_;
	ptt_ =  (replystr[p+2] != '0' ? 1 : 0);
	return ptt_;
}


void RIG_FTX1::tune_rig(int val)
{
	switch (val) {
		case 0:
			cmd = "AC100;";
			break;
		case 1:
			cmd = "AC101;";
			break;
		case 2:
		default:
			cmd = "AC103;";
			break;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "tune rig", cmd, replystr);
	sett("tune_rig");
}

int RIG_FTX1::get_tune()
{
	cmd = rsp = "AC";
	cmd += ';';
	wait_char(';', 5, 100, "get tune", ASC);

	rig_trace(2, "get_tuner status()", replystr.c_str());

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return 0;
	if (p + 4 >= replystr.length()) return 0;
	if (replystr[p+4] == '0') return 0;
	return 1;
}

int  RIG_FTX1::next_attenuator()
{
	switch (atten_state) {
		case 0: return 1;
		case 1: return 0;
	}
	return 0;
}

void RIG_FTX1::set_attenuator(int val)
{
	atten_state = val;
	if (val) {
    	atten_state = 1; // sanity limit
	}
	cmd = "RA00;";
	cmd[3] += atten_state;
	sendCommand(cmd);
	showresp(WARN, ASC, "SET att", cmd, replystr);
}

int RIG_FTX1::get_attenuator()
{
	cmd = rsp = "RA0";
	cmd += ';';
	wait_char(';', 5, 100, "get att", ASC);

	gett("get_attenuator()");

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.attenuator;
	if (p + 5 >= replystr.length()) return progStatus.attenuator;
	atten_state = replystr[p+3] - '0';
	return atten_state;
}

/**
 * Retrieves the AGC (Automatic Gain Control) setting for the current VFO.
 *
 * @return The current AGC level (0-4):
 *         - 0: AGC (default/auto)
 *         - 1: FST (fast)
 *         - 2: MED (medium)
 *         - 3: SLO (slow)
 *         - 4: AUT (auto)
 *
 * This function queries the transceiver for the AGC setting using the GT command.
 * The command format varies based on which VFO is currently active:
 * - "GT0;" for VFO A (when inuse != onB)
 * - "GT1;" for VFO B (when inuse == onB)
 *
 * The transceiver responds with "GTxn;" where:
 * - 'x' is the VFO selector (0 for VFO A, 1 for VFO B)
 * - 'n' is the AGC level (0-4)
 *
 * The function parses the response to extract the AGC value and ensures it
 * doesn't exceed the maximum value of 4.
 *
 * @note The function waits up to 100ms for a response with maximum 5 characters
 * @note Returns the previous agcval if parsing fails
 */
int RIG_FTX1::get_agc()
{
	if (inuse == onB)
		cmd = rsp = "GT1";
	else
		cmd = rsp = "GT0";

	cmd += ';';
	wait_char(';', 5, 100, "get agc", ASC);

	gett("get_agc()");

	size_t p = replystr.rfind(rsp);
    if (p == std::string::npos) return agcval;
	if (p + 3 >= replystr.length()) return agcval;

	agcval = replystr[p+3] - '0';
	if (agcval > 4) {
	  agcval = 4;
	}

//     TRACE_STREAM(1, "get_agc() replystr=" << replystr << ", agcval=" << agcval);

	return agcval;
}

/**
 * Calculates the next AGC (Automatic Gain Control) level in sequence.
 *
 * @return The next AGC level (0-4):
 *         - Returns 1 if current level is 0 or less (wrap from off to first level)
 *         - Returns current level + 1 if current level is less than 4
 *         - Returns 0 if current level is 4 or higher (wrap to off)
 *
 * This function implements a circular progression through AGC levels:
 * 0 -> 1 -> 2 -> 3 -> 4 -> 0 -> ...
 *
 * The progression corresponds to these AGC settings:
 * - 0: AGC (default/auto)
 * - 1: FST (fast)
 * - 2: MED (medium)
 * - 3: SLO (slow)
 * - 4: AUT (auto)
 *
 * @note This function only calculates the next value; it does not apply it to the transceiver
 * @note Use incr_agc() to both calculate and apply the next AGC level
 */
int RIG_FTX1::next_agc()
{
    int new_agc = 0;

    if (agcval <= 0) {
      new_agc = 1;
    } else if (agcval < 4) {
      new_agc =  agcval + 1;
    }
//     TRACE_STREAM(1, "next_agc() initial agcval=" << agcval << ", new_agc=" << new_agc);
    return new_agc;
}

/**
 * Increments the AGC level to the next setting and applies it to the transceiver.
 *
 * @return The new AGC level after incrementing (0-4)
 *
 * This function combines the calculation of the next AGC level with immediate
 * application to the transceiver. It performs a three-step operation:
 * 1. Calls next_agc() to determine the next AGC level in the sequence
 * 2. Updates the internal agcval state variable with the new level
 * 3. Sends the new AGC setting to the transceiver via set_agc()
 *
 * The AGC levels cycle through the sequence: 0 -> 1 -> 2 -> 3 -> 4 -> 0
 * corresponding to: AGC -> FST -> MED -> SLO -> AUT -> AGC
 *
 * This is typically called when the user clicks an AGC increment button or
 * similar UI control that steps through AGC settings.
 *
 * @note The new AGC value is both stored internally and transmitted to the radio
 * @note This is a convenience function that combines next_agc() and set_agc()
 */
int RIG_FTX1::incr_agc()
{
	agcval = this->next_agc();
//     TRACE_STREAM(1, "incr_agc() agcval=" << agcval);

    this->set_agc(agcval);
	return agcval;
}

/**
 * Sets the AGC (Automatic Gain Control) level on the transceiver.
 *
 * @param val The desired AGC level (0-4):
 *            - 0: AGC (default/auto)
 *            - 1: FST (fast)
 *            - 2: MED (medium)
 *            - 3: SLO (slow)
 *            - 4: AUT (auto)
 *
 * This function configures the AGC setting using the GT (Gain Time) command.
 * The command format varies based on which VFO is currently active:
 * - "GT0n;" for VFO A (when inuse != onB)
 * - "GT1n;" for VFO B (when inuse == onB)
 *
 * where 'n' is the AGC level character ('0' through '4').
 *
 * The function clamps the input value to the maximum of 4 to prevent invalid
 * settings. The clamped value is stored in the agcval member variable and
 * transmitted to the transceiver.
 *
 * @note Values greater than 4 are automatically clamped to 4
 * @note The function does not verify if the command was successful
 */
void RIG_FTX1::set_agc(int val)
{
	if (inuse == onB)
		cmd = rsp = "GT1";
	else
		cmd = rsp = "GT0";

//     TRACE_STREAM(1, "set_agc() val=" << val);

	agcval = val;
	if (val > 4) {
    	agcval = 4; // sanity limit
	}
    cmd += static_cast<char>('0' + agcval);
    cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET agc", cmd, replystr);
}

/**
 * Returns the human-readable label for the current AGC setting.
 *
 * @return A C-string pointer to the AGC mode label:
 *         - "AGC" for level 0 (default/auto)
 *         - "FST" for level 1 (fast)
 *         - "MED" for level 2 (medium)
 *         - "SLO" for level 3 (slow)
 *         - "AUT" for level 4 (auto)
 *
 * This function provides a text representation of the current AGC mode for
 * display in the user interface. The returned string is a static constant
 * from the agcstrs array and should not be modified or freed.
 *
 * @note The function uses the current agcval member variable as the index
 * @note No bounds checking is performed; ensure agcval is in range 0-4
 */
static const char *agcstrs[] = {"AGC", "FST", "MED", "SLO", "AUT"};
const char *RIG_FTX1::agc_label()
{
	return agcstrs[agcval];
}

/**
 * Returns the current AGC level value.
 *
 * @return The current AGC level (0-4):
 *         - 0: AGC (default/auto)
 *         - 1: FST (fast)
 *         - 2: MED (medium)
 *         - 3: SLO (slow)
 *         - 4: AUT (auto)
 *
 * This is a simple accessor function that returns the internally stored
 * AGC value without querying the transceiver. Use get_agc() if you need
 * to retrieve the current setting from the radio.
 *
 * @note This returns the cached value in agcval, not a fresh read from the radio
 */
int  RIG_FTX1::agc_val()
{
	return (agcval);
}

const unsigned long long VHF = 144000000ULL;
const unsigned long long UHF = 400000000ULL;

/**
 * Retrieves the frequency of the currently active VFO.
 *
 * @return The frequency in Hz as an unsigned long long integer
 *
 * This function determines which VFO (A or B) is currently in use and retrieves
 * its frequency. The active VFO is determined by the 'inuse' member variable:
 * - If inuse == onB: Returns the frequency from VFO B
 * - Otherwise: Returns the frequency from VFO A (default)
 *
 * The returned frequency value is in Hz and represents the current operating
 * frequency of the transceiver on the active VFO.
 */
unsigned long long RIG_FTX1::getFreqForCurrentVfo()
{
    unsigned long long freq = 0;
    if (inuse == onB)
        freq = get_vfoB();
    else
        freq = get_vfoA();
    return freq;
}

/**
 * Checks if the current VFO frequency is in the VHF band or higher (≥144 MHz).
 *
 * @return true if the current VFO frequency is 144 MHz or above (VHF/UHF/higher bands),
 *         false if below 144 MHz (HF bands)
 *
 * This function retrieves the frequency from the currently active VFO (A or B) and
 * compares it against the VHF threshold constant (144 MHz). This check is commonly
 * used to determine:
 * - Available preamplifier options (VHF/UHF bands have different preamp configurations)
 * - Band-specific feature availability
 * - Frequency-dependent operating constraints
 *
 * The VHF constant is defined as 144000000 Hz (144 MHz), which is the traditional
 * boundary between HF and VHF amateur radio bands.
 */
bool RIG_FTX1::is_two_meter_plus()
{
    unsigned long long freq = getFreqForCurrentVfo();
    const bool two_meter_plus = freq >= VHF;
    return two_meter_plus;
}

/**
 * Determines the appropriate preamplifier range based on the current VFO frequency.
 *
 * @return The preamplifier range index:
 *         - 0: HF bands (below 144 MHz) - standard preamp range
 *         - 1: VHF band (144-400 MHz) - VHF preamp range
 *         - 2: UHF band (400 MHz and above) - UHF preamp range
 *
 * This function retrieves the current frequency from the active VFO and determines
 * which preamplifier range should be used based on the band:
 * - Frequencies below 144 MHz (VHF threshold) use range 0
 * - Frequencies from 144 MHz to 400 MHz (UHF threshold) use range 1
 * - Frequencies at or above 400 MHz use range 2
 *
 * The preamplifier range affects which preamp settings are available and how
 * the transceiver configures its front-end amplification circuitry for optimal
 * performance in each frequency band.
 */
int RIG_FTX1::get_range_for_preamp()
{
    int preamp_range = 0;
    unsigned long long freq = getFreqForCurrentVfo();
    if (freq >= VHF) {
        if (freq >= UHF) {
            preamp_range = 2;
        } else { // VHF
            preamp_range = 1;
        }
    }
    return preamp_range;
}

/**
 * Determines the next preamp state based on current state and frequency band.
 *
 * @return The next preamp state value (0, 1, or 2)
 *
 * This function cycles through available preamp (preamplifier) states based on:
 * - Current preamp_state value (0 = IPO/off, 1 = Amp 1, 2 = Amp 2)
 * - Operating frequency band (VHF/UHF bands have limited preamp options)
 *
 * Behavior:
 * - State 0 (IPO): Always transitions to state 1 (Amp 1)
 * - State 1 (Amp 1):
 *   - For VHF/UHF bands (≥144 MHz): Returns to state 0 (only one amp level available)
 *   - For HF bands: Transitions to state 2 (Amp 2)
 * - State 2 or higher: Returns to state 0 (IPO)
 *
 * The function checks if the current frequency is in the VHF/UHF range using
 * is_two_meter_plus(), which determines whether only one amplifier level is
 * available instead of two.
 */
int  RIG_FTX1::next_preamp()
{
    const bool two_meter_plus = is_two_meter_plus();

	switch (preamp_state) {
		case 0: return 1;
		case 1:
            if (two_meter_plus) { // there is only one level of amplifier in this case
                return 0;
            } else {
		        return 2;
            }
		default: return 0;
	}
	return 0;
}

void RIG_FTX1::set_preamp(int val)
{
	preamp_state = val;
    const int preamp_range = get_range_for_preamp();

	cmd = "PA00;";

	if (preamp_range > 0) {
      cmd[2] = '0' + preamp_range;

	  if (preamp_state > 1) { // limit preamp for higher bands
		preamp_state = 1;
	  }
	}

	cmd[3] = '0' + preamp_state;
	sendCommand (cmd);
	showresp(WARN, ASC, "SET preamp", cmd, replystr);
}

int RIG_FTX1::get_preamp()
{
	const int preamp_range = get_range_for_preamp();

	cmd = "PA0";
    if (preamp_range > 0) {
      cmd[2] = '0' + preamp_range;
    }

    rsp = cmd;
    cmd += ';';

	wait_char(';', 4, 100, "get pre", ASC);

	gett("get_preamp()");

	size_t p = replystr.rfind(rsp);
	if (p != std::string::npos && p + 3 < replystr.length())
		preamp_state = replystr[p+3] - '0';
	return preamp_state;
}

static bool narrow = 0; // 0 - wide, 1 - narrow

/**
 * Retrieves bandwidth configuration data for a given mode.
 *
 * @param mode The operating mode for which to retrieve bandwidth data (e.g., mCW_U, mLSB, mFM)
 * @param bandwidths Reference to a pointer that will be set to the appropriate bandwidth labels vector
 * @param bw_vals Reference to a pointer that will be set to the appropriate bandwidth values array
 *
 * This function maps operating modes to their corresponding bandwidth options. Each mode has
 * specific bandwidth choices available:
 * - CW modes (CW-U, CW-L): Narrow bandwidths from 50Hz to 4000Hz
 * - AM/FM modes: Fixed bandwidths (wide/narrow variants)
 * - RTTY modes: Similar to CW bandwidth options
 * - DATA modes: PSK-style bandwidth options
 * - SSB modes (LSB, USB): Wide range from 300Hz to 4000Hz
 *
 * The function modifies the input pointer references to point to the appropriate static
 * vectors and arrays containing bandwidth labels and values for the specified mode.
 */
void RIG_FTX1::get_bandwidth_data(const int mode, std::vector<std::string>& bandwidths, const int *&bw_vals)
{
	switch (mode) {
		case mCW_U:
		case mCW_L:
            bandwidths = FTX1_widths_CW;
            bw_vals = FTX1_wvals_CW;
			break;

		case mAM:
			bandwidths = FTX1_widths_AMwide;
		    bw_vals = FTX1_wvals_AMFM;
			break;

		case mAM_N:
			bandwidths = FTX1_widths_AMnar;
			bw_vals = FTX1_wvals_AMFM;
			break;

		case mFM:
			bandwidths = FTX1_widths_FMwide;
			bw_vals = FTX1_wvals_AMFM;
			break;

		case mFM_N:
			bandwidths = FTX1_widths_FMnar;
			bw_vals = FTX1_wvals_AMFM;
			break;

		case mDATA_FM:
		case mC4FM_N:
		case mC4FM_VW:
			bandwidths = FTX1_widths_DATA_FM;
			bw_vals = FTX1_wvals_AMFM;
			break;

		case mDATA_FMN:
			bandwidths = FTX1_widths_DATA_FMN;
			bw_vals = FTX1_wvals_AMFM;
			break;

		case mRTTY_L:
		case mRTTY_U:
            bandwidths = FTX1_widths_RTTY;
            bw_vals = FTX1_wvals_RTTY;
			break;

		case mDATA_L:
		case mDATA_U:
		case mPSK:
            bandwidths = FTX1_widths_DATA;
            bw_vals = FTX1_wvals_PSK;
			break;

		case mLSB:
		case mUSB:
		default:
            bandwidths = FTX1_widths_SSB;
            bw_vals = FTX1_wvals_SSB;
			break;
	}

    int count = 0;
    while (bw_vals[count] != WVALS_LIMIT) {
        ++count;
    }
//     TRACE_STREAM(1, "get_bandwidth_data() mode=" << mode << ", str='" << FTX1modes_[mode] << "', bandwidths.len=" << bandwidths.size() << ", bw_vals.len=" << count);
}

/**
 * Adjusts the bandwidth index for a given mode.
 *
 * @param val The operating mode for which to adjust bandwidth (e.g., mCW_U, mLSB, mFM)
 * @return The bandwidth index appropriate for the mode and current narrow/wide setting
 *
 * This function retrieves the bandwidth configuration data for the specified mode
 * and returns the default bandwidth index based on the current narrow/wide state.
 * The narrow/wide state is determined by the global 'narrow' variable:
 * - When narrow is true (1): Returns the narrow bandwidth index from defBW_narrow[]
 * - When narrow is false (0): Returns the wide bandwidth index from defBW_wide[]
 *
 * The function also updates the class member variables bandwidths_ and bw_vals_
 * to point to the appropriate bandwidth tables for the specified mode.
 */
int RIG_FTX1::adjust_bandwidth(int val)
{
//     TRACE_STREAM(1, "adjust_bandwidth() val=" << val );

	int bw = 0;
	get_bandwidth_data(val, bandwidths_, bw_vals_);

	if (narrow)
		bw = defBW_narrow[val];
	else
		bw = defBW_wide[val];

	return bw;
}

int RIG_FTX1::def_bandwidth(int m)
{
	int bw = adjust_bandwidth(m);
	if (inuse == onB) {
		if (mode_bwB[m] == -1)
			mode_bwB[m] = bw;
		return mode_bwB[m];
	}
	if (mode_bwA[m] == -1)
		mode_bwA[m] = bw;
	return mode_bwA[m];
}

std::vector<std::string>& RIG_FTX1::bwtable(int n)
{
//     TRACE_STREAM(1, "bwtable() mode=" << n );

    get_bandwidth_data(n, bandwidths_, bw_vals_);
    return bandwidths_;
}

/**
 * Sets the RX clarifier state (on/off) for the current VFO.
 *
 * @param on true to enable RX clarifier, false to disable
 *
 * This function controls the receive clarifier state using the CF (Clarifier) command.
 * The command format varies based on which VFO is currently active:
 * - "CF000x0000;" for VFO A (when inuse != onB)
 * - "CF100x0000;" for VFO B (when inuse == onB)
 *
 * where 'x' is:
 * - '1' to enable RX clarifier
 * - '0' to disable RX clarifier
 *
 * The function constructs the appropriate command based on the active VFO and the on parameter,
 * sends it to the transceiver, and logs the operation for tracing purposes.
 *
 * @note This function does not verify if the command was successful
 * @note The clarifier offset value is set to "0000" (no offset) in this command
 */
void RIG_FTX1::set_rx_clarifier_state(bool on)
{
	if (inuse == onB)
		cmd = rsp = "CF100";
	else
		cmd = rsp = "CF000";
    const char state = !on ? '0' : '1';
	cmd += state;
	cmd += "0000;";
	sendCommand(cmd);
	showresp(WARN, ASC, "set_rx_clarifier_state", cmd, replystr);
//     TRACE_STREAM(1, "set_rx_clarifier_state(): cmd=" << cmd << ", replystr=" << replystr << ", on=" << on);
}

/**
 * Retrieves the RX clarifier state for the current VFO.
 *
 * @return true if RX clarifier is enabled, false if disabled
 *
 * This function queries the transceiver to determine if the receive clarifier is active
 * by sending the CF (Clarifier) command. The command format varies based on which VFO
 * is currently active:
 * - "CF000;" for VFO A (when inuse != onB)
 * - "CF100;" for VFO B (when inuse == onB)
 *
 * The transceiver responds with "CFx0y0000;" where:
 * - 'x' is the VFO selector (0 for VFO A, 1 for VFO B)
 * - 'y' is the clarifier state ('0' = enabled, '1' = disabled)
 * - The remaining digits represent the clarifier offset value
 *
 * The function parses the response to extract the state character at position 5 and
 * returns true if it equals '0' (clarifier enabled).
 *
 * @note The function waits up to 100ms for a response with maximum 11 characters
 * @note Returns false if parsing fails or clarifier is disabled
 */
bool RIG_FTX1::get_rx_clarifier_state()
{
	if (inuse == onB)
		cmd = rsp = "CF100";
	else
		cmd = rsp = "CF000";
	cmd += ';';
	wait_char(';', 11, 100, "get_rx_clarifier_state", ASC);

	gett("get_rx_clarifier_state()");

	bool clarifier_on = false;

	size_t p = replystr.rfind(rsp);
	if (p != std::string::npos && p + 10 < replystr.length()) {
		char state = replystr[p+5];
		clarifier_on = state != '0';
//         TRACE_STREAM(1, "get_rx_clarifier_state(): clarifier_on=" << clarifier_on << ", replystr=" << replystr << ", state=" << state);
	}
	return clarifier_on;
}

/**
 * Sets the RX clarifier offset value for the current VFO.
 *
 * @param level The clarifier offset in Hz, range -9999 to +9999
 *
 * This function sets the receive clarifier offset using the CF (Clarifier) command.
 * The command format varies based on which VFO is currently active:
 * - "CF001±nnnn;" for VFO A (when inuse != onB)
 * - "CF101±nnnn;" for VFO B (when inuse == onB)
 *
 * where:
 * - '±' is the sign character ('+' for positive, '-' for negative offsets)
 * - 'nnnn' is the absolute offset value as a 4-digit zero-padded decimal number
 *
 * The level parameter is clamped to the range -9999 to +9999 Hz. The function
 * converts the signed value to sign-magnitude format for transmission.
 *
 * @note This function does not verify if the command was successful
 * @note The offset is applied to the receive frequency only
 */
void RIG_FTX1::set_rx_clarifier_value(int level)
{
    int val = level;
	if (inuse == onB)
		cmd = rsp = "CF101";
	else
		cmd = rsp = "CF001";

    if (val > 9999) {
        val = 9999;
    } else if (val < -9999) {
       val = -9999;
    }

    char sign = '+';
    if (val < 0) {
        sign = '-';
        val = -val;
    }
    cmd += sign;

    char level_str[5];
    std::snprintf(level_str, sizeof(level_str), "%04d", val);

	cmd += level_str;
	cmd += ';';
// 	TRACE_STREAM(1, "set_rx_clarifier_value(): level=" << level << ", cmd=" << cmd);
	sendCommand(cmd);
	showresp(WARN, ASC, "set_rx_clarifier_value", cmd, replystr);
}

/**
 * Retrieves the RX clarifier offset value for the current VFO.
 *
 * @return The clarifier offset in Hz, range -9999 to +9999
 *
 * This function queries the transceiver for the current receive clarifier offset
 * by sending the CF (Clarifier) command. The command format varies based on which
 * VFO is currently active:
 * - "CF001;" for VFO A (when inuse != onB)
 * - "CF101;" for VFO B (when inuse == onB)
 *
 * The transceiver responds with "CFx01±nnnn;" where:
 * - 'x' is the VFO selector (0 for VFO A, 1 for VFO B)
 * - '±' is the sign character at position 5 ('+' or '-')
 * - 'nnnn' is the absolute offset value starting at position 6
 *
 * The function parses the response to extract the sign and magnitude, then
 * combines them to return the signed offset value.
 *
 * @note The function waits up to 100ms for a response with maximum 11 characters
 * @note Returns 0 if parsing fails or if the clarifier offset is not set
 */
int RIG_FTX1::get_rx_clarifier_value()
{
	if (inuse == onB)
		cmd = rsp = "CF101";
	else
		cmd = rsp = "CF001";
	cmd += ';';
	wait_char(';', 11, 100, "get_rx_clarifier_value", ASC);

	gett("get_rx_clarifier_value()");

	int clarifier_value = 0;

	size_t p = replystr.rfind(rsp);
	if (p != std::string::npos && p + 10 < replystr.length()) {
		int val = atoi(&replystr[p+6]);
    	if (replystr[p+5] == '-') val = -val;
        clarifier_value = val;

        TRACE_STREAM(1, "get_rx_clarifier_value() replystr='" << replystr << "', val =" << val);
	}
	return clarifier_value;
}

void RIG_FTX1::set_modeA(int val)
{
	modeA = val;
	if (inuse == onB)
		cmd = rsp = "MD1";
	else
		cmd = rsp = "MD0";
	cmd += FTX1_mode_chr[val];
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET mode A", cmd, replystr);
	adjust_bandwidth(modeA);
}

int RIG_FTX1::get_modeA()
{
	if (inuse == onB)
		cmd = rsp = "MD1";
	else
		cmd = rsp = "MD0";
	cmd += ';';
	wait_char(';', 5, 100, "get mode A", ASC);

	gett("get_modeA()");

	size_t p = replystr.rfind(rsp);
	if (p != std::string::npos && p + 3 < replystr.length()) {
		int md = replystr[p+3];
		int n = 0;
		for (n = 0; n < NUM_MODES; n++)
			if (md == FTX1_mode_chr[n])
				break;
		modeA = n;
	}
	adjust_bandwidth(modeA);
	return modeA;
}

void RIG_FTX1::set_modeB(int val)
{
	modeB = val;
	if (inuse == onA)
		cmd = rsp = "MD1";
	else
		cmd = rsp = "MD0";
	cmd += FTX1_mode_chr[val];
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET mode B", cmd, replystr);
	adjust_bandwidth(modeA);
}

int RIG_FTX1::get_modeB()
{
	if (inuse == onA)
		cmd = rsp = "MD1";
	else
		cmd = rsp = "MD0";
	cmd += ';';
	wait_char(';', 5, 100, "get mode B", ASC);

	gett("get_modeB()");

	size_t p = replystr.rfind(rsp);
	if (p != std::string::npos && p + 4 < replystr.length()) {
        int md = replystr[p+3];
        int n = 0;
        for (n = 0; n < NUM_MODES; n++)
            if (md == FTX1_mode_chr[n])
                break;
        modeB = n;
	}
	adjust_bandwidth(modeB);
	return modeB;
}

/**
 * Parses bandwidth index from transceiver reply string and maps it to the UI bandwidth index.
 *
 * @param mode The operating mode (e.g., mLSB, mCW_U, mFM) to determine which bandwidth table to use
 * @param prefix The command prefix to search for in the reply string (e.g., "SH0", "SH1")
 * @param bw_out Reference parameter that will be set to the resolved bandwidth index (0-based UI index)
 * @return The bandwidth index on success, or -1 if parsing fails
 *
 * This function extracts the bandwidth index from a transceiver reply string and converts it from
 * the radio's internal bandwidth value to the corresponding UI bandwidth index. The process involves:
 *
 * 1. Locating the command prefix in replystr (e.g., "SH0" for VFO A bandwidth)
 * 2. Extracting the 2-digit bandwidth value at offset +4 from the prefix
 * 3. Loading the appropriate bandwidth tables for the given mode
 * 4. Searching the bandwidth values array (bw_vals_) to find a matching value
 * 5. Returning the array index corresponding to that value
 *
 * Example reply string: "SH00013;" where:
 * - "SH0" is the prefix (VFO A bandwidth)
 * - "13" at position [p+4, p+5] is the bandwidth index from the radio
 * - The function maps this to the UI index (e.g., 13 -> index 12 in the bandwidth array)
 *
 * If the bandwidth value from the radio is not found in the expected table (hits WVALS_LIMIT),
 * the function defaults to index 0 (first/narrowest bandwidth). This can occur when:
 * - The noise blanker state affects available bandwidths
 * - The radio reports an unexpected/unsupported bandwidth value
 *
 * @note The function modifies replystr by null-terminating at position p+6
 * @note Returns -1 if: prefix not found, reply string too short, or parsing fails
 */
int RIG_FTX1::parse_bw_index_from_reply(int mode, const std::string& prefix, int &bw_out)
{
    size_t p = replystr.rfind(prefix);
    if (p == std::string::npos) return -1;
    if (p + 6 >= replystr.length()) return -1;

    replystr[p+6] = 0;
    int bw_idx = fm_decimal(replystr.substr(p+4), 2);
    get_bandwidth_data(mode, bandwidths_, bw_vals_);

    const int *idx = bw_vals_;
    int i = 0;
    while (*idx != WVALS_LIMIT) {
        if (*idx == bw_idx) break;
        idx++;
        i++;
    }
    if (*idx == WVALS_LIMIT){
//         TRACE_STREAM(1, "parse_bw_index_from_reply() hit limit looking for bw_idx='" << bw_idx << "', i ='" << i << ", nb_state=" << nb_state);

//         std::ostringstream bw_vals_dump;
//         bw_vals_dump << "bw_vals_ contents: [";
//         const int *dump_idx = bw_vals_;
//         bool first = true;
//         while (*dump_idx != WVALS_LIMIT) {
//             if (!first) bw_vals_dump << ", ";
//             bw_vals_dump << *dump_idx;
//             first = false;
//             dump_idx++;
//         }
//         bw_vals_dump << "]";
//         TRACE_STREAM(1, "parse_bw_index_from_reply() " << bw_vals_dump.str());

        i = 0; // default to first
    }

    bw_out = i;
    return i;
}

/**
 * Determines if a mode supports only one bandwidth option.
 *
 * @param mode The operating mode to check (e.g., mAM, mFM, mDATA_FM)
 * @return true if the mode supports only one bandwidth, false otherwise
 *
 * This function checks whether a given operating mode has a single fixed
 * bandwidth option. The following modes support only one bandwidth:
 * - AM (mAM): Fixed at 9000 Hz
 * - AM Narrow (mAM_N): Fixed at 6000 Hz
 * - FM (mFM): Fixed at 16000 Hz
 * - FM Narrow (mFM_N): Fixed at 9000 Hz
 * - DATA FM (mDATA_FM): Fixed at 16000 Hz
 * - DATA FM Narrow (mDATA_FMN): Fixed at 9000 Hz
 * - C4FM Narrow (mC4FM_N): Fixed bandwidth
 * - C4FM Voice Wide (mC4FM_VW): Fixed bandwidth
 *
 * All other modes (SSB, CW, RTTY, DATA, PSK) support multiple bandwidth options.
 */
bool RIG_FTX1::onlyOneBwSupported(int mode) const
{
    return mode == mAM || mode == mAM_N || mode == mFM || mode == mFM_N || mode == mDATA_FM || mode == mDATA_FMN || mode == mC4FM_N || mode == mC4FM_VW;
}

void RIG_FTX1::set_bwA(int val)
{
	int bw_indx = bw_vals_[val];
	bwA = val;

	if (onlyOneBwSupported(modeA)) {
		return;
	}
	cmd.clear();
	cmd.append("SH00");
	cmd += '0' + bw_indx / 10;
	cmd += '0' + bw_indx % 10;
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET bw A", cmd, replystr);
	sett("SET bwA");
	mode_bwA[modeA] = val;
}

int RIG_FTX1::get_bwA()
{
	if (onlyOneBwSupported(modeA)) {
		bwA = 0;
		mode_bwA[modeA] = bwA;
		return bwA;
	}
	cmd = rsp = "SH0";
	cmd += ';';
	wait_char(';', 7, 100, "get bw A", ASC);

	gett("get_bwA()");

    if (parse_bw_index_from_reply(modeA, rsp, bwA) < 0) return bwA;

	mode_bwA[modeA] = bwA;
	return bwA;
}

void RIG_FTX1::set_bwB(int val)
{
	int bw_indx = bw_vals_[val];
	bwB = val;

	if (onlyOneBwSupported(modeB)) {
		mode_bwB[modeB] = 0;
		return;
	}
	cmd.clear();
	cmd.append("SH10");
	cmd += '0' + bw_indx / 10;
	cmd += '0' + bw_indx % 10;
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET bw B", cmd, replystr);
	sett("SET bwB");
	mode_bwB[modeB] = bwB;
}

int RIG_FTX1::get_bwB()
{
	if (onlyOneBwSupported(modeB)) {
		bwB = 0;
		mode_bwB[modeB] = bwB;
		return bwB;
	}
	cmd = rsp = "SH1";
	cmd += ';';
	wait_char(';', 7, 100, "get bw B", ASC);

	gett("get_bwB()");

    if (parse_bw_index_from_reply(modeB, rsp, bwB) < 0) return bwB;

	mode_bwB[modeB] = bwB;
	return bwB;
}

std::string RIG_FTX1::get_BANDWIDTHS()
{
	std::stringstream s;
	for (int i = 0; i < NUM_MODES; i++)
		s << mode_bwA[i] << " ";
	for (int i = 0; i < NUM_MODES; i++)
		s << mode_bwB[i] << " ";
	return s.str();
}

void RIG_FTX1::set_BANDWIDTHS(std::string s)
{
	std::stringstream strm;
	strm << s;
	for (int i = 0; i < NUM_MODES; i++)
		strm >> mode_bwA[i];
	for (int i = 0; i < NUM_MODES; i++)
		strm >> mode_bwB[i];
}

int RIG_FTX1::get_modetype(int n)
{
	return FTX1_mode_type[n];
}

void RIG_FTX1::set_if_shift(int val)
{
	if (inuse == onB)
		cmd = "IS10+0000;";
	else
		cmd = "IS00+0000;";
	if (val != 0) progStatus.shift = true;
	else progStatus.shift = false;
	if (val < 0) cmd[4] = '-';
	val = abs(val);
	for (int i = 8; i > 4; i--) {
		cmd[i] += val % 10;
		val /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET if shift", cmd, replystr);
}

bool RIG_FTX1::get_if_shift(int &val)
{
	cmd = rsp = "IS0";
	cmd += ';';
	wait_char(';', 10, 100, "get if shift", ASC);

	gett("get_if_shift()");

	size_t p = replystr.rfind(rsp);
	val = progStatus.shift_val;
	if (p == std::string::npos || p + 8 >= replystr.length()) return progStatus.shift;
	val = atoi(&replystr[p+5]);
	if (replystr[p+4] == '-') val = -val;
	return (val != 0);
}

void RIG_FTX1::get_if_min_max_step(int &min, int &max, int &step)
{
	if_shift_min = min = -1200;
	if_shift_max = max = 1200;
	if_shift_step = step = 20;
	if_shift_mid = 0;
}

/*
BPabcde;
a: Fixed, '0'

b: Manual NOTCH ON/OFF, 1/0

cde: 001 - 320, (NOTCH Frequency : x 10 Hz )
*/
static std::string notch_str_on  = "BP00001;";
static std::string notch_str_off = "BP00000;";
static std::string notch_str_val = "BP01000;";
static int notch_val = 1500;

void RIG_FTX1::set_notch(bool on, int val)
{
	if (notch_val != val) {
		cmd = notch_str_on;
		sendCommand(cmd);
		showresp(WARN, ASC, "SET notch ON", cmd, replystr);
		set_trace(3,"set_notch ON", cmd.c_str(), replystr.c_str());
// set notch frequency
		notch_val = val;
		val /= 10;
		for (int i = 0; i < 3; i++) {
			notch_str_val[6 - i] = '0' + (val % 10);
			val /= 10;
		}
		cmd = notch_str_val;
// set notch ON
		sendCommand(cmd);
		showresp(WARN, ASC, "SET notch val", cmd, replystr);
		set_trace(3,"set_notch val", cmd.c_str(), replystr.c_str());
	}
	if (on)
		cmd = notch_str_on;
	else
		cmd = notch_str_off;
	sendCommand(cmd);
	set_trace(3,"set_notch OFF", cmd.c_str(), replystr.c_str());
	showresp(WARN, ASC, "SET notch OFF", cmd, replystr);

}

bool  RIG_FTX1::get_notch(int &val)
{
	bool ison = false;

	cmd = "BP00;";
	rsp = "BP";
	wait_char(';', 8, 100, "get notch on/off", ASC);
	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos || p + 6 >= replystr.length()) return ison;

	gett("get_notch()");

	if (replystr[p+6] == '1') // manual notch enabled
		ison = true;

	val = progStatus.notch_val;
	cmd = "BP01;";
	rsp = "BP";
	wait_char(';', 8, 100, "get notch val", ASC);

	gett("get_notch_val()");

	p = replystr.rfind(rsp);
	if (p == std::string::npos || p + 7 >= replystr.length())
		val = 10;
	else
		val = fm_decimal(replystr.substr(p+4), 3) * 10;

	return (notch_on = ison);
}

void RIG_FTX1::get_notch_min_max_step(int &min, int &max, int &step)
{
	min = 10;
	max = 3200;
	step = 10;
}

void RIG_FTX1::set_auto_notch(int v)
{
	if (inuse == onB)
		cmd = "BC10;";
	else
		cmd = "BC00;";
	if (v) cmd[3] = '1';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET auto notch", cmd, replystr);
}

int  RIG_FTX1::get_auto_notch()
{
	cmd = "BC0;";
	wait_char(';', 5, 100, "get auto notch", ASC);

	gett("get_auto_notch()");

	size_t p = replystr.rfind("BC");
	if (p == std::string::npos) return 0;
	if (p + 3 >= replystr.length()) return 0;
	if (replystr[p+3] == '1') return 1;
	return 0;
}

std::string currentLabel = ""; // singleton to save current label

/**
 * Retrieves the noise blanker (NB) label based on current state.
 *
 * @return A C-string pointer to the noise blanker label:
 *         - Returns the label from nb_labels_ vector corresponding to nb_state
 *         - If nb_state is 0, returns label for level 0 (typically "NB off")
 *         - Returns "NB" as a fallback if any exception occurs during lookup
 *
 * This function provides a safe way to access noise blanker labels, handling
 * potential out-of-range errors gracefully by catching exceptions and returning
 * a default "NB" label.
 */
const char *RIG_FTX1::nb_label() {
    try {
        int level = nb_level;
        if (nb_state == 0) {
            level = 0;
        }

        currentLabel = nb_labels_.at(level);
//         TRACE_STREAM(1, "nb_label() newLabel=''" << currentLabel << "'', nb_level='" << nb_level << ", nb_state=" << nb_state);

        return currentLabel.c_str();
    } catch (...) {
        return "NB";
    }
}

/**
 * Sets the noise blanker (NB) analog level.
 *
 * @param val The desired NB level (0 to 10)
 *
 * This function configures the noise blanker level by combining the level value
 * with the current nb_state to send to the transceiver. The command format is:
 * - "NL00nn;" for VFO A (when inuse != onB)
 * - "NL10nn;" for VFO B (when inuse == onB)
 *
 * where 'nn' is the level value as a 2-digit zero-padded decimal number.
 *
 * The function performs the following operations:
 * 1. Clamps the input value to the valid range (0-10)
 * 2. Stores the clamped value in nb_level
 * 3. If nb_state is 0 (off) or the level is 0, sets newVal to 0 and updates the UI label to "NB" (inactive)
 * 4. Otherwise, uses the requested level and updates the UI label to show the current NB setting (active)
 * 5. Formats the level as a 2-digit string and constructs the NL command
 * 6. Sends the command to the transceiver
 *
 * @note Values less than 0 are clamped to 0
 * @note Values greater than 10 are clamped to 10
 * @note The noise_blanker_label() function is called to update the UI display
 * @note When nb_state is 0, the level is forced to 0 regardless of the input value
 */
void RIG_FTX1::set_nb_level(int val) // 0 to 10
{
    nb_level = val;
	if (nb_level < 0) {
		nb_level = 0;
	} else if (nb_level > 10) {
		nb_level = 10;
	}
	int newVal = nb_level;

    if (inuse == onB)
        cmd = "NL10";
    else
        cmd = "NL00";

    if (nb_state == 0 || newVal <= 0) {
        newVal = 0;
        noise_blanker_label("NB", false);
    } else {
        noise_blanker_label(nb_label(), true);
    }

    char buf[3];
    std::snprintf(buf, sizeof(buf), "%02d", newVal);
    cmd = cmd + buf + ";";

//     trace the command
//     std::stringstream s;
//     s << "final  nb_state=" << nb_state << ", nb_level=" << nb_level << ", parameter val=" << val;
//     set_trace(3,"set_nb_level", cmd.c_str(), s.str().c_str());

    sendCommand (cmd);
    showresp(WARN, ASC, "SET NB Level", cmd, replystr);
}


/**
 * Retrieves the noise blanker (NB) analog level from the transceiver.
 *
 * @return The current NB level (0-10), or nb_state if parsing fails
 *
 * This function queries the transceiver for the current noise blanker level
 * by sending the NL (Noise Level) command. The command format is:
 * - "NL0;" for VFO A (when inuse != onB)
 * - "NL1;" for VFO B (when inuse == onB)
 *
 * The transceiver responds with "NLxnn;" where:
 * - 'x' is the VFO selector (0 for VFO A, 1 for VFO B)
 * - 'nn' is the NB level as a 2-digit decimal number (00-10)
 *
 * The function performs the following operations:
 * 1. Sends the NL query command for the active VFO
 * 2. Parses the 2-digit level value from the response
 * 3. Clamps the level to maximum of 10 if needed
 * 4. Updates nb_level with the current value
 * 5. If level > 0: Sets nb_state to 1 (on) if it was 0, updates UI label to show active NB
 * 6. If level == 0: Sets nb_state to 0 (off), ensures nb_level has a valid saved value (≥1), updates UI label to show inactive NB
 *
 * The function maintains the distinction between nb_level (the saved level setting)
 * and nb_state (whether NB is currently on/off). When NB is toggled on, it uses
 * the last saved nb_level value.
 *
 * @note The function waits up to 100ms for a response with maximum 7 characters
 * @note Returns nb_state if parsing fails
 * @note Performs sanity checks to ensure nb_level is at least 1 when NB is off
 */
int RIG_FTX1::get_nb_level()
{
  	if (inuse == onB)
  		rsp = "NL1";
  	else
  		rsp = "NL0";

 	cmd = rsp;
 	cmd += ';';
 	wait_char(';', 7, 100, "get NB Level", ASC);

 	gett("get_nb_level()");

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos || p + 6 >= replystr.length()) return nb_state;

    // Parse 2 digits starting at p+4 (i.e., replystr[p+4] and replystr[p+5])
    // Example: "NL0007;" -> nb_state = 7, "NL0010;" -> nb_state = 10
	std::string stateStr = replystr.substr(p + 4, 2);
    int level = strToI(stateStr);

//     TRACE_STREAM(1, "get_nb_level() replystr='" << replystr << "', level=" << level);

 	if (level > 0) {
 	    if (level > 10) {
 	        level = 10;
 	    }

 	    nb_level = level; // save current value
 		noise_blanker_label(nb_label(), true);

        if (nb_state == 0) {
            nb_state = 1;  // if greater than zero NB is actually on
//             TRACE_STREAM(1, "get_nb_level() level greater than 0, forcing nb_state on, nb_level=" << nb_level << ", nb_state=" << nb_state);
        } else {
//             TRACE_STREAM(1, "get_nb_level() level greater than 0, nb_level=" << nb_level << ", nb_state=" << nb_state);
        }
 	} else { // not greater than zero, so NB currently off
 	    level = 0;
        // if NB toggled on we continue to use the last saved value for nb_level

     	if (nb_state != 0) { // saved nb_state was on, but actually is off
            nb_state = 0;
//             TRACE_STREAM(1, "get_nb_level() level 0 so using previous nb_level and nb_state forced on, nb_level=" << nb_level << ", nb_state=" << nb_state);
     	} else {
//             TRACE_STREAM(1, "get_nb_level() level 0 so using previous nb_level, nb_state already off, nb_level=" << nb_level << ", nb_state=" << nb_state);
     	}
        if (nb_level < 1) { // sanity check
            nb_level = 1;
//             TRACE_STREAM(1, "get_nb_level() sanity check for nb_level, setting to 1, nb_level=" << nb_level << ", nb_state=" << nb_state);
        }
 		noise_blanker_label("NB", false);
    }

 	return nb_level;
}

/**
 * Sets the noise blanker (NB) state (on/off).
 *
 * @param b true to enable noise blanker, false to disable
 *
 * This function controls the noise blanker state by updating nb_state and
 * sending the appropriate level command to the transceiver. The function
 * manages the interaction between the on/off state and the level setting:
 *
 * When b is false (turning NB off):
 * - Sets nb_state to 0
 * - Updates the UI label to "NB" (inactive)
 * - Sends level 0 to the transceiver via set_nb_level()
 *
 * When b is true (turning NB on):
 * - Sets nb_state to 1
 * - Ensures level is at least 1 (performs sanity check)
 * - Sends the current nb_level to the transceiver via set_nb_level()
 *
 * The function preserves the last used nb_level value when toggling NB on/off,
 * so that re-enabling NB restores the previous level setting rather than
 * defaulting to a fixed value.
 *
 * @note The actual command transmission to the transceiver is handled by set_nb_level()
 * @note The nb_level value is preserved when toggling off, allowing it to be restored when toggling on
 * @note If nb_level is less than 1 when enabling NB, it is automatically set to 1
 */
void RIG_FTX1::set_noise(bool b) // b==0 is off
 {
     int level = nb_level;

	if (b == 0) { // b is off
	    if (nb_state == 0) {
//             TRACE_STREAM(1, "set_noise(" << b <<") nb_state already off, nb_level=" << nb_level << ", nb_state=" << nb_state);
	    } else {
//             TRACE_STREAM(1, "set_noise(" << b <<") nb_state was on so toggling off, nb_level=" << nb_level << ", nb_state=" << nb_state);
		    nb_state = 0;
	    }
		noise_blanker_label("NB", false);
	} else { // b is on
        if (nb_state == 0) {
            nb_state = 1;
//             TRACE_STREAM(1, "set_noise(" << b <<") nb_state was off so toggling on, nb_level=" << nb_level << ", nb_state=" << nb_state);
        } else {
//             TRACE_STREAM(1, "set_noise(" << b <<") nb_state already on, nb_level=" << nb_level << ", nb_state=" << nb_state);
        }

       if (level < 1) { // sanity check
            level = 1; // has to be at least 1 for NB to be on
//             TRACE_STREAM(1, "set_noise() sanity check for nb_level, setting to 1, nb_level=" << nb_level << ", nb_state=" << nb_state);
        }
	}

    this->set_nb_level(level); // send new level (and nb_state) to radio
 }

/**
 * Retrieves the noise blanker (NB) state.
 *
 * @return The NB state: 1 if on, 0 if off
 *
 * This function queries the transceiver for the current noise blanker state
 * by calling get_nb_level(), which updates both nb_level and nb_state based
 * on the transceiver's response. The function then returns the nb_state value.
 *
 * The nb_state represents whether the noise blanker is currently active:
 * - 0: Noise blanker is off
 * - 1: Noise blanker is on
 *
 * @note This function is a wrapper around get_nb_level() that extracts only the on/off state
 * @note The actual level value can be retrieved separately using get_nb_level()
 */
 int RIG_FTX1::get_noise()
 {
 	gett("get_noise()");
 	this->get_nb_level(); // get current level and update nb_state
 	return nb_state; // return 0 for off and 1 for on
 }

// val 0 .. 100
void RIG_FTX1::set_mic_gain(int val)
{
	cmd = "MG000;";
	for (int i = 3; i > 0; i--) {
		cmd[1+i] += val % 10;
		val /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET mic", cmd, replystr);
}

int RIG_FTX1::get_mic_gain()
{
	cmd = rsp = "MG";
	cmd += ';';
	wait_char(';', 6, 100, "get mic", ASC);

	gett("get_mic_gain()");

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.mic_gain;
	if (p + 2 >= replystr.length()) return progStatus.mic_gain;
	int val = atoi(&replystr[p+2]);
	return val;
}

void RIG_FTX1::get_mic_min_max_step(int &min, int &max, int &step)
{
	min = 0;
	max = 100;
	step = 1;
}

void RIG_FTX1::set_rf_gain(int val)
{
	cmd = "RG0000;";
	int rfval = val * 250 / 100;
	for (int i = 5; i > 2; i--) {
		cmd[i] = rfval % 10 + '0';
		rfval /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET rfgain", cmd, replystr);
}

int  RIG_FTX1::get_rf_gain()
{
	int rfval = 0;
	cmd = rsp = "RG0";
	cmd += ';';
	wait_char(';', 7, 100, "get rfgain", ASC);

	gett("get_rf_gain()");

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.rfgain;
	if (p + 6 > replystr.length()) return progStatus.rfgain;
	for (int i = 3; i < 6; i++) {
		rfval *= 10;
		rfval += replystr[p+i] - '0';
	}
	rfval = rfval * 100 / 250;
	if (rfval > 100) rfval = 100;
	return rfval;
}

void RIG_FTX1::get_rf_min_max_step(int &min, int &max, int &step)
{
	min = 0;
	max = 100;
	step = 1;
}

void RIG_FTX1::set_vox_onoff()
{
	cmd = "VX0;";
	if (progStatus.vox_onoff) cmd[2] = '1';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vox", cmd, replystr);
}

void RIG_FTX1::set_vox_gain()
{
	cmd = "VG";
	cmd.append(to_decimal(progStatus.vox_gain, 3)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vox gain", cmd, replystr);
}

void RIG_FTX1::set_vox_anti()
{
}

void RIG_FTX1::set_vox_hang()
{
	cmd = "VD";
	cmd.append(to_decimal(progStatus.vox_hang, 4)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vox delay", cmd, replystr);
}

void RIG_FTX1::set_vox_on_dataport()
{
    cmd = "EX0305100;";
	if (progStatus.vox_on_dataport) cmd[8] = '1';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vox on data port", cmd, replystr);
}

void RIG_FTX1::set_cw_wpm()
{
	cmd = "KS";
	if (progStatus.cw_wpm > 60) progStatus.cw_wpm = 60;
	if (progStatus.cw_wpm < 4) progStatus.cw_wpm = 4;
	cmd.append(to_decimal(progStatus.cw_wpm, 3)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET cw wpm", cmd, replystr);
}


void RIG_FTX1::enable_keyer()
{
	cmd = "KR0;";
	if (progStatus.enable_keyer) cmd[2] = '1';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET keyer on/off", cmd, replystr);
}

bool RIG_FTX1::set_cw_spot()
{
	if (vfo && (vfo->imode == 2 || vfo->imode == 6)) {
		cmd = "CS0;";
		if (progStatus.spot_onoff) cmd[2] = '1';
		sendCommand(cmd);
		showresp(WARN, ASC, "SET spot on/off", cmd, replystr);
		return true;
	} else
		return false;
}

void RIG_FTX1::set_cw_weight()
{
	int n = round(progStatus.cw_weight * 10);
	cmd.assign("EX020203").append(to_decimal(n, 2)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET cw weight", cmd, replystr);
}

void RIG_FTX1::set_cw_qsk()
{
	int n = progStatus.cw_qsk / 5 - 3;
	cmd.assign("EX020117").append(to_decimal(n, 1)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET cw qsk", cmd, replystr);
}

void RIG_FTX1::set_break_in()
{
	if (progStatus.break_in) {
		cmd = "BI1;";
		break_in_label("BK-IN");
	} else {
		cmd = "BI0;";
		break_in_label("QSK ?");
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET break in on/off", cmd, replystr);
	sett("set_break_in");
}

int RIG_FTX1::get_break_in()
{
	cmd = "BI;";
	wait_char(';', 4, 100, "get break in", ASC);
	if (replystr.length() > 2) {
		progStatus.break_in = (replystr[2] == '1');
	} else {
    	progStatus.break_in = false;
	}
	if (progStatus.break_in) {
		break_in_label("BK-IN");
		progStatus.cw_delay = 0;
	} else {
		break_in_label("QSK ?");
//		get_qsk_delay();
	}
	return progStatus.break_in;
}

/**
 * Sets the noise reduction value for the transceiver.
 *
 * @param val The noise reduction level (0-15, where 0 = off)
 *
 * This function configures the Digital Noise Reduction (DNR) level using the RL0 command.
 * The command format is "RL0nn;" where 'nn' is the noise reduction level as a 2-digit
 * zero-padded decimal number.
 *
 * If the noise reduction is toggled off (m_noise_reduction_on is false), the function
 * forces the value to 0 regardless of the input parameter, ensuring the DNR is disabled.
 *
 * The function constructs the RL0 command with the appropriate level value, sends it to
 * the transceiver, and logs the operation for debugging purposes.
 *
 * @note This function is called by the NR slider control in the UI
 * @note The actual value sent depends on the m_noise_reduction_on state
 * @note Valid range is typically 0-15, where higher values provide more noise reduction
 */ 
void RIG_FTX1::set_noise_reduction_val(int val)
{
    if (!m_noise_reduction_on) {
        val = 0; // if NR button is toggled off, make sure off
    }
	cmd.assign("RL0").append(to_decimal(val, 2)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET_noise_reduction_val", cmd, replystr);
	sett("set_noise_reduction_val");
}

/**
 * Retrieves the current noise reduction level from the transceiver.
 *
 * @return The current noise reduction level (0-15, where 0 = off)
 *
 * This function queries the transceiver for the current Digital Noise Reduction (DNR)
 * level by sending the RL0 command. The transceiver responds with "RL0nn;" where 'nn'
 * is the noise reduction level as a 2-digit decimal number.
 *
 * The function parses the response string to extract the numeric value at positions
 * [p+3, p+4] where p is the position of "RL0" in the reply. If parsing fails or the
 * response is invalid, the function returns 0.
 *
 * This value represents the slider position in the UI and is used to synchronize the
 * UI state with the transceiver's actual setting.
 *
 * @note This function is called by the NR slider control to read the current setting
 * @note The function waits up to 100ms for a response with maximum 6 characters
 * @note Returns 0 if the response cannot be parsed or NR is disabled
 */
int RIG_FTX1::get_noise_reduction_val()
{
	int val = 0;
	cmd = rsp = "RL0";
	cmd.append(";");
	wait_char(';',6, 100, "GET noise reduction val", ASC);
	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos || p + 5 >= replystr.length()) return val;
	val = atoi(&replystr[p+3]);
	return val;
}

/**
 * Sets the noise reduction state (on/off) with intelligent level management.
 *
 * @param val The desired state: 0 to disable noise reduction, non-zero to enable
 *
 * This function controls the Digital Noise Reduction (DNR) on/off state using the RL0
 * command, with special logic to manage the interaction between the toggle button and
 * the level slider:
 *
 * When enabling NR (val > 0):
 * 1. Sets m_noise_reduction_on to true
 * 2. Queries the current NR level from the transceiver
 * 3. If already non-zero (NR already on), returns immediately without sending a command
 * 4. If currently zero, sends RL001; to enable NR at minimum level (1)
 *
 * When disabling NR (val == 0):
 * 1. Sets m_noise_reduction_on to false
 * 2. Sends RL000; to disable NR
 *
 * This approach prevents unnecessary commands when NR is already in the desired state
 * and ensures that enabling NR always starts at a visible level (1) rather than staying
 * at zero.
 *
 * @note This function is called by the NR toggle button control in the UI
 * @note The function preserves the user's selected level when toggling on/off
 * @note Minimum NR level when enabling is 1 to provide immediate feedback
 */
void RIG_FTX1::set_noise_reduction(int val)
{
    int newValue = val;
    m_noise_reduction_on = val > 0;
    if (m_noise_reduction_on) { // if user selected NR on
        newValue = get_noise_reduction_val();
        if (newValue > 0) { // check if already on
            return; // nothing to do, already on
        }

        newValue = 1; // if user wants to switch on, start at minimum
    }

    cmd.assign("RL0").append(to_decimal(newValue, 2)).append(";");
    sendCommand(cmd);
	showresp(WARN, ASC, "SET noise reduction", cmd, replystr);
	sett("set_noise_reduction_on/off");
}

/**
 * Retrieves the noise reduction on/off state from the transceiver.
 *
 * @return 1 if noise reduction is enabled (level > 0), 0 if disabled
 *
 * This function determines the Digital Noise Reduction (DNR) on/off state by querying
 * the current noise reduction level from the transceiver. The NR is considered "on"
 * if the level value is greater than zero, and "off" if the level is zero.
 *
 * The function calls get_noise_reduction_val() to retrieve the current level and
 * returns a boolean-style integer (0 or 1) representing the state. This value is
 * used to synchronize the NR toggle button state in the UI.
 *
 * @note This function is called by the NR toggle button control to read the current state
 * @note The threshold for "on" is any value greater than 0 (level 1-15)
 * @note Returns 0 only when the noise reduction level is exactly 0
 */
int  RIG_FTX1::get_noise_reduction()
{
	return get_noise_reduction_val() > 0 ? 1 : 0; // any value but zero is on
}

// ---------------------------------------------------------------------
// set date and time
// ---------------------------------------------------------------------
// dt formated as YYYYMMDD
// ---------------------------------------------------------------------
void RIG_FTX1::sync_date(char *dt)
{
    if (!dt) return;

	cmd.assign("DT0");
	cmd.append(dt);
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "sync_date", cmd, replystr);
	sett("sync_date");
}

// ---------------------------------------------------------------------
// tm formated as HH:MM:SS
// ---------------------------------------------------------------------
void RIG_FTX1::sync_clock(char *tm)
{
    if (!tm || std::strlen(tm) < 8) return;

	cmd.assign("DT1");
	cmd += tm[0]; cmd += tm[1];
	cmd += tm[3]; cmd += tm[4];
	cmd += tm[6]; cmd += tm[7];
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "sync_time", cmd, replystr);
	sett("sync_time");
}

void RIG_FTX1::set_squelch(int val)
{
	cmd = "SQ0000;";
	for (int i = 5; i > 2; i--) {
		cmd[i] = val % 10 + '0';
		val /= 10;
	}

	set_trace(1, "set_squelch()");
	sendCommand(cmd);
	sett("");
	showresp(WARN, ASC, "SET squelch", cmd, replystr);
}

int  RIG_FTX1::get_squelch()
{
	int sqval = 0;
	cmd = rsp = "SQ0";
	cmd += ';';
	get_trace(1, "get_squelch()");
	wait_char(';',7, 100, "get squelch", ASC);
	gett("");

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.squelch;
	if (p + 6 >= replystr.length()) return progStatus.squelch;
	for (int i = 3; i < 6; i++) {
		sqval *= 10;
		sqval += replystr[p+i] - '0';
	}
	return ceil(sqval);
}

