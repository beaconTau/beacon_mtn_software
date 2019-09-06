#include "beacon-common.h" 
#include <stdio.h> 
#include <time.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/stat.h> 
#include <sys/mman.h> 
#include "beacon-cfg.h" 
#include "beacon.h" 
#include <fcntl.h>
#include <pthread.h> 



float timespec_difference_float(const struct timespec * a, const struct timespec * b) 
{

  float sec_diff = a->tv_sec - b->tv_sec; 
  float nanosec_diff = a->tv_nsec - b->tv_nsec; 
  return sec_diff + 1e-9 * (nanosec_diff); 
} 


int beacon_get_cfg_file(char ** name, beacon_program_t p) 
{
  switch (p) 
  {
    case BEACON_STARTUP: 
      asprintf(name, "%s/%s", getenv(CONFIG_DIR_ENV) ?: "cfg" , CONFIG_DIR_STARTUP_NAME); 
      break;
    case BEACON_HK: 
      asprintf(name, "%s/%s", getenv(CONFIG_DIR_ENV) ?: "cfg" , CONFIG_DIR_HK_NAME); 
      break;
    case BEACON_ACQ: 
      asprintf(name, "%s/%s", getenv(CONFIG_DIR_ENV) ?: "cfg" , CONFIG_DIR_ACQ_NAME); 
      break;
    case BEACON_COPY: 
      asprintf(name, "%s/%s", getenv(CONFIG_DIR_ENV) ?: "cfg" , CONFIG_DIR_COPY_NAME); 
      break;
    default: 
      return -1; 
  }
  return 0; 
} 

int mkdir_if_needed(const char * path)
{

  struct stat st; 
  if ( stat(path,&st) || ( (st.st_mode & S_IFMT) != S_IFDIR))//not there, try to make it
  {
     return mkdir(path,0755);
  }

  return 0; 

}

int do_close(gzFile gzf, char * path) 
{
  int ret = gzclose(gzf); 

  int pathlen = strlen(path); 

  //check if we end with a .tmp suffix 
  //and rename if we do 
  if (!strcasecmp(path + pathlen-tmp_suffix_len ,tmp_suffix)) 
  {
    char * final_path = strdup(path);
    final_path[pathlen-tmp_suffix_len] = 0; 
//    printf("Renaming %s to %s\n", path, final_path); 
    rename(path,final_path); 
    free(final_path); 
  }

  free(path); 
  return ret; 
}



static pthread_mutex_t * hk_lock =0; 
static int have_hk_lock =0;  


int lock_shared_hk() 
{
  if (!hk_lock) return -1; 
  if (have_hk_lock) return -1; 


  int ret = pthread_mutex_lock(hk_lock); 
  if(!ret)
  {
    have_hk_lock =1; 
  }
  return ret; 
}

int unlock_shared_hk() 
{
  if (!hk_lock || !have_hk_lock) return -1; 

  int ret = pthread_mutex_unlock(hk_lock); 
  if(!ret)
  {
    have_hk_lock =0; 
  }
  return ret; 
}

int open_shared_hk(const beacon_hk_cfg_t * cfg, int readonly, beacon_hk_t ** hk) 
{
    int ret = 0; 
    int shared_fd = shm_open(cfg->shm_name, O_CREAT | O_RDWR, 0666); 

    if (shared_fd < 0) return -1; 

    ret = ftruncate(shared_fd, sizeof(beacon_hk_t)); 

    if (! ret) 
    {
      close(shared_fd); 
      return -1; 
    }

    //create the lock
    if (!hk_lock) 
    {
        int lock_fd = shm_open(cfg->shm_lock_name, O_CREAT | O_RDWR, 0666); 
        if (lock_fd < 0) 
        {
          close(shared_fd); 
          return -1; 
        }

        if (!ftruncate(lock_fd, sizeof(pthread_mutex_t)))
        {
          close(shared_fd); 
          close(lock_fd); 
          return -1; 
        }

        hk_lock = mmap(0, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, lock_fd, 0); 
        close(lock_fd); 
        if (!hk_lock) 
        {
          close (shared_fd); 
          return -1; 
        }
        
        pthread_mutexattr_t attr; 
        pthread_mutexattr_init(&attr); 
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED); 
        pthread_mutex_init(hk_lock,&attr); 
    }

    *hk = mmap (0, sizeof(beacon_hk_t), readonly ? PROT_READ : PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0); 
    close(shared_fd); 
    if (!*hk) return -1; 
    return 0; 
}


__attribute__((destructor))
static void unload() 
{
//make sure we don't hold the mutex
  if(have_hk_lock) 
  {
    unlock_shared_hk(); 
  }
}





