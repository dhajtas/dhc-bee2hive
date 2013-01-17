
#include <stdio.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#include "include/rtc.h"
#include "include/sd_routines.h"
#include "include/routines.h"
#include "include/FAT32.h"
#include "include/rtc.h"


//uint8_t readCfgFile(DATE *adate,TIME *atime, uint8_t *filename_dat, uint8_t atomic)
uint8_t readCfgFile(uint8_t *filename_dat, uint8_t atomic)
{
	DIR *dir;
	uint32_t cluster, fileSize, firstSector;
	uint16_t k;
	uint8_t i,j,ld,lt;
	uint8_t filename[]="CONFIG  CFG";
	uint8_t filename2[]="_CONFIG CFG";
	
	TIME_t time;
	DATE_t date;
	
	
	dir = findFiles (GET_FILE, filename, filename, atomic); 	//get the file location
	if(dir == 0) 
		return(0);

	cluster = (((uint32_t) dir->firstClusterHI) << 16) | dir->firstClusterLO;
	fileSize = dir->fileSize;
	
	ld = 0;
	lt = 0;
	
	while(1)
	{
		firstSector = getFirstSector (cluster);
		for(j=0; j<SectorPerCluster; j++)
		{
			SD_readSingleBlock(firstSector + j, atomic);
			for(k=0; k<512; k++)
			{
				switch(buffer[k])
				{
					case	'S':
								k++;
								if(buffer[k]=='T')
								{
									k++;
									time.h = getDec((uint8_t*)buffer+k);
									//k +=2;
									time.m = getDec((uint8_t*)buffer+k+2);
									//k +=2;
									time.s = getDec((uint8_t*)buffer+k+4);
								}
								else if(buffer[k]=='D')
								{	
									k++;
									date.d = getDec(&buffer[k]);
									//k +=2;
									date.m = getDec(&buffer[k+2]);
									//k +=2;
									date.y = 20 + getDec(&buffer[k+4]);
								}
								else if(buffer[k]=='F')
								{
									k = k+2;
									for(i=0;i<13;i++)
									{
										if(buffer[k+i] < 0x20)		//if any control code
											break;
										filename_dat[i] = buffer[k+i];
									}
									k = k+i;
								}
								break;
					case	'A':
								k++;
								if(buffer[k]=='D')
								{	
									k++;
									ADate[ld].d = getDec((uint8_t*)buffer+k);
									//k +=2;
									ADate[ld].m = getDec((uint8_t*)buffer+k+2);
									//k +=2;
									ADate[ld].y = 20 + getDec((uint8_t*)buffer+k+4);
									ld++;
								}
								else if(buffer[k]=='T')
								{	
									k++;
									ATime[lt].h = getDec((uint8_t*)buffer+k);
									//k +=2;
									ATime[lt].m = getDec((uint8_t*)buffer+k+2);
									//k +=2;
									ATime[lt].s = getDec((uint8_t*)buffer+k+4);
									lt++;
								}
								break;
					
				}
				
				if (k >= fileSize )
				{
					findFiles(RENAME,filename, filename2, atomic);
					RTC_SetDateTime(&date, &time);
					return(lt);
				}
			}
		}
		cluster = getSetNextCluster (cluster, GET, 0, atomic);
		if(cluster == 0) 
		{
#if RSCOM == 1
			printf_P(PSTR("ERR GETTING CLUSTERb\n")); 
#endif		//BIGAVR
			return(0);
		}
	}
	return(0);
}

//-------------------------------------------------------------

uint8_t getDec(uint8_t *data)
{
	uint8_t out = 0;
	
	out = ((uint8_t)*data-0x30)*10+((uint8_t)*(data+1)-0x30);
	return(out);
}