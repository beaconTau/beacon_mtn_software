#ifndef _BEACON_COMMON_H 
#define _BEACON_COMMON_H 
#include <time.h> 
#include <zlib.h>

/** Various common things used by multiple programs */ 



/** Get the difference between two timespecs as a float */ 
float timespec_difference_float(const struct timespec * a, const struct timespec * b); 


typedef enum beacon_program
{
  BEACON_STARTUP, 
  BEACON_HK, 
  BEACON_ACQ, 
  BEACON_COPY 
}  beacon_program_t; 


#define CONFIG_DIR_ENV "BEACON_CONFIG_DIR" 
#define CONFIG_DIR_ACQ_NAME "acq.cfg" 
#define CONFIG_DIR_COPY_NAME "copy.cfg" 
#define CONFIG_DIR_HK_NAME "hk.cfg" 
#define CONFIG_DIR_STARTUP_NAME "startup.cfg" 
#define CONFIG_REREAD_SIGNAL SIGUSR1; 


int beacon_get_cfg_file(char ** name, beacon_program_t program); 


/* a bunch of directory making things */ 



/* Makes a directory if not already there 
 * If 0 is returned, it should be there */ 
int mkdir_if_needed(const char * path); 


/** Makes directories like:  
 *   prefix/(patttern} 
 *
 *   where pattern is something like yyyy/mm/dd/hh/ or sometihng like that 
 */ 
int mkdirs_for_time(const char * prefix, const char * pattern,  time_t when); 


#define tmp_suffix ".tmp" 
#define tmp_suffix_len  strlen(tmp_suffix) 

/* Closes and, if ends with .tmp suffix, renames */ 
int do_close(gzFile gzf, char * path); 


#endif
