#ifndef _BEACON_CFG_H
#define _BEACON_CFG_H

#include "beacon-common.h" 
#include "beacon.h" 
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
   * 0 should be M, 1 should be S 
   * */ 
  char * spi_device[2]; 

  /* the name of the file holding the desired run number*/ 
  char * run_file; 

  /* The name of the file holding the last status */ 
  char * status_save_file;

  /* Whether  or not to load the last thresholds from the status file on startup */ 
  int load_thresholds_from_status_file; 

  // swap the two boards 
  int swap_boards; 


  // vpp mode 
  int vpp_mode; 
  int coinc_window; 
  int ncoinc; 

  int enable_coinc; 
  int enable_pps; 
  double pps_delay; 

  int spi_enable; 
  int gpio_int[2]; 

  /* The output directory for files */ 
  char * output_directory; 

  int use_fixed_thresholds; 

  uint32_t fixed_threshold[BN_NUM_CHAN];  

  //scaler goals, in Hz, if servo enabled
  double scaler_goal[BN_NUM_CHAN]; 

  //fraction of trigger scalers that servo scalers run at 
  double servo_scaler_frac; 

  //weight of 1 Hz scaler in servo (other weight computed)
  double weight1Hz; 

  // trigger mask
  uint32_t trigger_mask; 

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

  // number of samples to save 
  int waveform_length;

  int events_per_file; 

  int status_per_file; 

  int realtime_priority; 

  char * copy_paths_to_rundir; 

  int copy_configs; 

  int pretrigger; 

  int gain_codes[2][8];  
  double target_rms; 

  int use_100Hz_scalers; 

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



#endif
