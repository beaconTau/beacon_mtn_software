#include <stdio.h> 
#include <string.h>
#include "beacon-cfg.h" 
#include "beacon-common.h" 
#include <fcntl.h> 
#include <sys/stat.h>
#include <signal.h>

/** Housekeeping Program. 
 * 
 *  - Polls housekeeping info and saves to file
 *  - also makes hk available to shared memory location 
 *  - with SIGUSR1, rereads config
 *
 */ 


static beacon_hk_cfg_t cfg; 
static char * config_file; 


static volatile int stop = 0; 
static beacon_hk_t * the_hk = 0;  



static int read_config() 
{


  int ret =  beacon_hk_config_read(config_file, &cfg); 
  beacon_hk_set_mate3_address(cfg.mate3_url, cfg.mate3_port); 

  if (!the_hk)//don't allow updating shared_hk
  {
    return ret + open_shared_hk(&cfg,0,&the_hk); 
  }

  return ret; 
}

static void signal_handler(int signal, siginfo_t * sig, void * v) 
{
  switch (signal)
  {
    case SIGUSR1: 
      read_config(); 
      break; 
    case SIGTERM: 
    case SIGUSR2: 
    case SIGINT: 
    default: 
      fprintf(stderr,"Caught deadly signal %d\n", signal); 
      stop =1; 
  }
}

static const char * mk_name(time_t t)
{
  struct tm * tim = gmtime(&t); 
  static char buf[1024]; 
  int ok = 0; 
  ok += mkdir_if_needed(cfg.out_dir); 

  sprintf(buf,"%s", cfg.out_dir); 
  ok += mkdir_if_needed(buf); 

  sprintf(buf,"%s/%04d", cfg.out_dir, 1900 + tim->tm_year); 
  ok += mkdir_if_needed(buf); 

  sprintf(buf,"%s/%04d/%02d", cfg.out_dir, 1900 + tim->tm_year, tim->tm_mon + 1); 
  ok += mkdir_if_needed(buf); 
  
  sprintf(buf,"%s/%04d/%02d/%02d", cfg.out_dir, 1900 + tim->tm_year, tim->tm_mon + 1, tim->tm_mday); 
  ok += mkdir_if_needed(buf); 

  if (!ok) 
  {
    sprintf(buf,"%s/%04d/%02d/%02d/%02d%02d%02d.hk.gz%s", cfg.out_dir, 1900 + tim->tm_year, tim->tm_mon + 1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec, tmp_suffix); 
    return buf; 
  }

  return 0; 
}





int main(int nargs, char ** args) 
{

  //set up signal handlers 
  
  sigset_t empty;
  sigemptyset(&empty); 
  struct sigaction sa; 
  sa.sa_mask = empty; 
  sa.sa_flags = 0; 
  sa.sa_sigaction = signal_handler; 

  sigaction(SIGINT,  &sa,0); 
  sigaction(SIGTERM, &sa,0); 
  sigaction(SIGUSR1, &sa,0); 
  sigaction(SIGUSR2, &sa,0); 


  /** Initialize the configuration */ 
  beacon_hk_config_init(&cfg); 

  /* If we were launched with an argument, use it as the config file */ 
  if (nargs > 1)
  {
    config_file = args[1]; 
  }
  else 
  {
    beacon_get_cfg_file(&config_file, BEACON_HK); 
  }



  if (read_config())
  {
    fprintf(stderr, "Could not set up properly\n"); 
  }


  time_t last = 0;



  gzFile outf = 0; 
  char * outf_name = 0; 
  

  while (!stop) 
  {
    lock_shared_hk(); 
    beacon_hk(the_hk); 
    unlock_shared_hk(); 

    time_t now = time(0); 

    if (now - last > cfg.max_secs_per_file) 
    {
      if (outf) do_close(outf, outf_name); 
      const char * fname = mk_name(now); 
      outf_name = strdup(fname); 
      outf = gzopen(fname,"w"); 
      last = now; 
    }

    beacon_hk_gzwrite(outf, the_hk);
    if (cfg.print_to_screen)
      beacon_hk_print(stdout, the_hk); 
    sleep(cfg.interval); 
  }



  return 0 ; 
}
