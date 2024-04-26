/* Main Acquisition program for vPhase 
 *
 *
 * Cosmin Deaconu <cozzyd@kicp.uchicago.edu>
 *
 *
 * Design: 
 *
 * Despite running on a single(at least for the first iteration) processor, the
 * work is divided into multiple threads to reduce latency of critical tasks. 
 *
 * Right now the threads are: 
 *
 *  - The main thread: reads the config, sets up, tears down, and waits for
 *  signals. 
 *
 * - Acquisition thread - takes the data and reads it
 * 
 * - A monitoring thread, reads in statuses, send software_triggers, and does the pid loops 
 *   (not sure if this should be merged into the acquisition thread...) 
 *
 * - A write thread, which writes to disk and screen.
 *
 * The config is read on startup. Right now, the configuration cannot be reloaded
 * without a restart.
 *
 **/ 



/************** Includes **********************************/

#include "beacon-common.h"
#include "beacon-cfg.h"
#include "beacon-buf.h" 
#define _BEACON_
#include "flower8.h"
#include <pthread.h> 
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h> 
#include <fcntl.h> 
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <inttypes.h> 
#include <math.h> 


/************** Structs /Typedefs ******************************/



/* This is what is stored within the acquisition buffer 
 *
 * It's a bit wasteful... we allocate space for all buffers even though
 * we often (usually? )won't have all of them. 
 *
 * Will see if this is a problem. 
 **/ 
typedef struct acq_buffer
{
  int nfilled; 
  beacon_event_t event;
  beacon_header_t header; 
} acq_buffer_t;


//coinc servo state for flower
typedef struct flower8_coinc_servo_state
{
  float value[BN_NUM_CHAN]; 
  float last_value[BN_NUM_CHAN]; 
  float error[BN_NUM_CHAN]; 
  float last_error[BN_NUM_CHAN]; 
  float sum_error[BN_NUM_CHAN]; 
} flower8_coinc_servo_state_t; 

//phased servo state for flower
typedef struct flower8_phased_servo_state
{
  float value[BN_NUM_BEAMS]; 
  float last_value[BN_NUM_BEAMS]; 
  float error[BN_NUM_BEAMS]; 
  float last_error[BN_NUM_BEAMS]; 
  float sum_error[BN_NUM_BEAMS]; 
} flower8_phased_servo_state_t; 


/* this is what is stored within the monitor buffer */ 
typedef struct monitor_buffer
{
  beacon_status_t status; //status before
  float coinc_thresholds[BN_NUM_CHAN]; //thresholds when written 
  flower8_coinc_servo_state_t servo; 
  float phased_thresholds[BN_NUM_BEAMS]; //thresholds when written 
  flower8_phased_servo_state_t servo; 
} monitor_buffer_t; 

/**************Static vars *******************************/

/* The configuration state */ 
static beacon_acq_cfg_t config; 


/* Mutex protecting the configuration */
static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER; 

/* The device */
static flower8_bouquet_t * device;

/* Acquisition thread handles */ 
static pthread_t the_acq_thread; 

/*Buffer for acq thread */ 
static beacon_buf_t* acq_buffer = 0; 

/*Buffer for monitor thread */ 
static beacon_buf_t* mon_buffer = 0; 

/* Monitor thread handle */ 
static pthread_t the_mon_thread; 

/* Write thread handle */ 
static pthread_t the_wri_thread; 


static int status_save_fd = -1; 
static beacon_status_t * saved_status = 0; 

/* used for exiting */ 
static volatile int die; 


//gain codes
uint8_t codes[2][BN_NUM_CHAN] = {0};     

/************** Prototypes *********************
 Brief documentation follows. More detailed documentation
 at implementation.
 */


static int run_number; 


// this sets everything up (opens device, starts threads, signal handlers, etc. ) 
//
//
const int SETUP_FAILED = 1; 
const int SETUP_TRY_AGAIN_LATER = 2; 
static int setup(); 
// this cleans up 
static int teardown(); 

//this reads in the config. Some things may be changed later. 
static int read_config(int first_time); 


// call this when we need to stop
static void fatal(); 

static void signal_handler(int, siginfo_t *, void *); 

/* Acquisition thread */ 
static void * acq_thread(void * p);

/* Monitor thread */ 
static void * monitor_thread(void * p); 

/* Write thread */ 
static void * write_thread(void * p ); 

static float clamp(float val, float min, float max) 
{
  if (val > max) return max; 
  if (val < min) return min; 
  return val; 
}

/* The main function... not too much here 
 *   Just check if it's time to pack our bags and let another run take care of things. 
 * */ 
int main(int nargs, char ** args) 
{


  int setup_return = setup(); 

  if (setup_return == SETUP_FAILED) 
  {
    return 1; //this is bad
  }

  if (setup_return == SETUP_TRY_AGAIN_LATER) 
  {
    sleep(5); 
    return 0; 
  }

  struct timespec start; 
  clock_gettime(CLOCK_MONOTONIC_COARSE, &start); 


  struct timespec now; 
  while(!die) 
  {
    clock_gettime(CLOCK_MONOTONIC_COARSE, &now); 
    if ( now.tv_sec - start.tv_sec > config.run_length) 
    {
      fatal(); 
    }

    usleep(500000);  //500 ms 

    sched_yield();
  }

  int teardown_return = teardown(); 

  return teardown_return; 
}



/*** Acquistion thread 
 *
 * This will wait for data, then record and it and put it into a memory buffer, awaiting to be written to disk. 
 *
 *
 ***/ 
void * acq_thread(void *v) 
{


  while(!die) 
  {
   /* grab a buffer to fill */ 
    acq_buffer_t * mem = (acq_buffer_t*) beacon_buf_getmem(acq_buffer); 
    mem->nfilled = 0; //nothing filled

    while (!mem->nfilled && !die) 
    {
      mem->nfilled = !beacon_wait_for_and_fill_event(device, &mem->header, &mem->event, 10); 
    }
    beacon_buf_commit(acq_buffer); // we filled it 
  }

  return 0; 
}





static struct drand48_data sw_rand; 
static double get_next_sw_trig_interval()
{
  if (!config.sw_trigger_interval) return 0; 
  if (config.randomize_sw_trigger)
  {
    double u = 0; 
    while (u==1 || u==0)  //make sure we don't have 0 or 1
    {
      drand48_r(&sw_rand,&u); 
    }

    return -log(u)*config.sw_trigger_interval; 
  }

  return config.sw_trigger_interval; 

}

static void servo_state_print(FILE *f , const flower8_servo_state_t * st) 
{
 fprintf(f,"========================FLOWR8 SERVO STATE=============\n"); 
 fprintf(f,"  ch  |  val |  lastval  | err  |  last_err |  sumerr\n"); 
 fprintf(f,"------------------------------------------------------\n"); 
 for (int i = 0; i < BN_NUM_CHAN; i++) 
 {
   fprintf(stderr,"  %d  |%0.3f | %0.3f  | %0.3f  | %0.3f  | %0.3f\n", 
       i, st->value[i], st->last_value[i], st->error[i], st->last_error[i], st->sum_error[i]); 
 }
}

static void update_flower_coinc_servo_state(flower8_coinc_servo_state_t *st, const beacon_status_t * ds) 
{

  float w1 = config.weight1Hz; 
  float wo = 1-w1; 


  double ofactor = ds->scaler_type == 1 ?  100 : 0.1; //
  
  for (int i = 0; i < BN_NUM_CHAN; i++)
  {

    float val =  wo * ofactor*ds->channel_servo_scalers[i][SCALER_VARIABLE]+ w1  * ds->channel_servo_scalers[i][SCALER_1HZ]; 
    st->last_value[i] = st->value[i]; 
    st->value[i] = val; 
    st->last_error[i] = st->error[i]; 
    st->error[i] = (val-config.channel_scaler_goal[i]); 
    st->sum_error[i] += st->error[i]; 
  } 
}

static void update_flower_phased_servo_state(flower8_phased_servo_state_t *st, const beacon_status_t * ds) 
{

  float w1 = config.weight1Hz; 
  float wo = 1-w1; 


  double ofactor = ds->scaler_type == 1 ?  100 : 0.1; //
  
  for (int i = 0; i < BN_NUM_BEAMS; i++)
  {

    float val =  wo * ofactor*ds->beam_servo_scalers[i][SCALER_VARIABLE]+ w1  * ds->beam_servo_scalers[i][SCALER_1HZ]; 
    st->last_value[i] = st->value[i]; 
    st->value[i] = val; 
    st->last_error[i] = st->error[i]; 
    st->error[i] = (val-config.beam_scaler_goal[i]); 
    st->sum_error[i] += st->error[i]; 
  } 
}
/***********************************************************************
 * Monitor thread
 *
 * This will periodically grab the status / adjust thresholds / send software triggers
 *
 *  The PID loops lies within here. 
 ***************************************************************************************/
void * monitor_thread(void *v) 
{

  //The start time 
  struct timespec start; 
  clock_gettime(CLOCK_MONOTONIC, &start); 

  //seed the rng for randomizing sw trigger
  srand48_r(start.tv_nsec, &sw_rand); 

  // this keeps track of when we sent the last sw trigger
  struct timespec last_sw_trig = { .tv_sec = 0, .tv_nsec = 0};

  //this keeps track of the last time we monitored
  struct timespec last_mon = { .tv_sec = 0, .tv_nsec = 0}; //dont' read scalers yet! 

  double sw_trig_interval =  get_next_sw_trig_interval(); 

  flower8_coinc_servo_state_t coinc_servo = {0};
  flower8_phased_servo_state_t phased_servo = {0};


  while(!die) 
  {
    //figure out the current time
    struct timespec now; 
    clock_gettime(CLOCK_MONOTONIC, &now); 



 
    /// Figure out how long it's been since last time we monitored and sent a software trigger
    float diff_mon = timespec_difference_float(&now, &last_mon); 
    float diff_swtrig = timespec_difference_float(&now, &last_sw_trig); 

    //////////////////////////////////////////////////////
    //read the status, and react to it 
    // herein lies the PID loop and all those wonderful things
    //////////////////////////////////////////////////////
    if (config.monitor_interval &&  diff_mon > config.monitor_interval)
    {
      monitor_buffer_t mb; 
      beacon_status_t *st = &mb.status;

      beacon_fill_status(device, st); 
     
      //beacon_status_print(stdout,st); 
      //maybe put if enable_coinc to save the trouble of servoing an off trigger type
      update_flower_coinc_servo_state(&coinc_servo, st); 

      for (int ichan = 0; ichan < BN_NUM_CHAN; ichan++)
      {

        if (config.use_fixed_thresholds) 
        {
          mb.channel_thresholds[ichan] = config.fixed_channel_threshold[ichan]; 

        }
        else
        {
          // modify threshold 
          double dthreshold =   config.k_p * coinc_servo.error[ichan] + config.k_i * coinc_servo.sum_error[ichan] + config.k_i * (coinc_servo.error[ichan] - coinc_servo.last_error[ichan]);
          
          //cap the threshold increase at each step 
          if (dthreshold > config.max_threshold_increase) dthreshold = config.max_threshold_increase;

          mb.channel_thresholds[ichan]+= dthreshold;

          if(mb.channel_thresholds[ichan] < config.min_channel_threshold){
            mb.channel_thresholds[ichan] = config.min_channel_threshold;
          }

        }

        mb.status.channel_servo_thresholds[ichan] = mb.channel_thresholds[ichan]; 
        mb.status.channel_trig_thresholds[ichan] = clamp(mb.channel_thresholds[ichan] / config.coinc_servo_scaler_frac,4,120); 
      }

      //apply the thresholds 
      flower8_set_thresholds(device, mb.status.channel_trig_thresholds, mb.status.channel_servo_thresholds, 0xff); 
      
      //copy over the current control status 
      if (!config.use_fixed_thresholds) memcpy(&mb.coinc_servo, &coinc_servo, sizeof(coinc_servo)); 
      


      update_flower_phased_servo_state(&phased_servo, st); 
      for (int ibeam = 0; ibeam < BN_NUM_BEAMS; ibeam++)
      {

        if (config.use_fixed_thresholds) 
        {
          mb.beam_thresholds[ibeam] = config.fixed_beam_threshold[ibeam]; 

        }
        else
        {
          // modify threshold 
          double dthreshold =   config.k_p_p * phased_servo.error[ibeam] + config.k_i_p * phased_servo.sum_error[ibeam] + config.k_i_p * (phased_servo.error[ibeam] - phased_servo.last_error[ibeam]);
          
          //cap the threshold increase at each step 
          //if (dthreshold > config.max_threshold_increase) dthreshold = config.max_threshold_increase;

          mb.beam_thresholds[ibeam]+= dthreshold;

          if(mb.phased_thresholds[ibeam] < config.min_phased_threshold){
            mb.phased_thresholds[ibeam] = config.min_phased_threshold;
          }

        }

        mb.status.beam_servo_thresholds[ibeam] = mb.beam_thresholds[ibeam]; 
        mb.status.beam_trig_thresholds[ibeam] = clamp(mb.beam_thresholds[ibeam] / config.phased_servo_scaler_frac,100,4095); 
      }

      //apply the thresholds 
      flower8_set_coinc_thresholds(device, mb.status.channel_trig_thresholds, mb.status.channel_servo_thresholds, 0xff); 
      flower8_set_phased_thresholds(device,mb.status.beam_trig_thresholds,mb.status.beam_servo_thresholds);
      //copy over the current control status 
      if (!config.use_fixed_thresholds) 
      {
        memcpy(&mb.coinc_servo, &coinc_servo, sizeof(coinc_servo)); 
        memcpy(&mb.phased_servo, &phased_servo, sizeof(phased_servo)); 
      }

      beacon_buf_push(mon_buffer, &mb);
      memcpy(&last_mon,&now, sizeof(now)); 
      diff_mon = 0; 
    }

    if (sw_trig_interval && diff_swtrig > sw_trig_interval)
    {
      flower8_force_trigger(device); 
      memcpy(&last_sw_trig,&now, sizeof(now)); 
      diff_swtrig = 0;
      sw_trig_interval = get_next_sw_trig_interval(); 
    }

    //now figure out how long to sleep 
    //
    float how_long_to_sleep = 0.1; //don't sleep longer than 100 ms 

    if (config.monitor_interval && config.monitor_interval - diff_mon  < how_long_to_sleep) how_long_to_sleep = config.monitor_interval - diff_mon; 
    if (sw_trig_interval && sw_trig_interval - diff_swtrig  < how_long_to_sleep) how_long_to_sleep = sw_trig_interval - diff_swtrig; 

    usleep(how_long_to_sleep * 1e6); 
  }

  return 0; 
}


const char * subdirs[] = {"event","header","status","aux","cfg"}; 
const int nsubdirs = sizeof(subdirs) / sizeof(*subdirs); 

//this makes the necessary directories for a time 
//returns 0 on success. 
static int make_dirs_for_output(const char * prefix) 
{ 
  char buf[strlen(prefix) + 512];  

  //check to see that prefix exists and is a directory
  if (mkdir_if_needed(prefix))
  {
    fprintf(stderr,"Couldn't find %s or it's not a directory. Bad things will happen!\n",prefix); 
    return 1; 
  }


  int i;

  for (i = 0; i < nsubdirs; i++)
  {
    snprintf(buf,sizeof(buf), "%s/%s",prefix,subdirs[i]); 
    if (mkdir_if_needed(buf))
    {
        fprintf(stderr,"Couldn't make %s. Bad things will happen!\n",buf); 
        return 1; 
    }
  }

  return 0; 
}


static char * output_dir = 0;

void copy_configs() 
{

  char * cfgpath = 0; 
  char bigbuf[1024];
  if (!output_dir) return; 

  beacon_program_t prog;
  for (prog = BEACON_ACQ; prog <= BEACON_COPY; prog++)
  {
    beacon_get_cfg_file(&cfgpath, prog); 
    snprintf(bigbuf,sizeof(bigbuf), "cp --backup=simple %s %s/cfg", cfgpath, output_dir); 
    system(bigbuf); 
  }

}

/** Will write output to disk and some status info to screen */ 
void * write_thread(void *v) 
{
  time_t start_time = time(0);
  time_t last_print_out =start_time ; 

  char bigbuf[strlen(config.output_directory)+512];

  int    data_file_size = 0;
  int    header_file_size = 0;
  int    status_file_size =0;

  gzFile data_file = 0 ; 
  gzFile header_file = 0 ; 
  gzFile status_file  = 0 ; 
  char * data_file_name = 0; 
  char * header_file_name = 0; 
  char * status_file_name = 0; 

  acq_buffer_t *events= 0; 
  monitor_buffer_t *mon= 0;

  beacon_status_t * last_status = (saved_status && saved_status != MAP_FAILED)  ? saved_status : malloc(sizeof(beacon_status_t)); 

  flower8_coinc_servo_state_t last_coinc_servo; 
  flower8_phased_servo_state_t last_phased_servo; 


  beacon_fill_status(device, last_status); 

  
  int num_events = 0; 
  int ntotal_events = 0;

  snprintf(bigbuf, sizeof(bigbuf),"%s/run%d/", config.output_directory, run_number); 
  if (make_dirs_for_output(bigbuf))
  {
      fatal(); 
  }
  output_dir = strdup(bigbuf); 

  if (config.copy_configs) 
  {
    copy_configs(); 
  }


  //Copy any other things we want to the run directory 
  char * thing_to_copy; 
  char * tmp_str = strdup(config.copy_paths_to_rundir); 
  char * save_ptr = 0;
  thing_to_copy = strtok_r(tmp_str,":",&save_ptr); 
  while (thing_to_copy!=NULL)
  {
     snprintf(bigbuf,sizeof(bigbuf), "cp -r %s %s/aux", thing_to_copy, output_dir); 
     system(bigbuf); 
     thing_to_copy = strtok_r(NULL,":",&save_ptr);
  }
  free(tmp_str); 

 
  while(1) 
  {
    time_t now; 
    time(&now); 
    int have_data= 0; 
    int have_status = 0;
    
    size_t occupancy = beacon_buf_occupancy(acq_buffer); 
    if (beacon_buf_occupancy(acq_buffer))
    {
        events = beacon_buf_pop(acq_buffer, events); 
        num_events += events->nfilled; 
        ntotal_events += events->nfilled;
        have_data=1;
    }

    if (beacon_buf_occupancy(mon_buffer)) 
    {
      mon = beacon_buf_pop(mon_buffer, mon); 
      have_status=1; 
    }
    
    //print something to screen
    if (config.print_interval > 0 && now - last_print_out > config.print_interval)  
    {
      printf("---------after %u seconds-----------\n", (unsigned) (now - start_time)); 
      printf("  total events written: %d\n", ntotal_events); 
      printf("  write rate:  %g Hz\n", (num_events == 0) ? 0. :  ((float) num_events) / (now - last_print_out)); 
      printf("  write buffer occupancy: %zu \n", occupancy); 
      beacon_status_print(stdout, last_status); 
      if (!config.use_fixed_thresholds) 
      {
        coinc_servo_state_print(stdout, &last_coinc_servo); 
        phased_servo_state_print(stdout, &last_phased_servo); 
      }
      last_print_out = now; 
      num_events = 0;
    }


    if (!have_data && !have_status)
    {
      if (die) 
      {
        if (data_file)  do_close(data_file,data_file_name); 
        if (header_file)  do_close(header_file, header_file_name); 
        if (status_file)  do_close(status_file, status_file_name); 

        break; 
      }

      //no data, so sleep a lot
      usleep(50000);  //50 ms 
      continue; 
    }
   
    //make sure we have the right dirs
  
        
    if (have_data)
    {

      if (events->nfilled)
      {

        if (!data_file || data_file_size >= config.events_per_file)
        {
          if (data_file) do_close(data_file, data_file_name); 
          snprintf(bigbuf,sizeof(bigbuf),"%s/run%d/event/%"PRIu64".event.gz%s", config.output_directory,run_number,  events->event.event_number, tmp_suffix ); 
          data_file = gzopen(bigbuf,"w");  //TODO add error check
          data_file_name = strdup(bigbuf); 
          data_file_size = 0; 
        }

        if (!header_file || header_file_size >= config.events_per_file)
        {
          if (header_file) do_close(header_file, header_file_name); 
          snprintf(bigbuf,sizeof(bigbuf),"%s/run%d/header/%"PRIu64".header.gz%s", config.output_directory,run_number, events->header.event_number, tmp_suffix ); 
          header_file = gzopen(bigbuf,"w");  //TODO add error check
          header_file_name = strdup(bigbuf); 
          header_file_size = 0; 
        }
       
        beacon_event_gzwrite(data_file, &events->event); 
        beacon_header_gzwrite(header_file, &events->header); 
        data_file_size++; 
        header_file_size++; 
      }
    }

    if (have_status)
    {
      if (!status_file || status_file_size >= config.status_per_file)
      {
        if (status_file) do_close(status_file, status_file_name); 
        snprintf(bigbuf,sizeof(bigbuf),"%s/run%d/status/%u.status.gz%s", config.output_directory, run_number,  (unsigned) now, tmp_suffix); 
        status_file = gzopen(bigbuf,"w");  //TODO add error check
        status_file_name = strdup(bigbuf); 
        status_file_size = 0; 
      }

      memcpy(last_status, &mon->status, sizeof(*last_status)); 
      if (!config.use_fixed_thresholds) 
      {
        memcpy(&last_coinc_servo, &mon->coinc_servo, sizeof(last_coinc_servo)); 
        memcpy(&last_phased_servo, &mon->phased_servo, sizeof(last_phased_servo)); 
      }

      //update the mmaped file if necessary 
      if ( saved_status == last_status) msync(saved_status, sizeof(beacon_status_t),MS_ASYNC); 


      //write out the file 
      beacon_status_gzwrite(status_file, &mon->status); 

      status_file_size++; 
    }
    
    //had data, so sleep just a little (unless occupancy is too high) 
    if (beacon_buf_occupancy(acq_buffer) < config.buffer_capacity/3)  usleep(25000);  //25 ms 
  }

  if (last_status != saved_status)  free(last_status); 

  return 0; 

}


void fatal()
{
  die = 1; 


}

void signal_handler(int signal, siginfo_t * sig, void * v) 
{
  switch (signal)
  {
    case SIGUSR1: 
      read_config(0); 
      break; 
    case SIGTERM: 
    case SIGUSR2: 
    case SIGINT: 
    default: 
      fprintf(stderr,"Caught deadly signal %d\n", signal); 
      fatal(); 
  }
}

const char * tmp_run_file = "/tmp/.runfile"; 


/* This is to avoid repeating code
 * twice for device things that may be changed on a reread */ 
static int configure_device() 
{

  flower8_set_buffer_length(device, config.waveform_length); 

  flower8_set_variable_scaler_speed(device, config.use_100Hz_scalers ? FLOWER8_SCAL_100Hz : FLOWER8_SCAL_100mHz); 

  double dly = config.pps_delay; 
  if (dly < 0)
  {
    fprintf(stderr,"Negative delays not implemented yet\n"); 
    dly = 0; 
  }

  uint32_t delay_cycles = dly / 40; 
  flower8_set_delayed_pps_delay(device, delay_cycles); 

  //setup the trigger_mode
  flower8_trigger_enables_t ten = { .enable_phased=config.enable_phased, .enable_coinc = config.enable_coinc, .enable_pps = config.enable_pps}; 

  flower8_set_trigger_enables(device, ten);

  //set up the pretrigger
  flower8_set_pretrigger(device, (uint8_t) config.pretrigger & 0xf);


  flower8_trigger_config_t trig_cfg = {.vpp_mode = config.vpp_mode, .window = config.coinc_window, .num_coinc = config.ncoinc}; 
  flower8_configure_trigger(device, trig_cfg); 

  flower8_set_coinc_trigger_mask(device, config.coinc_trigger_mask); 
  flower8_set_phased_trigger_mask(device, config.phased_trigger_mask_lower, config.phased_trigger_mask_upper); 

  return 0; 
}

static int setup()
{
  //let's do signal handlers. 
  //My understanding is that the main thread gets all the signals it doesn't block first, so we'll just handle it that way. 

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

  //Read configuration 
  int config_return = read_config(1); 

  if (config_return == SETUP_TRY_AGAIN_LATER) 
  {

    return SETUP_TRY_AGAIN_LATER;

  }
  /* open up the run number file, read the run, then increment it and save it
   * This is a bit fragile, potentially, so maybe revisit. 
   * */ 
  FILE * run_file = fopen(config.run_file, "r"); 
  fscanf(run_file, "%d\n", &run_number); 
  fclose(run_file); 

  run_file = fopen(tmp_run_file,"w"); 
  fprintf(run_file,"%d\n", run_number+1); 
  fclose(run_file); 
  rename(tmp_run_file, config.run_file); 


  //open the devices and configure properly
  // the gpio state should already have been set 
  int Mindex = config.swap_boards ? 1 : 0; 
  int Sindex = config.swap_boards ? 0 : 1; 
  flower8_dev_t * M = flower8_open(config.spi_device[Mindex], config.spi_enable, config.gpio_int[Mindex],FLOWER8_ENABLE_LOCKING); 
  flower8_dev_t * S = flower8_open(config.spi_device[Sindex], 0, config.gpio_int[Sindex], FLOWER8_ENABLE_LOCKING); 


  //set gains 
  for (int ibd = 0; ibd <2; ibd++) 
  {
    flower8_dev_t* bd = ibd == 0 ? M : S; 
    if (!bd) continue; 
    uint32_t equalize_mask = 0; 
    for (int i = 0; i < BN_NUM_CHAN; i++)
    {
      if (config.gain_codes[ibd][i] >= 0 && config.gain_codes[ibd][i] < FLOWER8_GAIN_TOO_HIGH)  
      {
        equalize_mask |= (1 << i); 
      }
    }

    if (equalize_mask != 0xff) 
    {
      flower8_equalize(bd, config.target_rms, &codes[ibd][0], equalize_mask | FLOWER8_EQUALIZE_VERBOSE); 
    }

    for (int i = 0; i < BN_NUM_CHAN; i++) 
    {
      if (config.gain_codes[ibd][i] >= 0 && config.gain_codes[ibd][i] < FLOWER8_GAIN_TOO_HIGH)  
      {
        codes[ibd][i] = config.gain_codes[ibd][i]; 
      }

      flower8_set_gains(bd, codes[ibd]); 
    }



  }



  device = flower8_bouquet_prepare(M,S); 


  if (!device)
  {
    //that would not be really good 
    fprintf(stderr,"Couldn't open device. Aborting! \n"); 
    exit(1); 
  }

  //If we are loading the thresholds from the status file,
  //we'll mmap the file and copy thresholds over there. 
  if (config.load_thresholds_from_status_file) 
  {

    status_save_fd = open(config.status_save_file, O_CREAT | O_RDWR,00755); 

    if (status_save_fd == -1) 
    {
      fprintf(stderr,"Could not open %s\n", config.status_save_file); 
    }
    else
    {

      //seek to the end to determine the size
      size_t file_size = lseek(status_save_fd, 0, SEEK_END); 

      //rewind the file 
      lseek(status_save_fd,0,SEEK_SET); 


      // If it's not the right size (either 0, or maybe an old version of the struct,
      // truncate it) 
      if (file_size != sizeof(beacon_status_t))
      {
        ftruncate(status_save_fd, sizeof(beacon_status_t)); 
      }

      //mmap it to save_status
      saved_status = mmap(0, sizeof(beacon_status_t), PROT_READ | PROT_WRITE, MAP_SHARED, status_save_fd, 0); 

      // if successful and right size, set the thresholds. Though these might get overriden by fixed thresholds... 
      if (saved_status!=MAP_FAILED && file_size == sizeof(beacon_status_t))
      {
        flower8_set_thresholds(device, saved_status->channel_trig_thresholds, saved_status->channel_servo_thresholds, 0xff); 
      }
    }
  }

  uint64_t run64 = run_number; 

  //Set event number offset
  flower8_set_event_number_offset(device, run64 * 1000000000); 

  configure_device(); 



  // set up the buffers
  acq_buffer = beacon_buf_init( config.buffer_capacity, sizeof(acq_buffer_t)); 
  mon_buffer = beacon_buf_init( config.buffer_capacity, sizeof(monitor_buffer_t)); 


  // set up the threads 
 
  pthread_create(&the_mon_thread, 0, monitor_thread, 0); 
  pthread_create(&the_acq_thread, 0, acq_thread, 0); 
  pthread_create(&the_wri_thread, 0, write_thread, 0); 
  

  //increase priority of acquistion thread
  if (config.realtime_priority > 0) 
  {
    struct sched_param sp;
    sp.sched_priority = config.realtime_priority; 
    pthread_setschedparam(the_acq_thread, SCHED_FIFO, &sp); 
  }


  return 0;
}

int teardown() 
{
  pthread_join(the_acq_thread,0); 
  pthread_join(the_mon_thread,0); 
  pthread_join(the_wri_thread,0); 




  flower8_bouquet_discard(device,1); 


  //munmap the persistent status if necessary 
  if (saved_status != 0 && saved_status != MAP_FAILED) 
  {
    munmap (saved_status, sizeof(beacon_status_t));
    close(status_save_fd); 
  }

  return 0;
}


int read_config(int first_time)
{

  char * cfgpath = 0;  
  

  pthread_mutex_lock(&config_lock); 

  if (first_time)
  {
    beacon_acq_config_init(&config); 
  }

  if (!beacon_get_cfg_file(&cfgpath, BEACON_ACQ))
  {
    printf("Using config file: %s\n", cfgpath); 
  }
 
  

  beacon_acq_config_read( cfgpath, &config); 

  pthread_mutex_unlock(&config_lock); 





  if (!first_time) 
  {

    configure_device(); 

    //rewrite run number in case we are using a different file 
    FILE * run_file = fopen(tmp_run_file,"w"); 
    fprintf(run_file,"%d\n", run_number+1); 
    fclose(run_file); 
    rename(tmp_run_file, config.run_file); 
  }


  if (!first_time && config.copy_configs)
  {
    copy_configs(); 
  }


  free(cfgpath); 

  return 0;
}

