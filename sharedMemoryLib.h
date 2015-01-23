/**
 *  @file sharedMemoryLib.h
 *  @brief 
 *  @author Beaulande Laurent
 *  @date   Decembre 2014
 *
 */

#ifndef __SHAREDMEMORYLIB_H__
#define __SHAREDMEMORYLIB_H__

int sharedMemory_init(char * mappingFile,int nbStreamingProvider);
int sharedMemory_set(int provider,struct EitInfo *eit);
int sharedMemory_get(int provider,struct EitInfo *eit);
int sharedMemory_getWithoutUpdate(int provider,struct EitInfo *eit);
short sharedMemory_get_updated(int provider);

void sharedMemory_close();

#endif
