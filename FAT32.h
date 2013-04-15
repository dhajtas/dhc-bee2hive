//********************************************************
// **** ROUTINES FOR FAT32 IMPLEMATATION OF SD CARD *****
//********************************************************
//Controller: ATmega32 (Clock: 8 Mhz-internal)
//Compiler	: AVR-GCC (winAVR with AVRStudio)
//Version 	: 2.4
//Author	: CC Dharmani, Chennai (India)
//			  www.dharmanitech.com
//Date		: 17 May 2010
//********************************************************

//Link to the Post: http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html

#ifndef _FAT32_H_
#define _FAT32_H_

//Structure to access Master Boot Record for getting info about partioions
typedef struct MBRinfo_Structure
{
	uint8_t		nothing[446];		//ignore, placed here to fill the gap in the structure
	uint8_t		partitionData[64];	//partition records (16x4)
	uint16_t	signature;			//0xaa55
} MBRinfo;

//Structure to access info of the first partioion of the disk 
typedef struct partitionInfo_Structure
{ 				
	uint8_t		status;				//0x80 - active partition
	uint8_t 	headStart;			//starting head
	uint16_t	cylSectStart;		//starting cylinder and sector
	uint8_t		type;				//partition type 
	uint8_t		headEnd;			//ending head of the partition
	uint16_t	cylSectEnd;			//ending cylinder and sector
	uint32_t	firstSector;		//total sectors between MBR & the first sector of the partition
	uint32_t	sectorsTotal;		//size of this partition in sectors
} partitionInfo;

//Structure to access boot sector data FAT32 only
typedef struct BS_Structure{
	uint8_t jumpBoot[3]; 			//default: 	0x009000EB
	uint8_t OEMName[8];				//here		"MSDOS5.0"
	uint16_t bytesPerSector; 		//default: 	0x0200
	uint8_t sectorPerCluster;		//here 		0x08
	uint16_t reservedSectorCount;	//here:		0x0026
	uint8_t numberofFATs;			//default:	0x02
	uint16_t rootEntryCount;		//default 	0x0000 in FAT32
	uint16_t totalSectors_F16; 		//must be 0 for FAT32
	uint8_t mediaType;				//here:		0xF8	fixed disk
	uint16_t FATsize_F16; 			//must be 0 for FAT32
	uint16_t sectorsPerTrack;
	uint16_t numberofHeads;
	uint32_t hiddenSectors;			//here: 	0x00000089
	uint32_t totalSectors_F32;		//here:		0x003A9F77
	uint32_t FATsize_F32; 			//here:		0x00000EA1 - count of sectors occupied by one FAT
	uint16_t extFlags;
	uint16_t FSversion; 			//0x0000 (defines version 0.0)
	uint32_t rootCluster; 			//first cluster of root directory (=2)
	uint16_t FSinfo; 				//sector number of FSinfo structure (=1)
	uint16_t BackupBootSector;		//here:		0x0006
	uint8_t reserved[12];
	uint8_t driveNumber;
	uint8_t reserved1;
	uint8_t bootSignature;			//here:		0x29
	uint32_t volumeID;
	uint8_t volumeLabel[11]; 		//"NO NAME    "
	uint8_t fileSystemType[8]; 		//"FAT32   "
	uint8_t bootData[420];
	uint16_t bootEndSignature; 		//0xaa55
} BS;

//Structure to access FSinfo sector data
typedef struct FSInfo_Structure
{
	uint32_t leadSignature; 	//0x41615252
	uint8_t reserved1[480];
	uint32_t structureSignature; //0x61417272
	uint32_t freeClusterCount; 	//initial: 0xffffffff
	uint32_t nextFreeCluster; 	//initial: 0xffffffff
	uint8_t reserved2[12];
	uint32_t trailSignature; 	//0xaa550000
} FSinfo;

//Structure to access Directory Entry in the FAT32
typedef struct dir32_Structure
{
	uint8_t name[11];
	uint8_t attrib; 			//file attributes
	uint8_t NTreserved; 		//always 0
	uint8_t timeTenth; 			//tenths of seconds, set to 0 here
	uint16_t createTime; 		//time file was created
	uint16_t createDate; 		//date file was created
	uint16_t lastAccessDate;
	uint16_t firstClusterHI; 	//higher word of the first cluster number
	uint16_t writeTime; 		//time of last write
	uint16_t writeDate; 		//date of last write
	uint16_t firstClusterLO; 	//lower word of the first cluster number
	uint32_t fileSize; 			//size of file in bytes
} DIR;

//Attribute definitions for file/directory
#define ATTR_READ_ONLY     0x01
#define ATTR_HIDDEN        0x02
#define ATTR_SYSTEM        0x04
#define ATTR_VOLUME_ID     0x08
#define ATTR_DIRECTORY     0x10
#define ATTR_ARCHIVE       0x20
#define ATTR_LONG_NAME     0x0f


#define DIR_ENTRY_SIZE     0x32
#define EMPTY              0x00
#define DELETED            0xe5
#define GET     0
#define SET     1
#define READ	0
#define VERIFY  1
#define ADD		0
#define REMOVE	1
#define LOW		0
#define HIGH	1	
#define TOTAL_FREE   1
#define NEXT_FREE    2
#define GET_LIST     0
#define GET_FILE     1
#define DELETE		 2
#define RENAME		 3
#define EOF		0x0ffffff8


//************* external variables *************
volatile uint32_t FirstDataSector, RootCluster, TotalClusters;
volatile uint16_t  BytesPerSector, SectorPerCluster, ReservedSectorCount;
uint32_t UnusedSectors; 
uint16_t FSIOffset;
extern uint16_t AppendStartSector __attribute__ ((section (".noinit")));
extern uint32_t AppendFileSector __attribute__ ((section (".noinit")));
extern uint32_t AppendFileLocation __attribute__ ((section (".noinit")));
extern uint32_t FileSize __attribute__ ((section (".noinit")));
extern uint32_t AppendStartCluster __attribute__ ((section (".noinit")));

//global flag to keep track of free cluster count updating in FSinfo sector
extern uint8_t FreeClusterCountUpdated __attribute__ ((section (".noinit")));


//************* functions *************
uint8_t getBootSectorData (uint8_t atomic);
uint32_t getFirstSector(uint32_t clusterNumber);
uint32_t getSetFreeCluster(uint8_t totOrNext, uint8_t get_set, uint32_t FSEntry, uint8_t atomic);
DIR* findFiles (uint8_t flag, uint8_t *fileName, uint8_t *newfileName, uint8_t atomic);
uint32_t getSetNextCluster (uint32_t clusterNumber,uint8_t get_set,uint32_t clusterEntry, uint8_t atomic);
uint8_t readFile (uint8_t flag, uint8_t *fileName, uint8_t atomic);
uint8_t convertFileName (uint8_t *fileName);
uint8_t renameFile(uint8_t *oldName, uint8_t *newName, uint8_t atomic);
uint8_t openFile(uint8_t *fileName, uint8_t flag, uint8_t atomic);
void writeFile (uint8_t atomic);
void writeSpectrum (uint8_t end_file, uint16_t size, uint16_t *spectrum, uint8_t atomic);
void writeZeroes (uint8_t end_file, uint16_t size, uint8_t atomic);
uint8_t closeFile(uint8_t atomic);
uint32_t searchNextFreeCluster (uint32_t startCluster,uint8_t atomic);
//void memoryStatistics (void);
//void displayMemory (uint8_t flag, uint32_t memory);
void deleteFile (uint8_t *fileName, uint8_t atomic);
void freeMemoryUpdate (uint8_t flag, uint32_t size, uint8_t atomic);
//void dumpBufferHex(void);
uint16_t getDateFAT(void);
uint16_t getTimeFAT(void);
uint8_t FAT32_InitFS(void);


#endif
