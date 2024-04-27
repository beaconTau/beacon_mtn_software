
#include <libconfig.h> 
#include <string.h> 
#include "beacon-cfg.h" 
#include "beacon.h"


/** Config file parsing uses libconfig. Not sure if it's the most efficient
 * option, but writing our own parser is a big hassle.
 *
 * Because comments aren't supported in writing out libconfig things, I just manually 
 * write out everything to file, including the "default" comments. 
 **/ 








/////////////////////////////////////////////////////
// copy config 
/////////////////////////////////////////////////////

void beacon_copy_config_init(beacon_copy_cfg_t * c) 
{
  c->remote_user = strdup("radio") ;
  c->remote_hostname = strdup("beacon_archive");
  c->local_path = strdup("/data/daq") ;
  c->remote_path = strdup("/home/radio/flower_data_archive/") ;
  c->port = 22; //The default for ssh is 22
  c->free_space_delete_threshold = 12000; 
  c->delete_files_older_than = 7;  // ? hopefully this is enough! 
  c->wakeup_interval = 600; // every 10 mins
  c->dummy_mode = 0; 
}


int beacon_copy_config_read(const char * file, beacon_copy_cfg_t * c) 
{

  config_t cfg; 
  config_init(&cfg); 
  config_set_auto_convert(&cfg,CONFIG_TRUE); 

  config_read_file(&cfg,file); 
 
  const char * remote_hostname_str; 
  if (config_lookup_string(&cfg,"remote_hostname", &remote_hostname_str))
  {
    free(c->remote_hostname);
    c->remote_hostname = strdup(remote_hostname_str); 
  } 

  config_lookup_int(&cfg,"port",&c->port);

  const char * remote_path_str; 
  if (config_lookup_string(&cfg,"remote_path", &remote_path_str))
  {
    free(c->remote_path); 
    c->remote_path = strdup(remote_path_str); 
  } 

  const char * remote_user_str; 
  if (config_lookup_string(&cfg,"remote_user", &remote_user_str))
  {
    free(c->remote_user); 
    c->remote_user = strdup(remote_user_str);
  } 

  const char * local_path_str; 
  if (config_lookup_string(&cfg,"local_path", &local_path_str))
  {
    free(c->local_path); 
    c->local_path = strdup(local_path_str); 
  } 

  config_lookup_int(&cfg,"free_space_delete_threshold",&c->free_space_delete_threshold); 
  config_lookup_int(&cfg,"delete_files_older_than",&c->delete_files_older_than); 
  config_lookup_int(&cfg,"wakeup_interval",&c->wakeup_interval); 
  config_lookup_int(&cfg,"dummy_mode",&c->dummy_mode); 


  config_destroy(&cfg); 


  return 0; 
}

int beacon_copy_config_write(const char * file, const beacon_copy_cfg_t * c) 
{
  FILE * f = fopen(file,"w"); 
  if (!f) return 1; 
  fprintf(f,"//Configuration file for beacon-copy\n\n"); 
  fprintf(f,"//The host to copy data to\n"); 
  fprintf(f,"remote_hostname = \"%s\";\n\n", c->remote_hostname); 
  fprintf(f,"//The ssh port through which to access the remote\n");
  fprintf(f,"port = %d;\n\n", c->port);
  fprintf(f,"//The remote path to copy data to\n"); 
  fprintf(f,"remote_path = \"%s\";\n\n", c->remote_path); 
  fprintf(f,"//The remote user to copy data as (if you didn't set up ssh keys, this won't work so well)\n"); 
  fprintf(f,"remote_user = \"%s\";\n\n", c->remote_user); 
  fprintf(f,"//The local path to copy data from (note that the CONTENTS of this directory are copied, e.g. an extra / is added to the rsync source)\n"); 
  fprintf(f,"local_path = \"%s\";\n\n", c->local_path); 
  fprintf(f,"//Only attempt to automatically delete old files when free space is below this threshold (in MB)\n"); 
  fprintf(f,"free_space_delete_threshold = %d;\n\n", c->free_space_delete_threshold); 
  fprintf(f,"//Delete files from local path with modifications times GREATER than this number of days (i.e. if 7, will delete files 8 days and older)\n"); 
  fprintf(f,"delete_files_older_than = %d;\n\n", c->delete_files_older_than); 
  fprintf(f,"//This controls how long the process sleeps between copies / deletes\n"); 
  fprintf(f,"wakeup_interval = %d;\n\n", c->wakeup_interval); 
  fprintf(f,"//If non-zero, won't actually delete any files\n"); 
  fprintf(f,"dummy_mode = %d;\n\n", c->dummy_mode); 
  fclose(f); 

  return 0; 
}



/////////////////////////////////////////////////////
// acq config 
/////////////////////////////////////////////////////



void beacon_acq_config_init ( beacon_acq_cfg_t * c) 
{
  c->spi_device[0] = strdup("/dev/spidev1.0"); 
  c->spi_device[1] = strdup("/dev/spidev0.0"); 
  c->run_file = strdup("/beacon/runfile") ;
  c->status_save_file = strdup("/beacon/last.st.bin"); 
  c->output_directory = strdup("/data/daq/") ; 

  c->load_thresholds_from_status_file = 1; 

  int i; 
  for ( i = 0; i < BN_NUM_CHAN; i++) c->coinc_scaler_goal[i] = 500; 
  for ( i = 0; i < BN_NUM_CHAN; i++) c->fixed_coinc_threshold[i] =  i < 4 ?  15 : 40; 

  for ( i = 0; i < BN_NUM_BEAMS; i++) c->phased_scaler_goal[i] = 500; 
  for ( i = 0; i < BN_NUM_BEAMS; i++) c->fixed_phased_threshold[i] =  i < 4 ?  15 : 40; 

  c->use_fixed_thresholds = 1;
  
  c->coinc_servo_scaler_frac = 0.9; 
  c->phased_servo_scaler_frac = 0.9; 


  //TODO tune this 
  c->k_p = 10; 
  c->k_i = 0; 
  c->k_d = 0; 
  c->k_p_p = 10; 
  c->k_i_p = 0; 
  c->k_d_p = 0; 
  c->min_coinc_threshold = 5;
  c->min_phased_threshold = 5;
  c->weight1Hz = 0.5; 
  c->max_threshold_increase = 5; 
  c->coinc_trigger_mask = 0xf; 
  c->phased_trigger_mask_lower = 0x1fffff; //21 beams here
  c->phased_trigger_mask_lower = 0x1fffff; //other 21 here 

  c->buffer_capacity = 256; 
  c->monitor_interval = 1.0; 
  c->sw_trigger_interval = 1; 
  c->randomize_sw_trigger = 0; 
  c->print_interval = 10; 
  c->swap_boards = 0; 

  c->run_length = 10800; 
  c->waveform_length = 512; 
  c->pretrigger = 6; 
  c->events_per_file = 200; 
  c->status_per_file = 100; 
  c->realtime_priority = 20; 

  c->copy_paths_to_rundir = strdup("/proc/loadavg");
  c->copy_configs = 1; 

  c->vpp_mode = 0; 
  c->coinc_window = 3; 
  c->ncoinc = 3; 
  c->enable_phased = 1;
  c->enable_coinc = 0;
  c->enable_pps = 1;
  c->pps_delay = 0; 
  c->spi_enable = -61; 
  c->gpio_int[0] = 44;
  c->gpio_int[1] = 89;


  for (int i = 0; i < BN_NUM_CHAN; i++) 
  {
    c->gain_codes[0][i] = i < 4 ? 0 : 5; 
    c->gain_codes[1][i] = 5; 
  }

  c->target_rms =5; 
  c->use_100Hz_scalers = 0; 
}


int beacon_acq_config_read(const char * fi, beacon_acq_cfg_t * c) 
{

  config_t cfg; 
  config_init(&cfg); 
  config_set_auto_convert(&cfg, CONFIG_TRUE); 
  if (!config_read_file(&cfg, fi))
  {
     fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
     config_error_line(&cfg), config_error_text(&cfg));

     config_destroy(&cfg); 
     return 1; 
  }
 
  
  int i; 
  for (i = 0; i < BN_NUM_CHAN; i++) 
  {
    char buf[128]; 
    int tmp; 
    sprintf(buf, "control.coinc_scaler_goal.ch%d",i); 
    config_lookup_float(&cfg, buf, &c->coinc_scaler_goal[i]); 
    sprintf(buf, "control.fixed_coinc_threshold.ch%d",i); 
    config_lookup_int(&cfg, buf, &tmp); 
    c->fixed_coinc_threshold[i] = tmp; 
  }

  for (i = 0; i < BN_NUM_BEAMS; i++) 
  {
    char buf[128]; 
    int tmp; 
    sprintf(buf, "control.phased_scaler_goal.bm%d",i); 
    config_lookup_float(&cfg, buf, &c->phased_scaler_goal[i]); 
    sprintf(buf, "control.fixed_phased_threshold.bm%d",i); 
    config_lookup_int(&cfg, buf, &tmp); 
    c->fixed_phased_threshold[i] = tmp; 
  }

  int tmp; 
  if ( config_lookup_int(&cfg,"control.coinc_trigger_mask",&tmp))
  {
    c->coinc_trigger_mask = tmp; 
  }
  if ( config_lookup_int(&cfg,"control.phased_trigger_mask_lower",&tmp))
  {
    c->phased_trigger_mask_lower = tmp; 
  }  
  if ( config_lookup_int(&cfg,"control.phased_trigger_mask_upper",&tmp))
  {
    c->phased_trigger_mask_upper = tmp; 
  }
  config_lookup_float(&cfg,"control.k_p",&c->k_p); 
  config_lookup_float(&cfg,"control.k_i",&c->k_i); 
  config_lookup_float(&cfg,"control.k_d",&c->k_d); 
  config_lookup_float(&cfg,"control.k_p_p",&c->k_p_p); 
  config_lookup_float(&cfg,"control.k_i_p",&c->k_i_p); 
  config_lookup_float(&cfg,"control.k_d_p",&c->k_d_p); 
  config_lookup_int(&cfg,"control.min_coinc_threshold",&tmp);
  c->min_coinc_threshold = tmp;
  config_lookup_int(&cfg,"control.min_phased_threshold",&tmp);
  c->min_phased_threshold = tmp;
  config_lookup_int(&cfg,"control.max_threshold_increase",&tmp);   
  c->max_threshold_increase = tmp; 
  config_lookup_float(&cfg,"control.monitor_interval",&c->monitor_interval); 
  config_lookup_float(&cfg,"control.sw_trigger_interval",&c->sw_trigger_interval); 
  config_lookup_int(&cfg,"control.randomize_sw_trigger",&c->randomize_sw_trigger); 
  config_lookup_int(&cfg,"realtime_priority",&c->realtime_priority); 
  config_lookup_int(&cfg,"control.use_fixed_thresholds",&c->use_fixed_thresholds); 

  config_lookup_float(&cfg,"control.scaler_weight_1Hz", &c->weight1Hz); 
  config_lookup_int(&cfg,"control.vpp_mode", &c->vpp_mode); 
  config_lookup_int(&cfg,"control.coinc_window", &c->coinc_window); 
  config_lookup_int(&cfg,"control.ncoinc", &c->ncoinc); 
  config_lookup_int(&cfg,"control.enable_phased_trig", &c->enable_phased); 
  config_lookup_int(&cfg,"control.enable_coinc_trig", &c->enable_coinc); 
  config_lookup_int(&cfg,"control.enable_pps_trig", &c->enable_pps); 
  config_lookup_float(&cfg,"control.pps_delay", &c->pps_delay); 

  const char * status_save = 0; 

  if (config_lookup_string(&cfg, "control.status_save_file", &status_save))
  {
    free(c->status_save_file);
    c->status_save_file = strdup(status_save); 
  }

  config_lookup_int(&cfg,"control.load_thresholds_from_status_file",&c->load_thresholds_from_status_file); 

  const char *spi = 0; 

  if (config_lookup_string(&cfg, "device.spi_device.M", &spi))
  {
    free(c->spi_device[0]);
    c->spi_device[0] = strdup(spi); 
  }

  if (config_lookup_string(&cfg, "device.spi_device.S", &spi))
  {
    free(c->spi_device[1]);
    c->spi_device[1] = strdup(spi); 
  }

  config_lookup_int(&cfg,"device.spi_enable",&c->spi_enable);
  config_lookup_int(&cfg,"device.gpio_int.M",&c->gpio_int[0]);
  config_lookup_int(&cfg,"device.gpio_int.S",&c->gpio_int[1]);
  for (int i = 0; i < BN_NUM_CHAN; i++) 
  {
    char buf[32];
    sprintf(buf,"device.gain_codes.M.[%d]",i);
    config_lookup_int(&cfg,buf, &c->gain_codes[0][i]); 
    sprintf(buf,"device.gain_codes.S.[%d]",i);
    config_lookup_int(&cfg,buf, &c->gain_codes[1][i]); 
  }
  config_lookup_float(&cfg,"device.target_rms", &c->target_rms); 
  config_lookup_int(&cfg,"device.use_100Hz_scalers",&c->use_100Hz_scalers); 

  config_lookup_int(&cfg,"device.buffer_capacity", &c->buffer_capacity); 
  config_lookup_int(&cfg,"device.waveform_length", &c->waveform_length); 
  config_lookup_int(&cfg,"device.pretrigger", &c->pretrigger); 
  config_lookup_int(&cfg,"device.swap_boards", &c->swap_boards); 


  const char * run_file ; 
  if (config_lookup_string( &cfg, "output.run_file", &run_file))
  {
    free(c->run_file);
    c->run_file = strdup(run_file); 
  }

  const char * output_directory ; 
  if (config_lookup_string( &cfg, "output.output_directory", &output_directory))
  {
    free(c->output_directory);
    c->output_directory = strdup(output_directory); 
  }

  const char * copy_paths; 
  if (config_lookup_string( &cfg, "output.copy_paths_to_rundir", &copy_paths))
  {
    free(c->copy_paths_to_rundir); 
    c->copy_paths_to_rundir = strdup(copy_paths); 
  }


  config_lookup_int(&cfg,"output.print_interval", &c->print_interval); 
  config_lookup_int(&cfg,"output.run_length", &c->run_length); 
  config_lookup_int(&cfg,"output.events_per_file", &c->events_per_file); 
  config_lookup_int(&cfg,"output.status_per_file", &c->status_per_file); 
  config_lookup_int(&cfg,"output.copy_configs", &c->copy_configs); 

  return 0; 

}

int beacon_acq_config_write(const char * fi, const beacon_acq_cfg_t * c) 
{

  FILE * f = fopen(fi,"w");  
  int i = 0; 
  if (!f) return -1; 
  fprintf(f,"// config file for beacon-acq\n"); 
  fprintf(f,"// not all options are changeable by restart\n\n"); 

  fprintf(f,"// settings related to threshold  / trigger control\n"); 
  fprintf(f,"// These all can be set without restarting\n"); 
  fprintf(f,"control:\n"); 
  fprintf(f,"{\n"); 
  fprintf(f,"   // scaler goals for each channel, desired rate ( in Hz)\n"); 
  fprintf(f,"   coinc_scaler_goal = {\n"); 
  for (i = 0; i < BN_NUM_CHAN; i++)
  {
    fprintf(f, "     ch%d : %g;\n", i, c->coinc_scaler_goal[i]); 
  }
  fprintf(f,"    };\n\n"); 

  fprintf(f,"   // fixed thresholds for each channel (in case of use_fixed_thresholds)\n"); 
  fprintf(f,"   fixed_coinc_threshold = {\n"); 
  for (i = 0; i < BN_NUM_CHAN; i++)
  {
    fprintf(f, "     ch%d : %u;\n", i, c->fixed_coinc_threshold[i]); 
  }
  fprintf(f,"    };\n\n"); 

  fprintf(f,"   //the channels allowed to participate in the trigger\n"); 
  fprintf(f,"   coinc_trigger_mask = 0x%x;\n\n", c->coinc_trigger_mask);  
  


  fprintf(f,"   // scaler goals for each beam, desired rate ( in Hz)\n"); 
  fprintf(f,"   phased_scaler_goal = {\n"); 
  for (i = 0; i < BN_NUM_BEAMS; i++)
  {
    fprintf(f, "     bm%d : %g;\n", i, c->phased_scaler_goal[i]); 
  }
  fprintf(f,"    };\n\n"); 

  fprintf(f,"   // fixed thresholds for each beam (in case of use_fixed_thresholds)\n"); 
  fprintf(f,"   fixed_phased_threshold = {\n"); 
  for (i = 0; i < BN_NUM_BEAMS; i++)
  {
    fprintf(f, "     bm%d : %u;\n", i, c->fixed_phased_threshold[i]); 
  }
  fprintf(f,"    };\n\n"); 

  fprintf(f,"   //the beams allowed to participate in the trigger\n"); 
  fprintf(f,"   phased_trigger_mask_lower = 0x%x;\n\n", c->phased_trigger_mask_lower);  
  fprintf(f,"   phased_trigger_mask_upper = 0x%x;\n\n", c->phased_trigger_mask_upper);  


  fprintf(f,"   // use fixed thresholds (don't servo!) \n"); 
  fprintf(f,"   use_fixed_thresholds = %d;\n\n", c->use_fixed_thresholds); 

  fprintf(f,"   // 1Hz scaler weight \n"); 
  fprintf(f,"   scaler_weight_1Hz = %f;\n\n", c->weight1Hz); 

  fprintf(f,"   // use 100 Hz scalers instead of 100 mHz scalers...\n"); 
  fprintf(f,"   use_100Hz_scalers = %d;\n\n", c->use_100Hz_scalers); 


  fprintf(f,"   // coinc pid loop proportional term\n"); 
  fprintf(f,"   k_p = %g;\n\n", c->k_p); 

  fprintf(f,"   // coinc pid loop integral term\n"); 
  fprintf(f,"   k_i = %g;\n\n", c->k_i);

  fprintf(f,"   // coinc pid loop differential term\n"); 
  fprintf(f,"   k_d = %g;\n\n", c->k_d);

  fprintf(f,"   // phased pid loop proportional term\n"); 
  fprintf(f,"   k_p_p = %g;\n\n", c->k_p_p); 

  fprintf(f,"   // phased pid loop integral term\n"); 
  fprintf(f,"   k_i_p = %g;\n\n", c->k_i_p);

  fprintf(f,"   // phased pid loop differential term\n"); 
  fprintf(f,"   k_d_p = %g;\n\n", c->k_d_p);

  fprintf(f,"   // puts a floor on the coinc thresholds\n"); 
  fprintf(f,"   min_threshold=%u;\n\n", c->min_coinc_threshold);

  fprintf(f,"   // puts a floor on the phased thresholds\n"); 
  fprintf(f,"   min_threshold=%u;\n\n", c->min_phased_threshold);

  fprintf(f,"   // max threshold increase per step \n"); 
  fprintf(f,"   max_threshold_increase=%u;\n\n", c->max_threshold_increase); 
  
  fprintf(f,"   //monitoring interval, for PID loop (in seconds)\n"); 
  fprintf(f,"   monitor_interval = %g;\n\n",c->monitor_interval); 

  fprintf(f,"   // software trigger interval (in seconds)\n"); 
  fprintf(f,"   sw_trigger_interval = %g;\n\n", c->sw_trigger_interval); 

  fprintf(f,"   // randomize sw trigger interval (using exponential distribution)\n"); 
  fprintf(f,"   randomize_sw_trigger = %d;\n\n", c->randomize_sw_trigger); 

  fprintf(f,"   //File to persist the status info (primarily for saving thresholds between restarts)\n") ;
  fprintf(f,"   status_save_file = \"%s\"\n\n", c->status_save_file); 

  fprintf(f,"   // load thresholds from status file on start.\n");  
  fprintf(f,"   load_thresholds_from_status_file=%d\n\n", c->load_thresholds_from_status_file); 

  fprintf(f,"   // use vpp for trigger\n"); 
  fprintf(f,"   vpp_mode=%d;\n\n",c->vpp_mode); 
   
  fprintf(f,"   // coincidence window, in units of clock ticks (125 MHz, so 8 ns/tick)\n"); 
  fprintf(f,"   coinc_window=%d;\n\n",c->coinc_window); 

  fprintf(f,"   // concidences required this is a >, so 0 means 1 channel, 1 means 2 cvhannels, etc.\n"); 
  fprintf(f,"   ncoinc=%d;\n\n",c->ncoinc); 

  fprintf(f,"   // enable phased trigger\n"); 
  fprintf(f,"   enable_phased_trig=%d;\n\n",c->enable_phased); 

  fprintf(f,"   // enable coincidence trigger\n"); 
  fprintf(f,"   enable_coinc_trig=%d;\n\n",c->enable_coinc); 

  fprintf(f,"   // enable pps trigger\n"); 
  fprintf(f,"   enable_pps_trig=%d;\n\n",c->enable_pps); 

  fprintf(f,"   // pps delay (ns)\n"); 
  fprintf(f,"   pps_delay=%f;\n\n",c->pps_delay); 
  fprintf(f,"};\n\n"); 

  fprintf(f,"// settings related to the acquisition\n"); 
  fprintf(f,"// Not all of these can be set without restarting\n"); 
  fprintf(f,"device: \n");
  fprintf(f,"{\n"); 
  fprintf(f,"  //spi devices, main (triggering) and secondary (non-triggering)\n"); 
  fprintf(f,"  spi_device =  { M: \"%s\", S: \"%s\"; } \n\n", c->spi_device[0], c->spi_device[1]); 

  fprintf(f,"  // gpios for interrupts\n");
  fprintf(f,"  gpio_int = { M: %d , S: %d ; }\n\n", c->gpio_int[0], c->gpio_int[1]); 

  fprintf(f,"  //spi enable, negative for active high\n"); 
  fprintf(f,"  spi_enable = %d;\n\n", c->spi_enable); 

   fprintf(f,"  //swap the two boards (M <->S )\n"); 
  fprintf(f,"  swap_boards = %d;\n\n", c->swap_boards); 

 
  fprintf(f,"  // circular buffer capacity. In-memory storage in between acquisition and writing. Requires restart.\n"); 
  fprintf(f,"  buffer_capacity = %d;\n\n", c->buffer_capacity); 

  fprintf(f,"  //the length of a waveform, in samples. \n"); 
  fprintf(f,"  waveform_length = %d;\n\n", c->waveform_length); 

  fprintf(f,"  //the pretrigger window length, in hardware units\n"); 
  fprintf(f,"  pretrigger = %d;\n\n", c->pretrigger); 

  fprintf(f,"  // fixed gain setting per channel (see table 21 of HMCAD1511 datasheet). Use -1 to equalize to target_rms. \n"); 
  fprintf(f,"  gain_codes = { M: [ %d,%d,%d,%d,%d,%d,%d,%d ], S: [ %d,%d,%d,%d,%d,%d,%d,%d] }; \n\n",
          c->gain_codes[0][0], c->gain_codes[0][1], c->gain_codes[0][2], c->gain_codes[0][3], c->gain_codes[0][4], c->gain_codes[0][5], c->gain_codes[0][6], c->gain_codes[0][7], 
          c->gain_codes[1][0], c->gain_codes[1][1], c->gain_codes[1][2], c->gain_codes[1][3], c->gain_codes[1][4], c->gain_codes[1][5], c->gain_codes[1][6], c->gain_codes[1][7] ); 

  fprintf(f,"  //target rms for equalized channels\n"); 
  fprintf(f,"  target_rms = %f;\n\n", c->target_rms); 
  fprintf(f,"};\n\n"); 


  fprintf(f,"//settings related to output\n"); 
  fprintf(f,"output: \n") ; 
  fprintf(f,"{\n"); 

  fprintf(f,"  // Run file, used to persist run number\n"); 
  fprintf(f,"  run_file = \"%s\";\n\n", c->run_file);  

  fprintf(f,"  // output directory, data will go here\n"); 
  fprintf(f,"  output_directory = \"%s\" ;\n\n", c->output_directory); 

  fprintf(f,"  //print to screen interval (0 to disable)\n"); 
  fprintf(f,"  print_interval = %d;\n\n", c->print_interval); 

  fprintf(f,"  // run length, in seconds\n"); 
  fprintf(f,"  run_length = %d; \n\n",c->run_length); 

  fprintf(f,"  //events per output file\n");
  fprintf(f,"  events_per_file = %d;\n\n", c->events_per_file); 

  fprintf(f,"  //statuses per output file\n"); 
  fprintf(f,"  status_per_file = %d;\n\n", c->status_per_file); 

  fprintf(f,"  //realtime priority setting. If 0, will use non-realtime priority. Otherwise, SCHED_FIFO is used with the given priority\n"); 
  fprintf(f,"  realtime_priority = %d;\n\n", c->realtime_priority); 

  fprintf(f,"  // Colon separated list of paths to copy (recursively) into run dir at start of run\n");
  fprintf(f,"  copy_paths_to_rundir = \"%s\";\n\n", c->copy_paths_to_rundir); 

  fprintf(f,"  //Whether or not to copy configs into run dir\n"); 
  fprintf(f,"  copy_configs = %d;\n", c->copy_configs); 

  fprintf(f,"};\n\n"); 

  return 0; 
}








