/**
 *  @file sharedMemoryLib.c
 *  @brief 
 *  @author Beaulande Laurent
 *  @date   Decembre 2014
 *  
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>
#include <stdbool.h>

#include "eit.h"
#include "util.h"
#include "logs.h"

static void* ptr_memory;
static size_t length;
static sem_t *mutex;

static char SEM_NAME[] = "/multicat.sem";

int sharedMemory_init(char * mappingFile,int nbStreamingProvider)
{
	  int fd = open (mappingFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	  if ( fd == -1 ) {
		return -1;
	  }
	  length = nbStreamingProvider * sizeof(struct EitInfo);
	
	  lseek (fd, length+1, SEEK_SET);
	  write (fd, "", 1);
      lseek (fd, 0, SEEK_SET);
	  
	  ptr_memory = mmap (0,length, PROT_WRITE, MAP_SHARED, fd, 0);
	  close (fd);	  
	  
	mutex = sem_open(SEM_NAME,O_CREAT,0644,1);
	if(mutex == SEM_FAILED)
    {
		sem_unlink(SEM_NAME);
		switch(errno) {	
			case EACCES : 
				msg_Err( NULL, "EACCES : The semaphore exists, but the caller does not have permission to open it.");
				break;
			case EEXIST :
				msg_Err( NULL, "EEXIST : Both O_CREAT and O_EXCL were specified in oflag, but a semaphore with this name already exists.");
				break;
			case EINVAL :
				msg_Err( NULL, "EINVAL : value was greater than SEM_VALUE_MAX.");
				msg_Err( NULL, "name consists of just \"/\", followed by no other characters.");
				break;
			case EMFILE :
				msg_Err( NULL, "EMFILE : The process already has the maximum number of files and open.");
				break;
			case ENAMETOOLONG :
				msg_Err( NULL, "ENAMETOOLONG : name was too long.");
				break;
			case ENFILE :
				msg_Err( NULL, "ENFILE : The system limit on the total number of open files has been reached.");
				break;
			case ENOENT : 
				msg_Err( NULL, "ENOENT : The O_CREAT flag was not specified in oflag and no semaphore with this name exists; or, O_CREAT was specified, but name wasn't well formed.");
				break;
			case ENOMEM :
				msg_Err( NULL, "ENOMEM : Insufficient memory.");
				break;
		}
		return -2;
    }	  
	
	return 0;
}

int sharedMemory_set(int provider,struct EitInfo *eit)
{
	eit->initialized = 1;
	eit->updated = 1;
    sem_wait(mutex);
	Logs(LOG_DEBUG,__FILE__,__LINE__,"sharedMemory_set use memory");
	struct EitInfo * ptr = (struct EitInfo *)(ptr_memory+((provider-1) * sizeof(struct EitInfo)));
	memcpy(ptr,eit,sizeof(struct EitInfo));
    sem_post(mutex);	
	
	return 0;
}

int sharedMemory_get(int provider,struct EitInfo *eit)
{
	//Logs(LOG_DEBUG,__FILE__,__LINE__,"sharedMemory_get mutex blocked");
    sem_wait(mutex);
	Logs(LOG_DEBUG,__FILE__,__LINE__,"sharedMemory_get use memory");
	struct EitInfo * ptr = (struct EitInfo *)(ptr_memory+((provider-1) * sizeof(struct EitInfo)));
	memcpy(eit,ptr,sizeof(struct EitInfo));
	ptr->updated = 0;
    sem_post(mutex);	
	//Logs(LOG_DEBUG,__FILE__,__LINE__,"sharedMemory_get mutex unblocked");

	eit->updated = 0;
	return 0;
}

int sharedMemory_getWithoutUpdate(int provider,struct EitInfo *eit)
{
    sem_wait(mutex);
	struct EitInfo * ptr = (struct EitInfo *)(ptr_memory+((provider-1) * sizeof(struct EitInfo)));
	memcpy(eit,ptr,sizeof(struct EitInfo));
    sem_post(mutex);	
	return 0;
}

short sharedMemory_get_updated(int provider)
{
	short ret =0;
    sem_wait(mutex);
	struct EitInfo * ptr = (struct EitInfo *)(ptr_memory+((provider-1) * sizeof(struct EitInfo)));
	ret=ptr->updated;
    sem_post(mutex);	

	return ret;
}

void sharedMemory_close()
{
  munmap (ptr_memory,length);
  sem_close(mutex);
  sem_unlink(SEM_NAME);  
}