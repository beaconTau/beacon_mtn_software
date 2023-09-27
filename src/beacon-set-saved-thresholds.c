#include <stdio.h> 
#include <stdlib.h> 
#include "beacon.h" 
#include "beacon-cfg.h" 
#include "beacon-common.h" 

/** This program overwrites 
 *  the saved status file for Acqd, allowing you to "seed" 
 *  Acqd with desired thresholds */ 

int main (int nargs, char ** args) 
{
  if (nargs != 2 && nargs != BN_NUM_CHAN +1) 
  {
    fprintf(stderr,"Usage:\tbeacon-set-saved-thresolds thresh\n\t\tbeacon-set-saved-thresholds thresh0 thresh1 thresh2 thresh3 ... thresh7\n"); 
    return 1; 
  }

  beacon_acq_cfg_t cfg; 
  beacon_acq_config_init(&cfg); 
  char * cfgpath; 
  if (!beacon_get_cfg_file(&cfgpath, BEACON_ACQ))
  {
    beacon_acq_config_read(cfgpath, &cfg);
  }

  beacon_status_t save; 
  
  int thresh = atoi(args[1]); 
  for (int ichan = 0; ichan < BN_NUM_CHAN; ichan++)
  {
    if (nargs > 2 && ichan > 0) 
    {
      thresh = atoi(args[1+ichan]); 
    }
    save.channel_trig_thresholds[ichan] = thresh; 
  }

  FILE * f = fopen(cfg.status_save_file,"w"); 
  fwrite(&save, sizeof(save),1,f); 
  fclose(f); 

  return 0; 
}
  
