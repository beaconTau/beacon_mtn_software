#ifndef _BEACON_CFG_H
#define _BEACON_CFG_H

#include "beacon-common.h" 
#include "beacon.h" 
#include "beaconhk.h"
#include "beacondaq.h" 
#include <stdlib.h>

/** 
 * \file beacon-cfg.h 
 *
 * This contains the configuration structs
 * and the methods to read / write them. 
 *
 * Usually, each program will use a config file to read/write these. 
 *
 * See the config files in cfg/ 
 */ 



/** Configuration options for beacon-acq */ 
typedef struct beacon_acq_cfg
{

  /* the names of the spi devices
   * 0 should be master, 1 should be slave/ 
   * */ 
  char * spi_device; 

  /* the name of the file holding the desired run number*/ 
  char * run_file; 

  /* The name of the file holding the last status */ 
  char * status_save_file;

  /* Whether  or not to load the last thresholds from the status file on startup */ 
  int load_thresholds_from_status_file; 


  /* The output directory for files */ 
  char * output_directory; 

  //scaler goals, in Hz 
  double scaler_goal[BN_NUM_BEAMS]; 

  // trigger mask
  uint32_t trigger_mask; 

  // channel mask
  uint8_t channel_mask; 

  //channel read_mask
  uint8_t channel_read_mask; 

  // pid goal constats;
  double k_p,k_i, k_d; 

  // puts a floor on the thresholds
  uint16_t min_threshold;

  // the maximum the threshold can increase in a  step 
  uint16_t max_threshold_increase; 

  /* The size of the circular buffers */ 
  int buffer_capacity; 

  /* Monitor interval  (in seconds) */ 
  double monitor_interval; 

  /* SW trigger interval (in seconds) */ 
  double sw_trigger_interval; 

  /* Use an exponential distribution for the sw trigger */ 
  int randomize_sw_trigger; 

  // print to screen interval 
  int print_interval; 

  /* The maximum length of a run in seconds */ 
  int run_length; 

  /* The SPI clock speed, in MHz */ 
  int spi_clock; 
  
  // number of samples to save 
  int waveform_length;

  /** 1 to enable the phased trigger, 0 otherwise */ 
  int enable_phased_trigger; 

  // Trigger polarization, see 
  beacon_trigger_polarization_t trigger_polarization;

  /** cal pulser state , 0 for off, 3 for on (or 2 for nothing)
   *
   * I don't think we'd ever want it on. If you do, make sure
   * you disable the trigout 
   * */ 
  int calpulser_state; 

  // enable the trigout
  int enable_trigout;

  // enable ext in 
  int enable_extin; 

  // The width (in 40 ns increments) of the external trigger output
  int trigout_width; 

  // disable the trigout on exit (this also means
  // it's temporarily disabled between runs ) 
  int disable_trigout_on_exit; 

  // Use this to apply the attenuations instead of just
  // using whatever is on the board. 
  int apply_attenuations; 
  uint8_t attenuation[BN_NUM_CHAN]; 


  // Program called to check alignment / align the cal pulser 
  char * alignment_command; 

  int pretrigger; 

  double slow_scaler_weight; 

  double fast_scaler_weight; 

  int subtract_gated; 

  int secs_before_phased_trigger; 

  int events_per_file; 

  int status_per_file; 

  int n_fast_scaler_avg; 

  int realtime_priority; 

  char * copy_paths_to_rundir; 

  int copy_configs; 

  uint16_t poll_usecs; 

  uint8_t trig_delays[BN_NUM_CHAN]; 

  int use_fixed_thresholds; 
  uint32_t fixed_threshold[BN_NUM_BEAMS];  
  int enable_dynamic_masking; 
  uint8_t dynamic_masking_threshold; 
  uint8_t dynamic_masking_holdoff; 
  int enable_low_pass_to_trigger; 

  /** Power monitoring commands */ 
  int try_again_sleep_amount; 
  int check_power_on; 
  int adc_threshold_for_on; 
  int auto_power_on; 
  int auto_power_off; 
  int power_monitor_interval; 
  int nzero_threshold_to_turn_off;
  double cc_voltage_to_turn_off; 
  double inv_voltage_to_turn_off; 
  double cc_voltage_to_turn_on; 
  double inv_voltage_to_turn_on; 

  char * power_off_command; 
  char * power_on_command; 

  /* Veto options */ 
  beacon_veto_options_t veto; 


} beacon_acq_cfg_t; 


/** Initialize a config with defaults. Usually a good idea to do this before reading a file in case the config file doesn't have all the keys */
void beacon_acq_config_init(beacon_acq_cfg_t *); 

/** Replace any values in the config with the values from the file */ 
int beacon_acq_config_read(const char * file, beacon_acq_cfg_t * ); 

/** Write out the current configuration to the file */ 
int beacon_acq_config_write(const char * file, const beacon_acq_cfg_t * ); 


typedef struct beacon_copy_cfg
{
  char * remote_hostname; 
  int port; //ssh port for the remote
  char * remote_path; 
  char * remote_user; 
  char * local_path;
  int free_space_delete_threshold; //MB 
  int delete_files_older_than;  //days
  int wakeup_interval; //seconds
  int dummy_mode; // don't actually delete, just print files 

} beacon_copy_cfg_t; 


void beacon_copy_config_init(beacon_copy_cfg_t *); 
int beacon_copy_config_read(const char * file, beacon_copy_cfg_t * ); 
int beacon_copy_config_write(const char * file, const beacon_copy_cfg_t * ); 

typedef struct beacon_start_cfg
{
  char * set_attenuation_cmd; 
  char * reconfigure_fpga_cmd; 
  char * out_dir; //output directory for hk data 
  double desired_rms; 
}beacon_start_cfg_t; 


void beacon_start_config_init(beacon_start_cfg_t *); 
int beacon_start_config_read(const char * file, beacon_start_cfg_t * ); 
int beacon_start_config_write(const char * file, const beacon_start_cfg_t * ); 

/* configuration options for beacon-hkd */ 
typedef struct beacon_hkd_cfg
{
  int interval; //polling interval for HK data . Default 5 seconds 
  char * out_dir; //output directory for hk data 
  int max_secs_per_file; // maximum number of seconds per file. Default 600
  char * shm_name; //shared memory name
  char * shm_lock_name; //shared memory lock name
  int print_to_screen; //1 to print to screen 
  char * mate3_url; 
  int mate3_port; 

} beacon_hk_cfg_t;



void beacon_hk_config_init(beacon_hk_cfg_t *); 
int beacon_hk_config_read(const char * file, beacon_hk_cfg_t * ); 
int beacon_hk_config_write(const char * file, const beacon_hk_cfg_t * ); 


#endif
