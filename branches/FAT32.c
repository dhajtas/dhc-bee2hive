//*******************************************************
// **** ROUTINES FOR FAT32 IMPLEMATATION OF SD CARD ****
//**********************************************************
//Controller: ATmega32 (Clock: 8 Mhz-internal)
//Compiler	: AVR-GCC (winAVR with AVRStudio)
//Version 	: 2.4
//Author	: CC Dharmani, Chennai (India)
//			  www.dharmanitech.com
//Date		: 17 May 2010
//********************************************************

//Link to the Post: http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#if BIGAVR == 1
	#include "include/glcd.h"
	#include "include/k0108.h"
#endif
#include "include/SD_routines.h"
#include "include/FAT32.h"
#include "include/spi.h"
#include "include/rtc.h"

static uint16_t BlockCounter __attribute__ ((section (".noinit")));
uint16_t AppendStartSector __attribute__ ((section (".noinit")));
uint32_t AppendFileSector __attribute__ ((section (".noinit")));
uint32_t AppendFileLocation __attribute__ ((section (".noinit")));
uint32_t FileSize __attribute__ ((section (".noinit")));
uint32_t AppendStartCluster __attribute__ ((section (".noinit")));

uint8_t FreeClusterCountUpdated __attribute__ ((section (".noinit")));

uint8_t FAT32_InitFS(void)
{
	uint8_t error=0;
	uint8_t i;

	cardType = 0;

	for (i=0; i<10; i++)
	{
		error = SD_Init(2);
		if(!error) break;
	}

	if(error)
	{
#if RSCOM == 1
		if(error == 1) 
			printf_P(PSTR("SD NOT DETECTED.."));
		if(error == 2)
			printf_P(PSTR("CRD INIT FAILED.."));
#endif
		PORTD.OUTSET = _BV(LED2); 		//switching ON the LED (for testing purpose only)
		return(1);  //no SD detected/failed 
	}

#if RSCOM == 1
	switch (cardType)
	{
		case 1:	printf_P(PSTR("STD CAPACITY CRD (V 1.X) DET!"));
				break;
		case 2:printf_P(PSTR("HIGH CAPACITY CRD DET!"));
				break;
		case 3:printf_P(PSTR("STD CAPACITY CRD (V 2.X) DET!"));
				break;
		default:printf_P(PSTR("UNKNOWN SD CRD DETECTED!"));
				break; 
	}
#endif


	SPI_HIGH_SPEED;	//SCK - 4 MHz

	error = getBootSectorData(0); //read boot sector and keep necessary data in global variables

	if(error) 	
	{
#if RSCOM == 1
		printf_P(PSTR("FAT32 NOT FOUND!"));  //FAT32 incompatible drive
#endif
		return(2);	//no FAT32 detected
	}
	return(0);	//FAT32 active...
}


//***************************************************************************
//Function: to read data from boot sector of SD card, to determine important
//parameters like bytesPerSector, sectorsPerCluster etc.
//Arguments: none
//return: none
//***************************************************************************
uint8_t getBootSectorData (uint8_t atomic)
{
	BS	 *bpb;
	MBRinfo *mbr;
	partitionInfo *partition;
	uint32_t dataSectors;
	uint32_t totalSectors;

	UnusedSectors = 0;

	SD_readSingleBlock(0, atomic);		
//	dumpBufferHex();
	bpb = (BS*)buffer;

	if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB)   	//check if it is boot sector
	{
		mbr = (MBRinfo*) buffer;       							//if it is not boot sector, it must be MBR
		if(mbr->signature != 0xaa55) 
			return(1);       									//if it is not even MBR then it's not FAT32
		partition = (partitionInfo*)(mbr->partitionData);		//first partition
		UnusedSectors = partition->firstSector; 				//the unused sectors, hidden to the FAT
  		SD_readSingleBlock(partition->firstSector, atomic);	//read the bpb sector 
		bpb = (BS*)buffer;
		if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB) 
			return(1); 
	}

	BytesPerSector = bpb->bytesPerSector;
		//transmitHex(INT, bytesPerSector); transmitByte(' ');
	FSIOffset = bpb->FSinfo;
	SectorPerCluster = bpb->sectorPerCluster;
		//transmitHex(INT, sectorPerCluster); transmitByte(' ');
	ReservedSectorCount = bpb->reservedSectorCount;
	totalSectors = bpb->totalSectors_F32;
	
	RootCluster = bpb->rootCluster;// + (sector / sectorPerCluster) +1;
	FirstDataSector = bpb->hiddenSectors + ReservedSectorCount + (bpb->numberofFATs * bpb->FATsize_F32);
	dataSectors = totalSectors - bpb->reservedSectorCount - ( bpb->numberofFATs * bpb->FATsize_F32);
	
	TotalClusters = dataSectors / SectorPerCluster;
		//transmitHex(LONG, totalClusters); transmitByte(' ');

	if((getSetFreeCluster (TOTAL_FREE, GET, 0, atomic)) > TotalClusters)  //check if FSinfo free clusters count is valid
		FreeClusterCountUpdated = 0;
	else
		FreeClusterCountUpdated = 1;
	return(0);
}

//***************************************************************************
//Function: to calculate first sector address of any given cluster
//Arguments: cluster number for which first sector is to be found
//return: first sector address
//***************************************************************************
uint32_t getFirstSector(uint32_t clusterNumber)
{
	return (((clusterNumber - 2) * SectorPerCluster) + FirstDataSector);
}

//***************************************************************************
//Function: get cluster entry value from FAT to find out the next cluster in the chain
//or set new cluster entry in FAT
//Arguments: 1. current cluster number, 2. get_set (=GET, if next cluster is to be found or = SET,
//if next cluster is to be set 3. next cluster number, if argument#2 = SET, else 0
//return: next cluster number, if if argument#2 = GET, else 0
//****************************************************************************
uint32_t getSetNextCluster (	uint32_t clusterNumber,
                                uint8_t get_set,
                                uint32_t clusterEntry, 
								uint8_t atomic)
{
	uint16_t FATEntryOffset;
	uint32_t *FATEntryValue;
	uint32_t FATEntrySector;
	uint8_t retry = 0;

	FATEntrySector = UnusedSectors + ReservedSectorCount + ((clusterNumber * 4) / BytesPerSector) ;	//get sector number of the cluster entry in the FAT

	FATEntryOffset = (uint16_t) ((clusterNumber * 4) % BytesPerSector);	//get the offset address in that sector number

	while(retry <10)										//read the sector into a buffer
	{ 
		if(!SD_readSingleBlock(FATEntrySector, atomic)) 
			break; 
		retry++;
	}

	FATEntryValue = (uint32_t*) &buffer[FATEntryOffset];		//get the cluster address from the buffer

	if(get_set == GET)
		return ((*FATEntryValue) & 0x0fffffff);

	*FATEntryValue = clusterEntry;   							//for setting new value in cluster entry in FAT
	SD_writeSingleBlock(FATEntrySector, atomic);
	return (0);
}

//********************************************************************************************
//Function: to get or set next free cluster or total free clusters in FSinfo sector of SD card
//Arguments: 1.flag:TOTAL_FREE or NEXT_FREE, 
//			 2.flag: GET or SET 
//			 3.new FS entry, when argument2 is SET; or 0, when argument2 is GET
//return: next free cluster, if arg1 is NEXT_FREE & arg2 is GET
//        total number of free clusters, if arg1 is TOTAL_FREE & arg2 is GET
//		  0xffffffff, if any error or if arg2 is SET
//********************************************************************************************
uint32_t getSetFreeCluster(	uint8_t totOrNext, 
							uint8_t get_set, 
							uint32_t FSEntry, 
							uint8_t atomic)
{
	FSinfo *FSI; 			// removed & from front of buffer ???
	uint8_t error;

	FSI = (FSinfo*) buffer;

	SD_readSingleBlock(UnusedSectors + FSIOffset, atomic);		

	if((FSI->leadSignature != 0x41615252) || (FSI->structureSignature != 0x61417272) || (FSI->trailSignature !=0xaa550000))
		return 0xffffffff;

	if(get_set == GET)
	{
		if(totOrNext == TOTAL_FREE)
			return(FSI->freeClusterCount);
		else 												// when totOrNext = NEXT_FREE
			return(FSI->nextFreeCluster);
	}
	else
	{
		if(totOrNext == TOTAL_FREE)
			FSI->freeClusterCount = FSEntry;
		else 												// when totOrNext = NEXT_FREE
			FSI->nextFreeCluster = FSEntry;
 		error = SD_writeSingleBlock(UnusedSectors + FSIOffset, atomic);		//update FSinfo 
	}
	return 0xffffffff;
}

//***************************************************************************
//Function: to get DIR/FILE list or a single file address (cluster number) or to delete a specified file
//Arguments: #1 - flag: GET_LIST, GET_FILE or DELETE #2 - pointer to file name (0 if arg#1 is GET_LIST)
//return: first cluster of the file, if flag = GET_FILE
//        print file/dir list of the root directory, if flag = GET_LIST
//		  Delete the file mentioned in arg#2, if flag = DELETE
//****************************************************************************


DIR* findFiles (uint8_t flag, uint8_t *fileName, uint8_t *newfileName, uint8_t atomic)
{
	uint32_t cluster, sector, firstSector, firstCluster, nextCluster;
	DIR *dir;
	uint16_t i;
	uint8_t j;

	cluster = RootCluster; 															//root cluster

	while(1)
	{
		firstSector = getFirstSector (cluster);

		for(sector = 0; sector < SectorPerCluster; sector++)
		{
			SD_readSingleBlock (firstSector + sector, atomic);
	

			for(i=0; i<BytesPerSector; i+=32)
			{
				dir = (DIR *) &buffer[i];

				if(dir->name[0] == EMPTY) 											//indicates end of the file list of the directory
				{
					if((flag == GET_FILE) || (flag == DELETE) || (flag == RENAME))
#if BIGAVR == 1			
						printf_P(PSTR("FILE DOES NOT EXIST!\n"));
#endif		//BIGAVR
//					dumpBufferHex();
					return(0);   
				}
				if((dir->name[0] != DELETED) && (dir->attrib != ATTR_LONG_NAME))
				{
					if((flag == GET_FILE) || (flag == DELETE) || (flag == RENAME))
					{
						for(j=0; j<11; j++)
							if(dir->name[j] != fileName[j]) 
								break;
						if(j == 11)
						{
							if(flag == GET_FILE)
							{
								AppendFileSector = firstSector + sector;		//Sector with file dir structure in root dir
								AppendFileLocation = i;							//Position within sector
								AppendStartCluster = (((uint32_t) dir->firstClusterHI) << 16) | dir->firstClusterLO;	//File data start cluster
								FileSize = dir->fileSize;						//total file size in bytes
								return (dir);
							}	
							if(flag == RENAME)
							{
								for(j=0; j<11; j++)
									dir->name[j] = newfileName[j];
								SD_writeSingleBlock (firstSector+sector, atomic);
							}
							if(flag == DELETE)    												//when flag = DELETE
							{
#if BIGAVR == 1			
								printf_P(PSTR("DELETING.."));
#endif		//BIGAVR
								firstCluster = (((uint32_t) dir->firstClusterHI) << 16) | dir->firstClusterLO;
													dir->name[0] = DELETED;    						//mark file as 'deleted' in FAT table
								SD_writeSingleBlock (firstSector+sector, atomic);
								freeMemoryUpdate (ADD, dir->fileSize, atomic);
								cluster = getSetFreeCluster (NEXT_FREE, GET, 0, atomic); 	//update next free cluster entry in FSinfo sector
								if(firstCluster < cluster)
									getSetFreeCluster (NEXT_FREE, SET, firstCluster, atomic);
								while(1)  											//mark all the clusters allocated to the file as 'free'
								{
									nextCluster = getSetNextCluster (firstCluster, GET, 0, atomic);
									getSetNextCluster (firstCluster, SET, 0, atomic);
									if(nextCluster > 0x0ffffff6) 
									{
#if BIGAVR == 1			
										printf_P(PSTR("FILE DELETED!\n"));
#endif		//BIGAVR
										return(0);
									}
									firstCluster = nextCluster;
								} 
							}
						}
					}
					else  															//when flag = GET_LIST
					{	
//						printf_P(NEWLINE);
//						for(j=0; j<11; j++)
//						{
//							if(j == 8) transmitByte(' ');
//							transmitByte (dir->name[j]);
//						}
//						printf_P (PSTR("   "));
#if BIGAVR == 1			
						printf_P(PSTR("\n %s"), dir->name);
						
						if((dir->attrib != 0x10) && (dir->attrib != 0x08))
						{
							printf_P(PSTR("FILE"));
							printf_P(PSTR("   "));
//							displayMemory (LOW, dir->fileSize);
						}
						else
							printf_P((dir->attrib == 0x10)? PSTR("DIR") : PSTR("ROOT"));
#endif		//BIGAVR
					}
				}
			}
		}
		cluster = (getSetNextCluster (cluster, GET, 0, atomic));
		if(cluster > 0x0ffffff6)
			return(0);
		if(cluster == 0) 
		{
#if BIGAVR == 1			
			printf_P(PSTR("ERR GETTING CLUSTERa\n"));  
#endif		//BIGAVR
			return(0);
		}
	}
	return(0);
}

//***************************************************************************
//Function: if flag=READ then to read file from SD card and send contents to UART 
//if flag=VERIFY then functions will verify whether a specified file is already existing
//Arguments: flag (READ or VERIFY) and pointer to the file name
//return: 0, if normal operation or flag is READ
//	      1, if file is already existing and flag = VERIFY
//		  2, if file name is incompatible
//***************************************************************************
uint8_t readFile (uint8_t flag, uint8_t *fileName, uint8_t atomic)
{
	DIR *dir;
	uint32_t cluster, byteCounter = 0, fileSize, firstSector;
	uint16_t k;
	uint8_t j, error;

	error = convertFileName(fileName); 	//convert fileName into FAT format
	if(error) 
		return(2);

	dir = findFiles(GET_FILE, fileName, fileName, atomic); 	//get the file location
	if(dir == 0) 
		return(0);

	if(flag == VERIFY) 
		return(1);							//specified file name is already existing

	cluster = (((uint32_t) dir->firstClusterHI) << 16) | dir->firstClusterLO;
	fileSize = dir->fileSize;

	while(1)
	{
		firstSector = getFirstSector (cluster);
		for(j=0; j<SectorPerCluster; j++)
		{
			SD_readSingleBlock(firstSector + j, atomic);
			for(k=0; k<512; k++)
			{
#if BIGAVR == 1			
				printf("%c",buffer[k]);
#endif		//BIGAVR
				if ((byteCounter++) >= fileSize ) 
					return(0);
			}
		}
		cluster = getSetNextCluster (cluster, GET, 0, atomic);
		if(cluster == 0) 
		{
#if BIGAVR == 1			
			printf_P(PSTR("ERROR GETTING CLUSTER\n")); 
#endif		//BIGAVR
			return(0);
		}
	}
	return(0);
}

//***************************************************************************
//Function: to convert normal short file name into FAT format
//Arguments: pointer to the file name
//return: 0, if successful else 1.
//***************************************************************************
uint8_t convertFileName (uint8_t *fileName)
{
	uint8_t fileNameFAT[11];
	uint8_t j, k;

	for(j=0; j<12; j++)
		if(fileName[j] == '.') 
			break;

	if(j>8) 
	{
#if BIGAVR == 1			
		printf_P(PSTR("INVALID FILENAME...\n")); 
#endif		//BIGAVR
		return 1;
	}

	for(k=0; k<j; k++) 				//setting file name
		fileNameFAT[k] = fileName[k];

	for(k=j; k<=7; k++) 				//filling file name trail with blanks
		fileNameFAT[k] = ' ';

	j++;
	for(k=8; k<11; k++) 				//setting file extention
	{
		if(fileName[j] != 0)
			fileNameFAT[k] = fileName[j++];
		else 							//filling extension trail with blanks
			while(k<11)
			fileNameFAT[k++] = ' ';
	}

	for(j=0; j<11; j++) 				//converting small letters to caps
		if((fileNameFAT[j] >= 0x61) && (fileNameFAT[j] <= 0x7a))
			fileNameFAT[j] -= 0x20;

	for(j=0; j<11; j++)
		fileName[j] = fileNameFAT[j];

	return 0;
}

//************************************************************************************
//Function: to rename a file in FAT32 format in the root directory 
//Arguments: pointer to the old file name, new file name
//return: error/done
//************************************************************************************

uint8_t renameFile(uint8_t *oldName, uint8_t *newName, uint8_t atomic)
{
	uint8_t error = 0;

	error = convertFileName(oldName);
	if(error)
		return(2);
	error = convertFileName(newName);
	if(error)
		return(2);

	findFiles(RENAME, oldName, newName, atomic);
	return(0);
}



//************************************************************************************
//Function: to create a file in FAT32 format in the root directory if given 
//			file name does not exist; if the file already exists then append the data
//Arguments: pointer to the file name
//return: stream number??? - 0 error
//************************************************************************************

uint8_t openFile(uint8_t *fileName, uint8_t flag, uint8_t atomic)
{
	DIR *dir;
	uint32_t cluster, prevCluster, newCluster, firstSector, clusterCount;
	uint16_t i, sector, dateFAT, timeFAT;
	uint8_t j, error, fileCreatedFlag = 0;

//tst
	BlockCounter = 0;
	
	error = convertFileName(fileName); 	//convert fileName into FAT format
	if(error) 
		return(2);	//invalid file name

	dir = findFiles(GET_FILE, fileName, fileName, atomic); 	//get the file location
	if(dir == 0) 
	{
#if BIGAVR == 1			
		printf_P(PSTR("\n CREATING FILE...\n"));
#endif		//BIGAVR

		cluster = getSetFreeCluster (NEXT_FREE, GET, 0, atomic);
		if(cluster > TotalClusters)
			cluster = RootCluster;
		cluster = searchNextFreeCluster(cluster, atomic);
		if(cluster == 0)
		{
#if BIGAVR == 1			
			printf_P(PSTR("\n NO FREE CLUSTER!\n"));
#endif		//BIGAVR
			return(1);
		}
		getSetNextCluster(cluster, SET, EOF, atomic);   //last cluster of the file, marked EOF
  
		FileSize = 0;
		dateFAT = getDateFAT();
		timeFAT = getTimeFAT();
		
		prevCluster = RootCluster; //root cluster
		while(!fileCreatedFlag)
		{
			firstSector = getFirstSector (prevCluster);
			for(sector = 0; sector < SectorPerCluster; sector++)
			{
				SD_readSingleBlock (firstSector + sector, atomic);
				for(i=0; i<BytesPerSector; i+=32)
				{
					dir = (DIR *) &buffer[i];
					if((dir->name[0] == EMPTY) || (dir->name[0] == DELETED))  //looking for an empty slot to enter file info
					{
						for(j=0; j<11; j++)
							dir->name[j] = fileName[j];
						dir->attrib = ATTR_ARCHIVE;				//settting file attribute as 'archive'
						dir->NTreserved = 0;						//always set to 0
						dir->timeTenth = 0;						//always set to 0
						dir->createTime = timeFAT; 				//setting time of file creation, obtained from RTC
						dir->createDate = dateFAT; 				//setting date of file creation, obtained from RTC
						dir->lastAccessDate = 0;   				//date of last access ignored
						dir->writeTime = timeFAT;  				//setting new time of last write, obtained from RTC
						dir->writeDate = dateFAT;  				//setting new date of last write, obtained from RTC
						dir->firstClusterHI = (uint16_t) ((cluster & 0xffff0000) >> 16 );
						dir->firstClusterLO = (uint16_t) (cluster & 0x0000ffff);
						dir->fileSize = 0;

						SD_writeSingleBlock(firstSector + sector, atomic);

#if BIGAVR == 1			
						printf_P(PSTR("\n File Created!"));
#endif		//BIGAVR

						freeMemoryUpdate (REMOVE, FileSize, atomic);		//updating free memory count in FSinfo sector
						fileCreatedFlag = 1;
						
						AppendFileSector = firstSector + sector;
						AppendFileLocation = i;
						
						break;
					}
				}
				if(fileCreatedFlag)
					break;
			}
			
			if(fileCreatedFlag)
				break;
			
			newCluster = getSetNextCluster (prevCluster, GET, 0, atomic);
			if(newCluster > 0x0ffffff6)
			{
				if(newCluster >= EOF)   								//this situation will come when total files in root is multiple of (32*sectorPerCluster)
				{  
					newCluster = searchNextFreeCluster(prevCluster, atomic); 	//find next cluster for root directory entries
					getSetNextCluster(prevCluster, SET, newCluster, atomic); 	//link the new cluster of root to the previous cluster
					getSetNextCluster(newCluster, SET, EOF, atomic);  		//set the new cluster as end of the root directory
				} 
				else
				{	
#if BIGAVR == 1			
					printf_P(PSTR("END OF CLUSTER CHAIN\n")); 
#endif		//BIGAVR
					return(1);
				}
			}
			if(newCluster == 0) 
			{
#if BIGAVR == 1			
				printf_P(PSTR("ERROR GETTING CLUSTER\n")); 
#endif		//BIGAVR
				return(1);
			}
			prevCluster = newCluster;
		}
		clusterCount = 0;
	}
	else
	{
//		FileSize = dir->fileSize;  // uz fo findFiles
#if BIGAVR == 1		
		printf_P(PSTR(" FILE EXISTS, APPENDING DATA...\n")); 
#endif			//BIGAVR
		cluster = AppendStartCluster;
		clusterCount=0;
		while(1)
		{
			newCluster = getSetNextCluster (cluster, GET, 0, atomic);	// next cluster of file in FAT
			if(newCluster >= EOF) 
				break;
			cluster = newCluster;
			clusterCount++;
		}

		
//last sector number of the last cluster of the file	
//		sector = (FileSize - (clusterCount * SectorPerCluster * BytesPerSector)) / BytesPerSector; 
//		start = 1;		
	}
	
	AppendStartCluster = cluster;
	AppendStartSector = (FileSize - (clusterCount * SectorPerCluster * BytesPerSector)) / BytesPerSector; 

	BlockCounter = 0;	//zmenit do buducna na presny vypocet...
	return(0);
}

//************************************************************************************
//Function: to create a file in FAT32 format in the root directory if given 
//			file name does not exist; if the file already exists then append the data
//Arguments: pointer to the file name
//return: stream number??? - 0 error
//************************************************************************************

void writeFile (uint8_t atomic)
{
	uint8_t error;
	uint32_t cluster, prevCluster;
	
		printf_P(PSTR("\n WRITING..."));
	
		startBlock = getFirstSector(AppendStartCluster) + AppendStartSector;

// add possibility of appending to the half full sector - i
// for now write only sector by sector (full sectors)

//test		
//		FileSize += RAM_Read_block(&buffer[0], BytesPerSector, atomic);

		error = SD_writeSingleBlock(startBlock, atomic);
//		allocNextSector(atomic);
		AppendStartSector++;
		if (AppendStartSector == SectorPerCluster)
		{
			AppendStartSector = 0;
			prevCluster = AppendStartCluster;

			cluster = searchNextFreeCluster(prevCluster, atomic); 	//look for a free cluster starting from the current cluster
			
			if(cluster == 0)
			{
				printf_P(PSTR("/n NO FREE CLUSTER!"));
				return;
			}

			getSetNextCluster(prevCluster, SET, cluster, atomic);
			getSetNextCluster(cluster, SET, EOF, atomic);   		//last cluster of the file, marked EOF

			AppendStartCluster = cluster;
		}
}

//************************************************************************************
//Function: to create a file in FAT32 format in the root directory if given 
//			file name does not exist; if the file already exists then append the data
//Arguments: pointer to the file name
//return: stream number??? - 0 error
//************************************************************************************

void writeSpectrum (uint8_t end_file, uint16_t size, uint16_t *spectrum, uint8_t atomic)
{
	uint8_t error, i;
	uint32_t cluster, prevCluster;
	uint16_t *spect_buffer;

	spect_buffer = (uint16_t*)buffer;

		startBlock = getFirstSector(AppendStartCluster) + AppendStartSector;

// add possibility of appending to the half full sector - i
// for now write only sector by sector (full sectors)

	for(i=0; i<size; i++) 	
	{
		if(!end_file)
		{
			spect_buffer[BlockCounter>>1] = spectrum[i];// move text to buffer
			FileSize += 2; 
			BlockCounter += 2;
		}
		if((BlockCounter == 512)||(end_file))		//max size of the block or ending the file
		{
			BlockCounter = 0;
			error = SD_writeSingleBlock(startBlock, atomic);	// write buffer to file
			if(!end_file)									// if ending file no need to search for next Sector/Cluster
			{
//				allocNextSector(atomic);
				AppendStartSector++;
				if (AppendStartSector == SectorPerCluster)
				{
					AppendStartSector = 0;
					prevCluster = AppendStartCluster;

					cluster = searchNextFreeCluster(prevCluster, atomic); 	//look for a free cluster starting from the current cluster
					
					if(cluster == 0)
					{
						printf_P(PSTR("/n NO FREE CLUSTER!"));
						return;
					}

					getSetNextCluster(prevCluster, SET, cluster, atomic);
					getSetNextCluster(cluster, SET, EOF, atomic);   		//last cluster of the file, marked EOF

					AppendStartCluster = cluster;
				}
			}
		}
	}
}

void writeBlock (uint8_t end_file, uint16_t size, uint8_t *spectrum, uint8_t atomic)
{
	uint8_t error;
	uint16_t i;
	uint32_t cluster, prevCluster;
	uint8_t *spect_buffer;

	spect_buffer = (uint8_t*)buffer;

	startBlock = getFirstSector(AppendStartCluster) + AppendStartSector;

	// add possibility of appending to the half full sector - i
	// for now write only sector by sector (full sectors)

	for(i=0; i<size; i++)
	{
		if(!end_file)
		{
			spect_buffer[BlockCounter] = spectrum[i];// move text to buffer
			FileSize++;
			BlockCounter++;
		}
		if((BlockCounter == 512)||(end_file))		//max size of the block or ending the file
		{
			BlockCounter = 0;
			error = SD_writeSingleBlock(startBlock, atomic);	// write buffer to file
			if(!end_file)									// if ending file no need to search for next Sector/Cluster
			{
//				allocNextSector(atomic);
				AppendStartSector++;
				if (AppendStartSector == SectorPerCluster)
				{
					AppendStartSector = 0;
					prevCluster = AppendStartCluster;
					cluster = searchNextFreeCluster(prevCluster, atomic); 	//look for a free cluster starting from the current cluster
						
					if(cluster == 0)
					{
						printf_P(PSTR("/n NO FREE CLUSTER!"));
						return;
					}

					getSetNextCluster(prevCluster, SET, cluster, atomic);
					getSetNextCluster(cluster, SET, EOF, atomic);   		//last cluster of the file, marked EOF

					AppendStartCluster = cluster;
				}
			}
		}
	}
}




void writeZeroes (uint8_t end_file, uint16_t size, uint8_t atomic)
{
	uint8_t error;
	uint16_t i;
	uint32_t cluster, prevCluster;
	uint8_t *spect_buffer;

	spect_buffer = (uint8_t*)buffer;

	startBlock = getFirstSector(AppendStartCluster) + AppendStartSector;

	// add possibility of appending to the half full sector - i
	// for now write only sector by sector (full sectors)

	for(i=0; i<size; i++)
	{
		if(!end_file)
		{
			spect_buffer[BlockCounter] = 0x00;// move text to buffer
			FileSize++;
			BlockCounter++;
		}
		if((BlockCounter == 512)||(end_file))		//max size of the block or ending the file
		{
			BlockCounter = 0;
			error = SD_writeSingleBlock(startBlock, atomic);	// write buffer to file
			if(!end_file)									// if ending file no need to search for next Sector/Cluster
			{
//				allocNextSector(atomic);
				AppendStartSector++;
				if (AppendStartSector == SectorPerCluster)
				{
					AppendStartSector = 0;
					prevCluster = AppendStartCluster;

					cluster = searchNextFreeCluster(prevCluster, atomic); 	//look for a free cluster starting from the current cluster
					
					if(cluster == 0)
					{
						printf_P(PSTR("/n NO FREE CLUSTER!"));
						return;
					}

					getSetNextCluster(prevCluster, SET, cluster, atomic);
					getSetNextCluster(cluster, SET, EOF, atomic);   		//last cluster of the file, marked EOF

					AppendStartCluster = cluster;
				}
			}
		}
	}
}

void allocNextSector(uint8_t atomic)
{
	uint32_t cluster, prevCluster;

	AppendStartSector++;
	if (AppendStartSector == SectorPerCluster)
	{
		AppendStartSector = 0;
		prevCluster = AppendStartCluster;

		cluster = searchNextFreeCluster(prevCluster, atomic); 	//look for a free cluster starting from the current cluster
		
		if(cluster == 0)
		{
			printf_P(PSTR("/n NO FREE CLUSTER!"));
			return;
		}

		getSetNextCluster(prevCluster, SET, cluster, atomic);
		getSetNextCluster(cluster, SET, EOF, atomic);   		//last cluster of the file, marked EOF

		AppendStartCluster = cluster;
	}
}

//************************************************************************************
//Function: to close a file in FAT32 format in the root directory if given 
//			file name does not exist; if the file already exists then append the data
//Arguments: pointer to the file name
//return: stream number??? - 0 error
//************************************************************************************


uint8_t closeFile(uint8_t atomic)
{
	uint16_t timeFAT, dateFAT;
	uint32_t extraMemory, nextCluster;
	DIR *dir;
	

	dateFAT = getDateFAT();    //get current date & time from the RTC
	timeFAT = getTimeFAT();
	
	SD_readSingleBlock(AppendFileSector, atomic);    
	dir = (DIR *) &buffer[AppendFileLocation]; 

	dir->lastAccessDate = 0;   							//date of last access ignored
	dir->writeTime = timeFAT;  							//setting new time of last write, obtained from RTC
	dir->writeDate = dateFAT;  							//setting new date of last write, obtained from RTC
	extraMemory = FileSize - dir->fileSize;
	dir->fileSize = FileSize;
	SD_writeSingleBlock (AppendFileSector, atomic);
	freeMemoryUpdate (REMOVE, extraMemory, atomic); 			//updating free memory count in FSinfo sector;

	nextCluster = searchNextFreeCluster(AppendStartCluster, atomic);
	getSetFreeCluster(NEXT_FREE, SET, nextCluster, atomic); 			//update FSinfo next free cluster entry

#if	BIGAVR == 1  
	printf_P(PSTR("\n FILE CLOSED!\n"));
#endif		//BIGAVR	
	return(0);
}


//***************************************************************************
//Function: to search for the next free cluster in the root directory
//          starting from a specified cluster
//Arguments: Starting cluster
//return: the next free cluster
//****************************************************************
uint32_t searchNextFreeCluster (uint32_t startCluster, uint8_t atomic)
{
	uint32_t cluster, *value, sector;
	uint8_t i;
    
//128 claster entries within one 512 bytes large sector of FAT !!! If different byte count change!!!	
	startCluster -=  (startCluster % 128);   				//to start with the first file in a FAT sector
    for(cluster = startCluster; cluster < TotalClusters; cluster+=128) 
    {
		sector = UnusedSectors + ReservedSectorCount + ((cluster * 4) / BytesPerSector);

		SD_readSingleBlock(sector, atomic);

		for(i=0; i<128; i++)
		{
			value = (uint32_t *) &buffer[i*4];
			if(((*value) & 0x0fffffff) == 0)
				return(cluster+i);
		}  
    } 
	return(0);
}

//***************************************************************************
//Function: to display total memory and free memory of SD card, using UART
//Arguments: none
//return: none
//Note: this routine can take upto 15sec for 1GB card (@1MHz clock)
//it tries to read from SD whether a free cluster count is stored, if it is stored
//then it will return immediately. Otherwise it will count the total number of
//free clusters, which takes time
//****************************************************************************
/*
void memoryStatistics (void)
{
	uint32_t freeClusters, totalClusterCount, cluster;
	uint32_t totalMemory, freeMemory;
	uint32_t sector, *value;
	uint16_t i;


	totalMemory = totalClusters * sectorPerCluster / 1024;
	totalMemory *= bytesPerSector;

	printf_P(PSTR("\n\nTOTAL MEMORY: "));

	displayMemory (HIGH, totalMemory);

	freeClusters = getSetFreeCluster (TOTAL_FREE, GET, 0);
//	freeClusters = 0xffffffff;    

	if(freeClusters > totalClusters)
	{
		freeClusterCountUpdated = 0;
		freeClusters = 0;
		totalClusterCount = 0;
		cluster = rootCluster;    
		while(1)
		{
			sector = unusedSectors + reservedSectorCount + ((cluster * 4) / bytesPerSector) ;
			SD_readSingleBlock(sector);
			for(i=0; i<128; i++)
			{
				value = (uint32_t *) &buffer[i*4];
				if(((*value)& 0x0fffffff) == 0)
					freeClusters++;;
				totalClusterCount++;
				if(totalClusterCount == (totalClusters+2)) 
					break;
			}  
			if(i < 128) 
				break;
			cluster+=128;
		} 
	}

	if(!freeClusterCountUpdated)
	getSetFreeCluster (TOTAL_FREE, SET, freeClusters); 	//update FSinfo next free cluster entry
	freeClusterCountUpdated = 1;  //set flag
	freeMemory = freeClusters * sectorPerCluster / 1024;
	freeMemory *= bytesPerSector ;

	printf_P(PSTR("\n FREE MEMORY: "));
	displayMemory (HIGH, freeMemory);
	printf_P(NEWLINE); 
}
*/

//************************************************************
//Function: To convert the uint32_t value of memory into 
//          text string and send to UART
//Arguments: 1. uint8_t flag. If flag is HIGH, memory will be displayed in KBytes, else in Bytes. 
//			 2. uint32_t memory value
//return: none
//************************************************************

//void displayMemory (uint8_t flag, uint32_t memory)
//{
/*	uint8_t memoryString[] = "              Bytes"; 	//19 character long string for memory display
	uint8_t i;

	for(i=12; i>0; i--) 								//converting freeMemory into ASCII string
	{
		if(i==5 || i==9) 
		{
			memoryString[i-1] = ',';  
			i--;
		}
		memoryString[i-1] = (memory % 10) | 0x30;
		memory /= 10;
		if(memory == 0) 
			break;
	}
	if(flag == HIGH)  
		memoryString[13] = 'K';
	transmitString(memoryString);		
*/
	
//	if(flag == HIGH)
//		printf_P(PSTR("%u kBYTES"),memory);
//	else
//		printf_P(PSTR("%u BYTES"),memory);
//}

//********************************************************************
//Function: to delete a specified file from the root directory
//Arguments: pointer to the file name
//return: none
//********************************************************************
void deleteFile (uint8_t *fileName, uint8_t atomic)
{
	uint8_t error;

	error = convertFileName (fileName);
	if(error) 
		return;
	findFiles (DELETE, fileName, fileName, atomic);
}

//********************************************************************
//Function: update the free memory count in the FSinfo sector. 
//			Whenever a file is deleted or created, this function will be called
//			to ADD or REMOVE clusters occupied by the file
//Arguments: #1.flag ADD or REMOVE #2.file size in Bytes
//return: none
//********************************************************************
void freeMemoryUpdate (uint8_t flag, uint32_t size, uint8_t atomic)
{
	uint32_t freeClusters;
  
	if((size % 512) == 0) 								//convert file size into number of clusters occupied
		size = size / 512;
	else 
		size = (size / 512) +1;
	if((size % 8) == 0) 
		size = size / 8;
	else 
		size = (size / 8) +1;

	if(FreeClusterCountUpdated)
	{
		freeClusters = getSetFreeCluster (TOTAL_FREE, GET, 0, atomic);
		if(flag == ADD)
			freeClusters = freeClusters + size;
		else  											//when flag = REMOVE
			freeClusters = freeClusters - size;
		getSetFreeCluster (TOTAL_FREE, SET, freeClusters, atomic);
	}
}

//*******************************************************************
/*
void dumpBufferHex(void)
{
	uint8_t i,j;
	
	GLCD_Clr();
	GLCD_Locate(0,0);
	
	for(i=0;i<8;i++)
	{
		for(j=0;j<16;j++)
		{
			printf("%02X",buffer[(16*i)+j]);
		}
		printf_P(PSTR("\n"));
	}
}
*/

uint16_t getDateFAT(void)
{
	uint16_t dateFAT;
	
	DATE_t *date;
	
	date = RTC_GetDate();
	
	dateFAT  = ((uint16_t) date->y) << 9;
	dateFAT |= ((uint16_t) (date->m & 0x0F)) << 5;
	dateFAT |= date->d & 0x1F;
	
	return(dateFAT);
}

uint16_t getTimeFAT(void)
{
	uint16_t timeFAT;
	TIME_t *time;
	
	time = RTC_GetTime();
	
	timeFAT  = ((uint16_t) time->h) << 11;
	timeFAT |= ((uint16_t) (time->m & 0x1F)) << 5;
	timeFAT |= (time->s >> 1) & 0x1F;
	
	return(timeFAT);
	
}


//******** END ****** www.dharmanitech.com *****
