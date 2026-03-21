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
static const char FTX1_mode_type[] = { 'L', 'U', 'U', 'U', 'U', 'L', 'L', 'L', 'U', 'U', 'U', 'U', 'U', 'U', 'U', 'U', 'U', 'U' };

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

static const int FTX1_wvals_AMFM[] = { 0, WVALS_LIMIT };

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
static const char *vFTX1_nb_labels[] = { "NB off", "NB 1", "NB 2", "NB 3", "NB 4", "NB 5", "NB 6", "NB 7", "NB 8", "NB 8", "NB 10" };
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
    has_vfo_mem = true;

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

void RIG_FTX1::vfo_mem_toggle()
{
	sendCommand("VM;");
	sett("vfo_mem_toggle");
}

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


static bool in_memory_mode = false;
static int memory_channel = 0;
static std::string memory_channel_id_str;
static std::string memory_channel_tag;

std:string RIG_FTX1::get_memory_tag(const std::string memory_channel_id_str_)
{
	cmd = rsp = "MT";
	cmd = cmd + memory_channel_id_str_ + ';'; // add the memory channel number to the MT command to get the memory channel tag
	wait_char(';', 30, 100, "get_current_memory_tag", ASC);
	size_t p = replystr.rfind(rsp);
	if (p != std::string::npos) {
		memory_channel_tag = replystr.substr(p + 7, 12);
	}
	//			TRACE_STREAM(1, "get_current_memory_tag() replystr=" << replystr << ", memory_channel_tag=" << memory_channel_tag << ", memory_channel_id_str='" << memory_channel_id_str << "'");
	memory_channel_tag.erase(0, memory_channel_tag.find_first_not_of(" \t\n\r"));
	memory_channel_tag.erase(memory_channel_tag.find_last_not_of(" \t\n\r") + 1);
	//			TRACE_STREAM(1, "get_current_memory_tag() trimmed memory_channel_tag='" << memory_channel_tag << "'");
	if (memory_channel_tag.empty()) {
		memory_channel_tag = memory_channel_id_str;
		//				TRACE_STREAM(1, "get_current_memory_tag() fall back to using memory_channel_id_str=" << memory_channel_id_str);
	}
	return memory_channel_tag;
}

struct MemoryResponse {
	std::string ChannelNum;  // channel number (5 bytes)
	std::string Frequency;  // frequency (9 bytes)
	std::string Clarifier;  // clarifier (5 bytes)
	std::string RxClarifier;  // RX clarifier (1 byte)
	std::string TxClarifier;  // TX clarifier (1 byte)
	std::string Mode;  // mode (1 byte)
	std::string VfoMem;  // VFO/memory mode (1 byte)
	std::string RepeaterMode;  // repeater mode (1 byte)
	std::string Shift; // shift (1 byte)
};

bool RIG_FTX1::parse_memory_response(const std::string replystr, const size_t offset, const MemoryResponse &parsedResponse)
{
	if (p != std::string::npos) {
		// get channel number
		parsedResponse.ChannelNum = replystr.substr(p + 2, 5); // P1 = 5 bytes representing current memory channel. NOTE - the numbers get strange on Emergency channels - seeing semicolons
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
	
	// not valid response
	return false;
}

bool RIG_FTX1::get_memory_config(const std::string memory_channel_id_str_, const MemoryResponse &parsedResponse)
{
	cmd = rsp = "MR";
	cmd = cmd + memory_channel_id_str_ + ';'; // add the memory channel number to the MR command to get the memory channel config
	wait_char(';', 30, 100, "get_memory_config", ASC);
	size_t p = replystr.rfind(rsp);
	const bool parsed = parse_memory_response(replystr, p, parsedResponse)
	return parsed;
}

bool RIG_FTX1::get_current_memory(int &memory_channel_, std::string &memory_channel_tag_)
{
	int in_memory_mode_ = false;
	memory_channel_ = 0;
	memory_channel_tag = "";

	if (inuse == onA)
		cmd = rsp = "IF";
	else // onB
		cmd = rsp = "OI";

	cmd += ';';
	wait_char(';', 30, 100, "get_current_memory", ASC);

// 	sett("get_current_memory");

	size_t p = replystr.rfind(rsp);
	const MemoryResponse parsedResponse;
	const bool parsed = parse_memory_response(replystr, p, parsedResponse)
    if (p != std::string::npos) {
        memory_channel_id_str = parsedResponse.ChannelNum;
        memory_channel_ = std::stoi(memory_channel_id_str);
        char vfoMem = parsedResponse.VfoMem[0];
//         TRACE_STREAM(1, "get_current_memory() replystr=" << replystr << ", memory_channel_id_str='" << memory_channel_id_str << "', vfoMem=" << vfoMem);
        if (vfoMem != '0') {
            in_memory_mode_ = true;
        }
 	}
	
	if (in_memory_mode_) {
		memory_channel_tag = get_memory_tag(const std::string memory_channel_id_str_)
	}

 	in_memory_mode = in_memory_mode_;
 	memory_channel = memory_channel_;
	memory_channel_tag_ = memory_channel_tag;
 	return in_memory_mode_;
}

void RIG_FTX1::get_band_selection(int v)
{
	int memory_channel = 0;
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

	if (p != std::string::npos)
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
	if (p == std::string::npos) return 0;
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
	cmd = rsp = "SM0";
	cmd += ';';
	wait_char(';', 7, 100, "get smeter", ASC);

	gett("get_smeter()");

	int mtr = 0;
	sscanf(replystr.c_str(), "SM0%d", &mtr);
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
	size_t p = replystr.rfind("RM6");
	sscanf(&replystr[p], "RM6%3d%3d", &mtr, &dmy);

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
	if (p != std::string::npos) {
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
	if (p != std::string::npos) {
		sscanf(&replystr[p], "RM8%3d%3d", &mtr, &dmy);
		// initial: val = 13.8 * mtr / 190;
		val = 0.028 * mtr + 7.46; // through measurement
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

	agcval = replystr[p+3] - '0';
	if (agcval > 4) {
	  agcval = 4;
	}

//     TRACE_STREAM(1, "get_agc() replystr=" << replystr << ", agcval=" << agcval);

	return agcval;
}

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

int RIG_FTX1::incr_agc()
{
	agcval = this->next_agc();
//     TRACE_STREAM(1, "incr_agc() agcval=" << agcval);

    this->set_agc(agcval);
	return agcval;
}

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

static const char *agcstrs[] = {"AGC", "FST", "MED", "SLO", "AUT"};
const char *RIG_FTX1::agc_label()
{
	return agcstrs[agcval];
}

int  RIG_FTX1::agc_val()
{
	return (agcval);
}


bool RIG_FTX1::is_two_meter_plus()
{
    unsigned long long freq = 0;
    if (inuse == onB)
        freq = get_vfoB();
    else
        freq = get_vfoA();

    const bool two_meter_plus = freq >= 144000000ULL;
    return two_meter_plus;
}

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
	cmd = "PA00;";

	const bool two_meter_plus = is_two_meter_plus();
	if (two_meter_plus && (preamp_state > 1)) { // limit preamp for higher bands
		preamp_state = 1;
	}

	cmd[3] = '0' + preamp_state;
	sendCommand (cmd);
	showresp(WARN, ASC, "SET preamp", cmd, replystr);
}

int RIG_FTX1::get_preamp()
{
	cmd = rsp = "PA0";
	cmd += ';';
	wait_char(';', 5, 100, "get pre", ASC);

	gett("get_preamp()");

	size_t p = replystr.rfind(rsp);
	if (p != std::string::npos)
		preamp_state = replystr[p+3] - '0';
	return preamp_state;
}

static bool narrow = 0; // 0 - wide, 1 - narrow

int RIG_FTX1::adjust_bandwidth(int val)
{
	int bw = 0;
	if (val == mCW_U || val == mCW_L) {
		bandwidths_ = FTX1_widths_CW;
		bw_vals_ = FTX1_wvals_CW;
	} else if (val == mFM || val == mAM || val == mFM_N || val == mDATA_FM || val == mAM_N) {
		if (val == mFM) bandwidths_ = FTX1_widths_FMwide;
		else if (val ==  mAM) bandwidths_ = FTX1_widths_AMwide;
		else if (val == mAM_N) bandwidths_ = FTX1_widths_AMnar;
		else if (val == mFM_N) bandwidths_ = FTX1_widths_FMnar;
		else if (val == mDATA_FM) bandwidths_ = FTX1_widths_DATA_FM;
		else if (val == mDATA_FMN) bandwidths_ = FTX1_widths_DATA_FMN;
		bw_vals_ = FTX1_wvals_AMFM;
	} else if (val == mRTTY_L || val == mRTTY_U) { // RTTY
		bandwidths_ = FTX1_widths_RTTY;
		bw_vals_ = FTX1_wvals_RTTY;
	} else if (val == mDATA_L || val == mDATA_U) { // PSK
		bandwidths_ = FTX1_widths_DATA;
		bw_vals_ = FTX1_wvals_PSK;
	} else {
		bandwidths_ = FTX1_widths_SSB;
		bw_vals_ = FTX1_wvals_SSB;
	}

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
	switch (n) {
		case mCW_U: case mCW_L:
			return FTX1_widths_CW;
		case mFM:
			return FTX1_widths_FMwide;
		case mAM:
			return FTX1_widths_AMwide;
		case mAM_N :
			return FTX1_widths_AMnar;
		case mRTTY_L: case mRTTY_U:
			return FTX1_widths_RTTY;
		case mDATA_L: case mDATA_U:
			return FTX1_widths_DATA;
		case mFM_N:
			return FTX1_widths_DATA_FMN;
		case mDATA_FM:
			return FTX1_widths_DATA_FM;
		default: ;
	}
	return FTX1_widths_SSB;
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
	if (p != std::string::npos) {
		if (p + 3 < replystr.length()) {
			int md = replystr[p+3];
			int n = 0;
			for (n = 0; n < NUM_MODES; n++)
				if (md == FTX1_mode_chr[n])
					break;
			modeA = n;
		}
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
	if (p != std::string::npos) {
		if (p + 3 < replystr.length()) {
			int md = replystr[p+3];
			int n = 0;
			for (n = 0; n < NUM_MODES; n++)
				if (md == FTX1_mode_chr[n])
					break;
			modeB = n;
		}
	}
	adjust_bandwidth(modeB);
	return modeB;
}

void RIG_FTX1::set_bwA(int val)
{
	int bw_indx = bw_vals_[val];
	bwA = val;

	if (modeA == mFM || modeA == mAM || modeA == mFM_N || modeA == mDATA_FM ) {
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
	if (modeA == mFM || modeA == mAM || modeA == mFM_N || modeA == mDATA_FM) {
		bwA = 0;
		mode_bwA[modeA] = bwA;
		return bwA;
	}
	cmd = rsp = "SH0";
	cmd += ';';
	wait_char(';', 7, 100, "get bw A", ASC);

	gett("get_bwA()");

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return bwA;

	replystr[p+6] = 0;
	int bw_idx = fm_decimal(replystr.substr(p+4), 2);

	const int *idx = bw_vals_;
	int i = 0;
	while (*idx != WVALS_LIMIT) {
		if (*idx == bw_idx) break;
		idx++;
		i++;
	}
	if (*idx == WVALS_LIMIT) i = 0;
	bwA = i;
	mode_bwA[modeA] = bwA;
	return bwA;
}

void RIG_FTX1::set_bwB(int val)
{
	int bw_indx = bw_vals_[val];
	bwB = val;

	if (modeB == mFM || modeB == mAM || modeB == mFM_N || modeB == mDATA_FM) {
		mode_bwB[modeB] = 0;
		return;
	}
	cmd.clear();
	cmd.append("SH00");
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
	if (modeB == mFM || modeB == mAM || modeB == mFM_N || modeB == mDATA_FM) {
		bwB = 0;
		mode_bwB[modeB] = bwB;
		return bwB;
	}
	cmd = rsp = "SH0";
	cmd += ';';
	wait_char(';', 7, 100, "get bw B", ASC);

	gett("get_bwB()");

	size_t p = replystr.rfind(rsp);
	p = replystr.find(rsp);
	if (p == std::string::npos) return bwB;

	replystr[p+6] = 0;
	int bw_idx = fm_decimal(replystr.substr(p+4),2);

	const int *idx = bw_vals_;
	int i = 0;
	while (*idx != WVALS_LIMIT) {
		if (*idx == bw_idx) break;
		idx++;
		i++;
	}
	if (*idx == WVALS_LIMIT) i = 0;
	bwB = i;
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
	if (p == std::string::npos) return progStatus.shift;
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
	if (p == std::string::npos) return ison;

	gett("get_notch()");

	if (replystr[p+6] == '1') // manual notch enabled
		ison = true;

	val = progStatus.notch_val;
	cmd = "BP01;";
	rsp = "BP";
	wait_char(';', 8, 100, "get notch val", ASC);

	gett("get_notch_val()");

	p = replystr.rfind(rsp);
	if (p == std::string::npos)
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
	if (replystr[p+3] == '1') return 1;
	return 0;
}

// this is for setting the noise blanker NB analog level
void RIG_FTX1::set_nb_level(int val)
{
 	if (inuse == onB)
 		cmd = "NL10";
 	else
 		cmd = "NL00";

	if (nb_state < 0) {
		nb_state = 0;
	} else if (nb_state > 10) {
		nb_state = 10;
		noise_blanker_label(nb_label(), true);
	}

    char buf[3];
    std::snprintf(buf, sizeof(buf), "%02d", nb_state);
	cmd = cmd + buf + ";";

    // trace the command
    //     std::stringstream s;
    //     s << "final  nb_state=" << nb_state;
    //     set_trace(3,"set_noise", cmd.c_str(), s.str().c_str());

 	sendCommand (cmd);
 	showresp(WARN, ASC, "SET NB Level", cmd, replystr);
}

// this is for getting the noise blanker NB analog level
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
 	if (p == std::string::npos) return nb_state;

    // Parse 2 digits starting at p+4 (i.e., replystr[p+4] and replystr[p+5])
    // Example: "NL0007;" -> nb_state = 7, "NL0010;" -> nb_state = 10
    std::string stateStr = replystr.substr(4, 2);
    nb_state = std::stoi(stateStr);

// trace the command
//     std::stringstream s;
//     s << " response" << rsp << ", stateStr=" << stateStr << ", nb_state=" << nb_state;
//     set_trace(3,"get_noise", cmd.c_str(), replystr.c_str());
//     set_trace(2,"get_noise2", s.str().c_str());

 	if (nb_state) {
 	    if (nb_state > 10) {
 	        nb_state = 10;
 	    }
 		noise_blanker_label(nb_labels_[nb_state].c_str(), true);
 	} else
 		noise_blanker_label("NB", false);

 	return nb_state;
}

// this is for toggling the noise blanker (NB), each call cycles to next blanking level
void RIG_FTX1::set_noise(bool b)
 {
    // start with last level and move to next state - jump by 3's
	if (nb_state == 0) {
		nb_state = 1; // switch from off to on at level 1
		noise_blanker_label(nb_label(), true);
	} else if (nb_state < 8) {
		nb_state += 3; // bump up by 3
		noise_blanker_label(nb_label(), true);
	} else if (nb_state < 10) {
		nb_state += 1; // bump up by 1
		noise_blanker_label(nb_label(), true);
	} else {
		nb_state = 0; // switch off
		noise_blanker_label(nb_label(), false);
	}

    this->set_nb_level(nb_state);
 }

 // this is for the noise blanker NB - boolean true if on
 int RIG_FTX1::get_noise()
 {
 	gett("get_noise()");

 	int noiseLevel = this->get_nb_level();

 	return noiseLevel > 0; // return boolean for on/off
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
	if (vfo->imode == 2 || vfo->imode == 6) {
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
	progStatus.break_in = (replystr[2] == '1');
	if (progStatus.break_in) {
		break_in_label("BK-IN");
		progStatus.cw_delay = 0;
	} else {
		break_in_label("QSK ?");
//		get_qsk_delay();
	}
	return progStatus.break_in;
}

// DNR - called by NR slider
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

// DNR - NR slider value
int  RIG_FTX1::get_noise_reduction_val()
{
	int val = 0;
	cmd = rsp = "RL0";
	cmd.append(";");
	wait_char(';',6, 100, "GET noise reduction val", ASC);
	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return val;
	val = atoi(&replystr[p+3]);
	return val;
}

// DNR - called by NR toggle button
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

// DNR - value for NR toggle button
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
	for (int i = 3; i < 6; i++) {
		sqval *= 10;
		sqval += replystr[p+i] - '0';
	}
	return ceil(sqval);
}

