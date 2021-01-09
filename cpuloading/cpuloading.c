/*
=============================================================================
Filename     : cpuloading.c
Version      : 1.0
Created      : 06/13/2018 01:15:34 PM
Revision     : none
Compiler     : gcc
Author       : Bamboo Do, dovanquyen.vn@gmail.com
Copyright (c) 2018,  All rights reserved.
Description  :
=============================================================================
*/
/*******************************************************************/
/**************************** Header Files *************************/
/*******************************************************************/
/* Start Including Header Files */
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* End Including Headers */

/*******************************************************************/
/****************************** define *****************************/
/*******************************************************************/
/* Start #define */
#ifndef BBLOG_PRINT
#define	BBLOG_PRINT(fmt, ...)	fprintf(stdout, "\033[0m" fmt  "\033[0m" "",  ## __VA_ARGS__)
#endif
#ifndef BBLOG_ERR
#define	BBLOG_ERR(fmt, ...)		fprintf(stdout, "\033[31m" fmt  "\033[0m" "",  ## __VA_ARGS__)
#endif

#define ERR_OK						0
#define ERR_FAIL					1 
#define TRUE						1
#define FALSE 						0 

#define CPULOADING_NUM_FRAME_RATE   (100)
#define CPULOADING_LOGGING_PATH		"/tmp/cpuloading.log"

#define CPULOADING_APP_VERSION		"1.0"
/* End #define */

/*******************************************************************/
/*********************** Type Defination ***************************/
/*******************************************************************/
/* Start typedef */
typedef int	BOOL;
/* End typedef */


/*******************************************************************/
/*********************** Global Variables **************************/
/*******************************************************************/
/* Start global variable */
/* End global variable */


/*******************************************************************/
/*********************** Static Variables **************************/
/*******************************************************************/
/* Start static variable */
static BOOL    s_bIsQuit	= FALSE;
/* End static variable */


/*******************************************************************/
/******************** Global Functions ********************/
/*******************************************************************/
/* Start global functions */
extern const char *g_strAppBuildDate;
extern const char *g_strAppBuildUser;
/* End global functions */


/*******************************************************************/
/*********************** Static Functions **************************/
/*******************************************************************/
/* Start static functions */
/* End static functions */


/*******************************************************************/
/*********************** Function Description***********************/
/*******************************************************************/
#define ___STATIC_FUNCTION_________________
/* 
 * @Desc
 * @Param
 * @Return
 * */
static void signalHandler(int signum)
{
	BBLOG_PRINT("Quitting!!!\n");
    s_bIsQuit = TRUE;
}

/* 
 * @Desc
 * @Param
 * @Return
 * */ 
static int cpuload_get(int *procLoad, int *cpuLoad)
{
    static unsigned long   	prevIdle    = 0;
    static unsigned long   	prevTotal   = 0;
    static unsigned long   	prevProc    = 0;
    unsigned long          	user, nice, sys, idle, total, proc;
    unsigned long          	uTime, sTime, cuTime, csTime;
	unsigned long		   	deltaTotal, deltaIdle, deltaProc;
    BOOL           			cpuLoadFound= FALSE;
    char           			textBuf[4];
    FILE            		*fp;

    /* 
     * Read the overall system information
     * */
    fp = fopen("/proc/stat", "r");
    if(fp == NULL)
    {
        BBLOG_ERR("Cannot open /proc/stat. Is the /proc filesystem mounted?\n");
        return ERR_FAIL;
    }

    /* Scan the file line by line */
    while(fscanf(fp, "%4s %lu %lu %lu %lu %*[^\n]", textBuf, &user, &nice, &sys, &idle) != EOF)
	{
		if(strcmp(textBuf, "cpu") == 0)
		{
			cpuLoadFound = TRUE;
			break;
		}
	}
	
	if(fclose(fp) != 0)
	{
        BBLOG_ERR("Cannot close /proc/stat. Is the /proc filesystem mounted?\n");
        return ERR_FAIL;		
	}

	if(!cpuLoadFound)
	{
		return ERR_FAIL;
	}
	
	/* Read the current process information */
	fp = fopen("/proc/self/stat", "r");
    if(fp == NULL)
    {
        BBLOG_ERR("Cannot open /proc/self/stat. Is the /proc filesystem mounted?\n");
        return ERR_FAIL;
    }

    if (fscanf(fp, "%*d %*s %*s %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %lu "
                     "%lu %lu %lu", &uTime, &sTime, &cuTime, &csTime) != 4) {
        BBLOG_ERR("Failed to get process load information.\n");
        fclose(fp);
        return ERR_FAIL;
    }	
	
	if(fclose(fp) != 0)
	{
        BBLOG_ERR("Cannot close /proc/self/stat. Is the /proc filesystem mounted?\n");
        return ERR_FAIL;		
	}	
	
    total = user + nice + sys + idle;
    proc = uTime + sTime + cuTime + csTime;
	
	/* Check if this is the first time, if so init the prev values */
	if (prevIdle == 0 && prevTotal == 0 && prevProc == 0)
	{
        prevIdle = idle;
        prevTotal = total;
        prevProc = proc;
		return ERR_OK;
	}
	
    deltaIdle = idle - prevIdle;
    deltaTotal = total - prevTotal;
    deltaProc = proc - prevProc;

    prevIdle = idle;
    prevTotal = total;
    prevProc = proc;

    *cpuLoad = 100 - deltaIdle * 100 / deltaTotal;
    *procLoad = deltaProc * 100 / deltaTotal;

	return ERR_OK;	
}

#define ___GLOBAL_FUNCTION_________________
/* 
 * @Desc
 * @Param
 * @Return
 * */
int main(int argc, char **argv)
{
	struct sigaction	sigAction;
	double				total 	= 0.0;
	int					proc 	= 0;
	int					cpu 	= 0;
	int					max 	= 0;
	int					cnt 	= 0;
	int					i 		= 0;
	int					cpuLoadCnt = CPULOADING_NUM_FRAME_RATE;
	FILE				*fp;
	
	BBLOG_PRINT("\n\n\n ================================================\n");
	BBLOG_PRINT("   Start CPU Loading v%s application. Copyright (c)BBTECH Lab!!\n", CPULOADING_APP_VERSION);
	BBLOG_PRINT("   Build date : %s \n", g_strAppBuildDate);
	BBLOG_PRINT("   Build user : %s \n", g_strAppBuildUser);
	BBLOG_PRINT("================================================\n\n\n");
	
    /* insure a clean shutdown if user types ctrl-c */
    sigAction.sa_handler = signalHandler;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;
    sigaction(SIGINT, &sigAction, NULL);
	
	BBLOG_PRINT("Usage: ./cpu_loading [count]\n");
	
	if(1 == argc)
	{
		cpuLoadCnt = CPULOADING_NUM_FRAME_RATE;
	}
	else
	{
		cpuLoadCnt = atoi(argv[1]);
	}
	
	BBLOG_PRINT("CPU LOAD RUNNING FOR COUNT: %d \n", cpuLoadCnt);
	
	if((fp = fopen(CPULOADING_LOGGING_PATH, "wb")) == NULL)
	{
		BBLOG_ERR("Cannot create a %s\n", CPULOADING_LOGGING_PATH);
		s_bIsQuit = TRUE;
	}
	
	while(!s_bIsQuit)
	{
		cpuload_get(&proc, &cpu);
		if((cnt > 0)&&(cnt <= cpuLoadCnt))
		{
			if(cpu > max)
			{
				max = cpu;
			}
			total += cpu;
			i++;
			BBLOG_PRINT("COUNT:%4d\tCPU LOADING:%5d\n", cnt, cpu);
			fprintf(fp, "COUNT:%4d\tCPU LOADING:%5d\n", cnt, cpu);
		}
		else
		{
			if(cnt > cpuLoadCnt)
			{
				s_bIsQuit = TRUE;
			}
		}
		cnt++;
		sleep(1);
	}
	
	total /= i;
	if(fp)
	{
		fprintf(fp, "AVERAGE:%4.3f\tMAX:%d\n", total, max);
		BBLOG_PRINT("AVERAGE:%4.3f\tMAX:%d\n", total, max);
	}
	
	if(fp)
	{
		fclose(fp);
	}
	
	return ERR_OK;
}

/*********************** End of File ******************************/


