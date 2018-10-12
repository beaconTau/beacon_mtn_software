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



//////////////////////////////////////////////////////
//start config 
/////////////////////////////////////////////////////

void beacon_start_config_init(beacon_start_cfg_t * c) 
{
  c->set_attenuation_cmd = "cd /home/beacon/beacon_python; python set_attenuation.py";
  c->reconfigure_fpga_cmd = "cd /home/beacon/beacon_python; ./reconfigureFPGA -a 0;";
  c->desired_rms= 4.2; 
  c->out_dir = "/data/startup/"; 
}


int beacon_start_config_read(const char * file, beacon_start_cfg_t * c) 
{
  config_t cfg; 
  config_init(&cfg); 
  config_set_auto_convert(&cfg,CONFIG_TRUE); 

  if (!config_read_file(&cfg, file))
  {
     fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
     config_error_line(&cfg), config_error_text(&cfg));

     config_destroy(&cfg); 
     return 1; 
  }
  config_lookup_float(&cfg,"desired_rms", &c->desired_rms);

  const char * attenuation_cmd; 
  if (config_lookup_string(&cfg, "set_attenuation_cmd", &attenuation_cmd))
  {
    c->set_attenuation_cmd = strdup(attenuation_cmd); //memory leak :( 

 
  const char * reconfigure_cmd; 
  if (config_lookup_string(&cfg, "reconfigure_fpga_cmd", &reconfigure_cmd))
  {
    c->reconfigure_fpga_cmd = strdup(reconfigure_cmd); //memory leak :( 
  }
 }

  const char * out_dir; 
  if (config_lookup_string(&cfg, "out_dir", &out_dir))
  {
    c->out_dir = strdup(out_dir); //memory leak :( 
  }


  config_destroy(&cfg); 
  return 0; 
}

int beacon_start_config_write(const char * file, const beacon_start_cfg_t * c) 
{
  FILE * f = fopen(file,"w"); 

  if (!f) return 1; 

  fprintf(f,"//Configuration file for beacon-start\n\n");  
  fprintf(f,"// Command to run after turning on boards to tune attenuations \n"); 
  fprintf(f,"set_attenuation_cmd = \"%s\";\n\n", c->set_attenuation_cmd); 
  fprintf(f,"// Command to run after turning on boards to reconfigure FGPA's\n"); 
  fprintf(f,"reconfigure_fpga_cmd = \"%s\";\n\n", c->reconfigure_fpga_cmd); 
  fprintf(f,"//rms goal \n"); 
  fprintf(f,"desired_rms= %f;\n\n", c->desired_rms); 
  fprintf(f, "//output directory \n"); 
  fprintf(f, "out_dir=\"%s\";\n\n", c->out_dir); 
  fclose(f); 

  return 0; 
}



//////////////////////////////////////////////////////
//hk config 
/////////////////////////////////////////////////////
void beacon_hk_config_init(beacon_hk_cfg_t * c) 
{
  c->interval = 5; 
  c->out_dir = "/data/hk/"; 
  c->max_secs_per_file = 600; 
  c->shm_name = "/hk.bin"; 
  c->print_to_screen = 1; 
}

int beacon_hk_config_read(const char * file, beacon_hk_cfg_t * c) 
{
  config_t cfg; 
  config_init(&cfg); 
  config_set_auto_convert(&cfg,CONFIG_TRUE); 

  if (!config_read_file(&cfg, file))
  {
     fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
     config_error_line(&cfg), config_error_text(&cfg));

     config_destroy(&cfg); 
     return 1; 
  }
  config_lookup_int(&cfg,"interval", &c->interval);
  config_lookup_int(&cfg,"print_to_screen", &c->print_to_screen);
  config_lookup_int(&cfg,"max_secs_per_file", &c->max_secs_per_file);

  const char * outdir_str; 
  if (config_lookup_string(&cfg,"out_dir", &outdir_str))
  {
    c->out_dir = strdup(outdir_str); //memory leak, but not easy to do anything else here. 
  }
  const char * shm_str; 
  if (config_lookup_string(&cfg,"shm_name", &shm_str))
  {
    c->shm_name = strdup(shm_str); //memory leak, but not easy to do anything else here. 
  }


  config_destroy(&cfg); 
  return 0; 
}

int beacon_hk_config_write(const char * file, const beacon_hk_cfg_t * c) 
{
  FILE * f = fopen(file,"w"); 

  if (!f) return 1; 

  fprintf(f,"//Configuration file for beacon-hk\n");  
  fprintf(f, "//Polling interval, in seconds. Treated as integer.  \n"); 
  fprintf(f, "interval=%d;\n\n", c->interval); 
  fprintf(f, "//max seconds per file, in seconds. Treated as integer.  \n"); 
  fprintf(f, "max_secs_per_file=%d;\n\n", c->max_secs_per_file); 
  fprintf(f, "//output directory \n"); 
  fprintf(f, "out_dir=\"%s\";\n\n", c->out_dir); 
  fprintf(f, "//shared binary data name \n"); 
  fprintf(f, "shm_name=\"%s\";\n\n", c->shm_name); 
  fprintf(f, "//1 to print to screen\n"); 
  fprintf(f, "print_to_screen=%d;\n\n", c->print_to_screen); 
  fclose(f); 

  return 0; 
}


/////////////////////////////////////////////////////
// copy config 
/////////////////////////////////////////////////////

void beacon_copy_config_init(beacon_copy_cfg_t * c) 
{
  c->remote_user = "radio" ;
  c->remote_hostname = "beacon_archive";
  c->local_path = "/data" ;
  c->remote_path = "/home/radio/data_archive/" ;
  c->port = 2234; //The default for ssh is 22
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
    c->remote_hostname = strdup(remote_hostname_str); //memory leak, but not easy to do anything else here. 
  } 

  config_lookup_int(&cfg,"port",&c->port);

  const char * remote_path_str; 
  if (config_lookup_string(&cfg,"remote_path", &remote_path_str))
  {
    c->remote_path = strdup(remote_path_str); //memory leak, but not easy to do anything else here. 
  } 

  const char * remote_user_str; 
  if (config_lookup_string(&cfg,"remote_user", &remote_user_str))
  {
    c->remote_user = strdup(remote_user_str); //memory leak, but not easy to do anything else here. 
  } 

  const char * local_path_str; 
  if (config_lookup_string(&cfg,"local_path", &local_path_str))
  {
    c->local_path = strdup(local_path_str); //memory leak, but not easy to do anything else here. 
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
  c->spi_device = "/dev/spidev1.0"; 
  c->run_file = "/beacon/runfile" ; 
  c->status_save_file = "/beacon/last.st.bin"; 
  c->output_directory = "/data/" ; 
  c->alignment_command = "cd /home/nuphase/nuphase_python/;  python align_adcs_beacon.py" ;

  c->load_thresholds_from_status_file = 1; 

  int i; 
  for ( i = 0; i < BN_NUM_BEAMS; i++) c->scaler_goal[i] = i < 20 ? 0.75 : 0 ; 
  for ( i = 0; i < BN_NUM_BEAMS; i++) c->fixed_threshold[i] =  i < 20 ? 20000 : 0; 

  c->enable_dynamic_masking = 1; 
  c->dynamic_masking_threshold = 5; 
  c->dynamic_masking_holdoff = 100; 
  c->use_fixed_thresholds = 0; 
  c->enable_low_pass_to_trigger = 1; 

  //TODO tune this 
  c->k_p = 10; 
  c->k_i = 0.1; 
  c->k_d = 0; 
  c->min_threshold = 1000;
  c->max_threshold_increase = 500; 
  c->trigger_mask = 0xffffff; 
  c->channel_mask = 0xff; 
  c->channel_read_mask = 0xff;

  c->buffer_capacity = 256; 
  c->monitor_interval = 1.0; 
  c->sw_trigger_interval = 1; 
  c->randomize_sw_trigger = 0; 
  c->print_interval = 10; 
  c->poll_usecs = 500; 

  c->run_length = 10800; 
  c->spi_clock = 20; 
  c->waveform_length = 512; 
  c->enable_phased_trigger = 1;
  c->trigger_polarization = BEACON_DEFAULT_TRIGGER_POLARIZATION;
  c->calpulser_state = 0; 


  c->apply_attenuations = 0; 
  c->enable_trigout=1; 
  c->enable_extin = 0; 
  c->trigout_width = 3; 
  c->disable_trigout_on_exit = 1; 

  //provisional reasonable values 
  c->attenuation[0] = 0; 
  c->attenuation[1] = 0; 
  c->attenuation[2] = 0; 
  c->attenuation[3] = 0; 
  c->attenuation[4] = 0; 
  c->attenuation[5] = 0; 
  c->attenuation[6] = 0; 
  c->attenuation[7] = 0; 


  c->pretrigger = 6; 
  c->slow_scaler_weight = 0.3; 
  c->fast_scaler_weight = 0.7; 
  c->subtract_gated = 1; 
  c->secs_before_phased_trigger = 20; 
  c->events_per_file = 1000; 
  c->status_per_file = 200; 
  c->n_fast_scaler_avg = 20; 
  c->realtime_priority = 20; 

  c->copy_paths_to_rundir = "/home/beacon/beacon_python/output:/proc/loadavg";
  c->copy_configs = 1; 
  memset(c->trig_delays,0,sizeof(c->trig_delays)); 
}




void config_lookup_pol(config_t* cfg, const char* key, beacon_trigger_polarization_t* pol){

  const char* str;
  if(config_lookup_string(cfg, key, &str)){

    int foundMatch = 0;
    int polInd=-1;
    const char* polName = NULL;
    do {
      polInd++;
      polName = beacon_trigger_polarization_name((beacon_trigger_polarization_t) polInd);
      if(polName && strcmp(str, polName)==0){
	foundMatch = 1;
	break;
      }
    }
    while(polName != NULL);

    if(foundMatch==0){
      fprintf(stderr, "Warning in %s: Got unexpected pol config: %s\n", __PRETTY_FUNCTION__, key);
      fprintf(stderr, "Setting trigger polarization to \"%s\" (default)\n", beacon_trigger_polarization_name(BEACON_DEFAULT_TRIGGER_POLARIZATION));
      pol = BEACON_DEFAULT_TRIGGER_POLARIZATION;
    }
  }
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
  for (i = 0; i < BN_NUM_BEAMS; i++) 
  {
    char buf[128]; 
    int tmp; 
    sprintf(buf, "control.scaler_goal.beam%d",i); 
    config_lookup_float(&cfg, buf, &c->scaler_goal[i]); 
    sprintf(buf, "control.fixed_threshold.beam%d",i); 
    config_lookup_int(&cfg, buf, &tmp); 
    c->fixed_threshold[i] = tmp; 
  }

  int tmp; 
  if ( config_lookup_int(&cfg,"control.trigger_mask",&tmp))
    c->trigger_mask = tmp; 
  if (config_lookup_int(&cfg,"control.channel_mask",&tmp))
    c->channel_mask = tmp; 
  config_lookup_float(&cfg,"control.k_p",&c->k_p); 
  config_lookup_float(&cfg,"control.k_i",&c->k_i); 
  config_lookup_float(&cfg,"control.k_d",&c->k_d); 
  config_lookup_int(&cfg,"control.min_threshold",&tmp);
  c->min_threshold = tmp;
  config_lookup_int(&cfg,"control.max_threshold_increase",&tmp);   
  c->max_threshold_increase = tmp; 
  config_lookup_float(&cfg,"control.monitor_interval",&c->monitor_interval); 
  config_lookup_float(&cfg,"control.sw_trigger_interval",&c->sw_trigger_interval); 
  config_lookup_int(&cfg,"control.randomize_sw_trigger",&c->randomize_sw_trigger); 
  config_lookup_int(&cfg,"control.enable_phased_trigger",&c->enable_phased_trigger); 
  config_lookup_pol(&cfg,"control.trigger_polarization",&c->trigger_polarization);  
  config_lookup_int(&cfg,"control.secs_before_phased_trigger",&c->secs_before_phased_trigger); 
  config_lookup_float(&cfg,"control.fast_scaler_weight",&c->fast_scaler_weight); 
  config_lookup_float(&cfg,"control.slow_scaler_weight",&c->slow_scaler_weight); 
  config_lookup_int(&cfg,"control.n_fast_scaler_avg",&c->n_fast_scaler_avg); 
  config_lookup_int(&cfg,"control.subtract_gated",&c->subtract_gated); 
  config_lookup_int(&cfg,"realtime_priority",&c->realtime_priority); 
  config_lookup_int(&cfg,"poll_usecs",&tmp); 
  c->poll_usecs = tmp; 
  config_lookup_int(&cfg,"control.enable_dynamic_masking",&c->enable_dynamic_masking); 
  config_lookup_int(&cfg,"control.use_fixed_thresholds",&c->use_fixed_thresholds); 
  config_lookup_int(&cfg,"control.dynamic_masking_threshold",&tmp);
  c->dynamic_masking_threshold = tmp; 
  config_lookup_int(&cfg,"control.dynamic_masking_holdoff",&tmp); 
  c->dynamic_masking_holdoff = tmp; 
  config_lookup_int(&cfg,"device.enable_low_pass_to_trigger",&c->enable_low_pass_to_trigger); 



  const char * status_save = 0; 

  if (config_lookup_string(&cfg, "control.status_save_file", &status_save))
  {
    c->status_save_file = strdup(status_save); 
  }

  config_lookup_int(&cfg,"control.load_thresholds_from_status_file",&c->load_thresholds_from_status_file); 


  const char *spi = 0; 

  if (config_lookup_string(&cfg, "device.spi_device", &spi))
  {
    c->spi_device = strdup(spi); 
  }
 


  config_lookup_int(&cfg,"device.buffer_capacity", &c->buffer_capacity); 
  config_lookup_int(&cfg,"device.waveform_length", &c->waveform_length); 
  config_lookup_int(&cfg,"device.pretrigger", &c->pretrigger); 
  config_lookup_int(&cfg,"device.calpulser_state", &c->calpulser_state); 
  config_lookup_int(&cfg,"device.enable_trigout", &c->enable_trigout); 
  config_lookup_int(&cfg,"device.enable_extin", &c->enable_extin); 
  config_lookup_int(&cfg,"device.trigout_width", &c->trigout_width); 
  config_lookup_int(&cfg,"device.disable_trigout_on_exit", &c->disable_trigout_on_exit); 
  config_lookup_int(&cfg,"device.spi_clock", &c->spi_clock); 
  config_lookup_int(&cfg,"device.apply_attenuations", &c->apply_attenuations); 

  for (i = 0; i < BN_NUM_CHAN; i++) 
  {
      char buf[128]; 
      sprintf(buf,"device.attenuation.ch%d", i); 
      if (config_lookup_int(&cfg,buf, &tmp) )
      {
        c->attenuation[i]=tmp; 
      }
  }

  
  if (config_lookup_int(&cfg, "device.channel_read_mask", &tmp)) 
    c->channel_read_mask = tmp; 

  const char * cmd; 
  if (config_lookup_string(&cfg, "device.alignment_command", &cmd) )
  {
    c->alignment_command = strdup (cmd); 
  }

  const char * run_file ; 
  if (config_lookup_string( &cfg, "output.run_file", &run_file))
  {
    c->run_file = strdup(run_file); 
  }

  const char * output_directory ; 
  if (config_lookup_string( &cfg, "output.output_directory", &output_directory))
  {
    c->output_directory = strdup(output_directory); 
  }

  const char * copy_paths; 
  if (config_lookup_string( &cfg, "output.copy_paths_to_rundir", &copy_paths))
  {
    c->copy_paths_to_rundir = strdup(copy_paths); 
  }


  config_lookup_int(&cfg,"output.print_interval", &c->print_interval); 
  config_lookup_int(&cfg,"output.run_length", &c->run_length); 
  config_lookup_int(&cfg,"output.events_per_file", &c->events_per_file); 
  config_lookup_int(&cfg,"output.status_per_file", &c->status_per_file); 
  config_lookup_int(&cfg,"output.copy_configs", &c->copy_configs); 

  for (i = 0; i < BN_NUM_CHAN; i++)
  {
    char buf[128]; 
    sprintf(buf,"device.trig_delays.ch%d",i); 
    if (config_lookup_int(&cfg,buf,&tmp))
    {
      c->trig_delays[i] = tmp; 
    }
  }



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
  fprintf(f,"   // scaler goals for each beam, desired rate ( in Hz)\n"); 
  fprintf(f,"   scaler_goal = {\n"); 
  for (i = 0; i < BN_NUM_BEAMS; i++)
  {
    fprintf(f, "     beam%d : %g;\n", i, c->scaler_goal[i]); 
  }
  fprintf(f,"    };\n\n"); 

  fprintf(f,"   // fixed thresholds for each beam (in case of use_fixed_thresholds)\n"); 
  fprintf(f,"   fixed_threshold = {\n"); 
  for (i = 0; i < BN_NUM_BEAMS; i++)
  {
    fprintf(f, "     beam%d : %u;\n", i, c->fixed_threshold[i]); 
  }
  fprintf(f,"    };\n\n"); 



  fprintf(f,"   //the beams allowed to participate in the trigger\n"); 
  fprintf(f,"   trigger_mask = 0x%x;\n\n", c->trigger_mask);  

  fprintf(f,"   // the channels on the master allowed to participate in the trigger\n"); 
  fprintf(f,"   channel_mask = 0x%x;\n\n", c->channel_mask); 

  fprintf(f,"   // enable on-board dynamic masking\n"); 
  fprintf(f,"   enable_dynamic_masking = %d;\n\n", c->enable_dynamic_masking); 

  fprintf(f,"   // dynamic masking threshold\n"); 
  fprintf(f,"   dynamic_masking_threshold = %u;\n\n", c->dynamic_masking_threshold); 

  fprintf(f,"   // dynamic masking holdoff\n"); 
  fprintf(f,"   dynamic_masking_holdoff = %u;\n\n", c->dynamic_masking_holdoff); 

  fprintf(f,"   // use fixed thresholds (don't servo!) \n"); 
  fprintf(f,"   use_fixed_thresholds = %d;\n\n", c->use_fixed_thresholds); 


  fprintf(f,"   // pid loop proportional term\n"); 
  fprintf(f,"   k_p = %g;\n\n", c->k_p); 

  fprintf(f,"   // pid loop integral term\n"); 
  fprintf(f,"   k_i = %g;\n\n", c->k_i);

  fprintf(f,"   // pid loop differential term\n"); 
  fprintf(f,"   k_d = %g;\n\n", c->k_d);

  fprintf(f,"   // puts a floor on the thresholds\n"); 
  fprintf(f,"   min_threshold=%u;\n\n", c->min_threshold);

  fprintf(f,"   // max threshold increase per step \n"); 
  fprintf(f,"   max_threshold_increase=%u;\n\n", c->max_threshold_increase); 
  
  fprintf(f,"   //monitoring interval, for PID loop (in seconds)\n"); 
  fprintf(f,"   monitor_interval = %g;\n\n",c->monitor_interval); 

  fprintf(f,"   // software trigger interval (in seconds)\n"); 
  fprintf(f,"   sw_trigger_interval = %g;\n\n", c->sw_trigger_interval); 

  fprintf(f,"   // randomize sw trigger interval (using exponential distribution)\n"); 
  fprintf(f,"   randomize_sw_trigger = %d;\n\n", c->randomize_sw_trigger); 

  fprintf(f,"   //enable the phased trigger readout\n"); 
  fprintf(f,"   enable_phased_trigger = %d;\n\n",c->enable_phased_trigger); 

  /* fprintf(f, "//Which polarization to trigger on, 0=H, 1=V, higher values reserved for as-yet unimplemented combinations\n"); */
  /* fprintf(f, "trigger_polarization = %d\n\n", c->trigger_polarization); */
  fprintf(f, "   // Polarization for triggering, current options are \"H\", \"V\"\n");
  fprintf(f, "   // @see config_lookup_pol in beacon-cfg.c\n");
  fprintf(f, "   // @see beacon_trigger_polarization_t in beacondaq.h in libbeacon\n");
  fprintf(f, "   trigger_polarization = \"%s\";\n", beacon_trigger_polarization_name(c->trigger_polarization));

  fprintf(f,"   //delay for phased trigger to start\n"); 
  fprintf(f,"   secs_before_phased_trigger = %d;\n\n", c->secs_before_phased_trigger); 

  fprintf(f,"   //weight of fast scaler in pid loop\n"); 
  fprintf(f,"   fast_scaler_weight = %g;\n\n", c->fast_scaler_weight); 

  fprintf(f,"   //weight of slow scaler in pid loop\n"); 
  fprintf(f,"   slow_scaler_weight = %g;\n\n", c->slow_scaler_weight); 

  fprintf(f,"   //number of fast scalers to average\n"); 
  fprintf(f,"   n_fast_scaler_avg = %d;\n\n", c->n_fast_scaler_avg); 

  fprintf(f,"   //Whether or not to subtract off gated scalers\n"); 
  fprintf(f,"   subtract_gated = %d;\n\n", c->subtract_gated); 


  fprintf(f,"   //File to persist the status info (primarily for saving thresholds between restarts)\n") ;
  fprintf(f,"   status_save_file = \"%s\"\n\n", c->status_save_file); 

  fprintf(f,"   // load thresholds from status file on start.\n");  
  fprintf(f,"   load_thresholds_from_status_file=%d\n\n", c->load_thresholds_from_status_file); 


  fprintf(f,"};\n\n"); 

  fprintf(f,"// settings related to the acquisition\n"); 
  fprintf(f,"// Not all of these can be set without restarting\n"); 
  fprintf(f,"device: \n");
  fprintf(f,"{\n"); 
  fprintf(f,"  //spi devices, master first, requires restart to change\n"); 
  fprintf(f,"  spi_device = \"%s\"; \n\n", c->spi_device); 

  
  fprintf(f,"  // circular buffer capacity. In-memory storage in between acquisition and writing. Requires restart.\n"); 
  fprintf(f,"  buffer_capacity = %d;\n\n", c->buffer_capacity); 

  fprintf(f,"  //the length of a waveform, in samples. \n"); 
  fprintf(f,"  waveform_length = %d;\n\n", c->waveform_length); 

  fprintf(f,"  //the pretrigger window length, in hardware units\n"); 
  fprintf(f,"  pretrigger = %d;\n\n", c->pretrigger); 

  fprintf(f,"  //calpulser state, 0 (off) , 2 (baseline)  or 3 (calpulser)\n"); 
  fprintf(f,"  calpulser_state = %d;\n\n", c->calpulser_state); 

  fprintf(f,"  // Whether or not to enable the trigger output\n"); 
  fprintf(f,"  enable_trigout = %d;\n\n", c->enable_trigout); 

  fprintf(f,"  // Whether or not to enable external trigger input\n"); 
  fprintf(f,"  enable_extin = %d;\n\n", c->enable_extin); 

  fprintf(f,"  // The width of the trigger output in 40 ns intervals\n"); 
  fprintf(f,"  trigout_width = %d;\n\n", c->trigout_width); 

  fprintf(f,"  // Whether or not to disable the trigger output on exit\n"); 
  fprintf(f,"  disable_trigout_on_exit = %d;\n\n", c->disable_trigout_on_exit); 

  fprintf(f,"  //spi clock speed, MHz\n"); 
  fprintf(f,"  spi_clock = %d;\n\n", c->spi_clock); 
 
  fprintf(f,"  // True to apply attenuations \n"); 
  fprintf(f,"  apply_attenuations = %d;\n\n", c->apply_attenuations); 

  fprintf(f,"  // attenuation, per channel, if applied. \n"); 

  fprintf(f,"  attenuation =  {"); 
  for (i = 0; i < BN_NUM_CHAN; i++)
    fprintf(f,  "ch%d: %d;  " , i, c->attenuation[i]); 
  fprintf(f,"} ;\n\n"); 

  fprintf(f,"  //which channels to digitize\n"); 
  fprintf(f,"  channel_read_mask = 0x%x; \n\n", c->channel_read_mask); 

  fprintf(f,"  //command used to run the alignment program.\n"); 
  fprintf(f,"  alignment_command=\"%s\",\n\n", c->alignment_command); 

  fprintf(f,"  //channel trig delays (right now can be 0-3)\n"); 
  fprintf(f,"  trig_delays = {\n"); 
  for (i = 0; i < BN_NUM_CHAN; i++)
  {
    fprintf(f, "     ch%d : %u;\n", i, c->trig_delays[i]); 
  }

  fprintf(f,"  };\n\n"); 

  fprintf(f,"  //Enable the low pass to trigger.\n"); 
  fprintf(f,"  enable_low_pass_to_trigger = %d; \n\n", c->enable_low_pass_to_trigger); 

 
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

  fprintf(f,"  //Interval between polling SPI link for data. 0 to just sched_yield\n"); 
  fprintf(f,"  poll_usecs = %u;\n\n", c->poll_usecs); 

  fprintf(f,"  // Colon separated list of paths to copy (recursively) into run dir at start of run\n");
  fprintf(f,"  copy_paths_to_rundir = \"%s\";\n\n", c->copy_paths_to_rundir); 

  fprintf(f,"  //Whether or not to copy configs into run dir\n"); 
  fprintf(f,"  copy_configs = %d;\n", c->copy_configs); 

  fprintf(f,"};\n\n"); 

  return 0; 
}








