#include "beacon-cfg.h" 
#include "beacon-common.h" 
#include <stdio.h> 

int main(int nargs, char ** args) 
{
  beacon_hk_cfg_t cfg; 
  beacon_hk_config_init(&cfg); 

  char * config_name = 0; 

  beacon_get_cfg_file(&config_name, BEACON_HK); 
  beacon_hk_config_read(config_name, &cfg); 


  char shbuf[512]; 
  sprintf(shbuf,"/dev/shm/%s", cfg.shm_name); 

  FILE * f = fopen(shbuf,"r"); 

  beacon_hk_t hk; 
  fread(&hk, sizeof(hk),1, f); 

  fclose(f); 
  beacon_hk_print(stdout, &hk); 


  return 0; 

}
