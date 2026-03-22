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

#ifndef _FTX1_H
#define _FTX1_H

#include "rigbase.h"

class RIG_FTX1 : public rigbase {
private:
	bool notch_on;
	int  m_60m_indx;
	bool  m_noise_reduction_on;
	char m_tX_output; // will be either '1' for field only, or '2' for SPA-1 attached

public:
	RIG_FTX1();
	~RIG_FTX1() {}

	virtual void initialize();

	virtual bool check();

	virtual unsigned long long get_vfoA();
	virtual void set_vfoA(unsigned long long);

	virtual unsigned long long get_vfoB();
	virtual void set_vfoB(unsigned long long);

	virtual int get_vfoAorB();

	virtual bool twovfos();
	virtual void selectA();
	virtual void selectB();
	virtual void A2B();
	virtual bool can_split();
	virtual void set_split(bool val);
	virtual int  get_split();

	virtual void swapAB();
	virtual bool canswap() { return true; }

	virtual void set_modeA(int val);
	virtual int  get_modeA();
	virtual int  get_modetype(int n);

	virtual void set_modeB(int val);
	virtual int  get_modeB();

	virtual void set_bwA(int val);
	virtual int  get_bwA();

	virtual void set_bwB(int val);
	virtual int  get_bwB();

	virtual int  adjust_bandwidth(int val);
	virtual int  def_bandwidth(int val);

	virtual void set_BANDWIDTHS(std::string s);
	virtual std::string get_BANDWIDTHS();

	virtual int  get_smeter();
	virtual int  get_swr();
	virtual int  get_alc();
	virtual double get_idd();
	virtual double get_voltmeter();

	virtual int  get_power_out();
	virtual double get_power_control();
	virtual void set_power_control(double val);
	virtual void get_pc_min_max_step(double &min, double &max, double &step) {
		min = 5; pmax = max = 100; step = 1; }

	void set_squelch(int val);
	int  get_squelch();
	void get_squelch_min_max_step(int &min, int &max, int &step) {
		min = 0; max = 100; step = 5; }

	virtual void set_volume_control(int val);
	virtual int  get_volume_control();
	virtual void set_PTT_control(int val);
	virtual int  get_PTT();
	virtual void tune_rig(int);
	virtual int  get_tune();

	virtual int  next_attenuator();
	virtual void set_attenuator(int val);
	virtual int  get_attenuator();

    virtual int  get_agc();
    virtual void set_agc(int val);
    virtual int  next_agc();
    virtual int  incr_agc();
    virtual int  agc_val();
    virtual const char *  agc_label();

	virtual int  next_preamp();
	virtual void set_preamp(int val);
	virtual int  get_preamp();
    virtual bool is_two_meter_plus();
	virtual void set_if_shift(int val);
	virtual bool get_if_shift(int &val);
	virtual void get_if_min_max_step(int &min, int &max, int &step);

	virtual void set_notch(bool on, int val);
	virtual bool get_notch(int &val);
	virtual void get_notch_min_max_step(int &min, int &max, int &step);

	virtual void set_auto_notch(int v);
	virtual int  get_auto_notch();

	virtual void set_noise(bool b);
	virtual int  get_noise();
//	void get_nb_min_max_step(int &min, int &max, int &step) {
//		min = 1; max = 10; step = 3; }
	void set_nb_level(int val);
	int  get_nb_level();

	virtual void set_mic_gain(int val);
	virtual int  get_mic_gain();
	virtual void get_mic_min_max_step(int &min, int &max, int &step);

	virtual void set_rf_gain(int val);
	virtual int  get_rf_gain();
	virtual void get_rf_min_max_step(int &min, int &max, int &step);
	virtual std::vector<std::string>& bwtable(int);

	virtual void set_vox_onoff();
	virtual void set_vox_gain();
	virtual void set_vox_anti();
	virtual void set_vox_hang();
	virtual void set_vox_on_dataport();

	virtual void get_cw_wpm_min_max(int &min, int &max) {
		min = 4; max = 60; }

	virtual void set_cw_weight();
	virtual void set_cw_wpm();
	virtual void enable_keyer();
	virtual void set_cw_qsk();
//	virtual void set_cw_vol();
	virtual bool set_cw_spot();
//	virtual void set_cw_spot_tone();
	void set_break_in();
	int  get_break_in();

    virtual std::vector<MemoryResponse> get_memory_channels();
    std::vector<MemoryResponse> get_memory_range(int start_channel, int end_channel);
    std::string get_memory_tag(const std::string memory_channel_id_str_);
    bool parse_memory_response(const std::string replystr, const size_t offset, MemoryResponse &parsedResponse);
    bool get_memory_config(const std::string memory_channel_id_str_, MemoryResponse &parsedResponse);
	virtual void select_channel(int channel);

    virtual bool get_current_memory(int &memory_channel, std::string &memory_channel_tag);
	virtual void get_band_selection(int v);
    virtual void vfo_mem_toggle();
	virtual void change_channel(bool channel_up);
	virtual void power(bool on);

	void get_nr_min_max_step(int &min, int &max, int &step) {
		min = 1; max = 15; step = 1; }
	void set_noise_reduction_val(int val);
	int  get_noise_reduction_val();
	void set_noise_reduction(int val);
	int  get_noise_reduction();

	void set_xcvr_auto_on();
	void set_xcvr_auto_off();

	void sync_date(char *dt);
	void sync_clock(char *tm);

};

#endif
