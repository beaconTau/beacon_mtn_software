/* 
 * \file nuphase-startup.c
 *
 * Startup program. This does absolutely nothing except call another command. 
 *
 **/ 


#include "nuphasehk.h" 
#include "nuphase-cfg.h" 
#include "nuphase-common.h" 
#include <stdio.h> 
#include <string.h> 
#include <signal.h>


static nuphase_start_cfg_t cfg; 
static int read_config()
{
  char * config_file; 
  if (!nuphase_get_cfg_file(&config_file, NUPHASE_STARTUP))
  {
    printf("Using config file: %s\n", config_file); 
    nuphase_start_config_read(config_file, &cfg); 
    free(config_file);
  }

  return 0; 
}


static void signal_handler(int signal, siginfo_t * sig, void * v) 
{
  switch (signal)
  {
    case SIGUSR1: 
      read_config(); 
      break; 
    default: 
      fprintf(stderr,"Caught signal %d\n", signal); 
  }
}



int main (int nargs, char ** args) 
{

  //set up signal handlers 
  sigset_t empty;
  sigemptyset(&empty); 
  struct sigaction sa; 
  sa.sa_mask = empty; 
  sa.sa_flags = 0; 
  sa.sa_sigaction = signal_handler; 
  sigaction(SIGUSR1, &sa,0); 



  //read the config file
  nuphase_start_config_init(&cfg); 
  read_config(); 
  


  //make the output directory
  gzFile out = 0; 
  char * out_name = 0;

  time_t start_t; 
  time(&start_t); 

  if(!mkdir_if_needed(cfg.out_dir)) 
  {
    char buf[1024]; 
    struct tm * tim = gmtime(&start_t); 
    sprintf(buf,"%s/%04d_%02d_%02d_%02d%02d%02d.hk.gz%s", cfg.out_dir, 1900 + tim->tm_year, tim->tm_mon + 1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec,tmp_suffix); 
    out = gzopen(buf,"w"); 
    out_name = strdup(buf); 
  }

  nuphase_hk_t hk; 


  printf("Final state: \n"); 

  nuphase_hk(&hk); 
  nuphase_hk_print(stdout,&hk); 

  if (out) 
  {
    nuphase_hk_gzwrite(out, &hk); 
  }


  if (strlen(cfg.reconfigure_fpga_cmd))
  {
    printf("Reconfiguring FGPAs with command: %s\n", cfg.reconfigure_fpga_cmd); 
    system(cfg.reconfigure_fpga_cmd);
  }

  if (strlen(cfg.set_attenuation_cmd))
  {
    char cmd[1024]; 
    sprintf(cmd,"%s %g", cfg.set_attenuation_cmd, cfg.desired_rms); 
    printf("Waiting for everything to be on\n"); 
    printf("Running: %s\n", cmd); 
    system(cmd); 
  }


  if (out) //take another reading here 
  {
    nuphase_hk(&hk); 
    nuphase_hk_gzwrite(out, &hk); 
    do_close(out, out_name); 
  }

  return 0; 
}
