#pragma once
# pragma execution_character_set("utf-8")
typedef unsigned long long dq; //Quad Word
typedef unsigned int dd; //Double Word
typedef unsigned short dw; //Word
typedef unsigned char db; //Byte

constexpr dd DISK_SIZE = 40960000;

constexpr dd FAT_CLUS_RSVD = 0x1;
constexpr dd FAT_CLUS_FREE = 0x0;
constexpr dd FAT_CLUS_LAST = 0x0FFFFFFF;
constexpr dd FAT_CLUS_USED = 0x2; //值会被替换
constexpr dd FAT_CLUS_WORN = 0x0FFFFFF7;

constexpr db FAT_ATTR_DIR = 0x10;

//模拟硬盘
struct fs_VirtualDisk {
	db data[DISK_SIZE];
};

//FAT32 引导区
#pragma  pack(1) 
struct fat32_bootSectorPre {
	//跳转指令 0x02
	db jumpInstruction[3] = { 0xeb,0x58,0x90 };
	//OEM 信息 0x0A
	db oemInfo[8] = { '1','9','5','0','6','4','1' };
	//BPB 信息
	dw bpb_BytsPerSec = 512;	//0x0b
	db bpb_SecPerClus = 1;	//0x0d
	dw bpb_RsvdSecs = 20;	//0x0e
	db bpb_NumFATs = 2;		//0x10
	dw bpb_RootEntCnt = 0;	//0x11
	dw bpb_TotSec16 = 0;	//0x13
	db bpb_Media = 0xf8;		//0x14
	dw bpb_FATSz16 = 0;		//0x16
	dw bpb_SecPerTrk = 0x003F;	//0x18
	dw bpb_NumHeads = 0xff;	//0x1a
	dd bpb_HiddSec = 0;		//0x1c
	dd bpb_TotSec32;	//0x20
	dd bs_SecPerFAT;	//0x24
	dw bs_Sign;			//0x28
	dw bs_Ver = 0x0;			//0x2A
	dd bs_RootFirstClus = 2; //0x2C
	dw bs_FsInfoFirstClus = 1; //0x30
	dw bs_DBRSecId = 6;		//0x32
	db bs_Rsvd[12] = { 0x00 };		//0x34
	db bs_BIOSDrv = 0x80;		//0x40
	db bs_Rsvd2 = 0x00;		//0x41
	db bs_ExtBootSign = 0x28;	//0x42
	dd bs_VolId = 0x33391CFE;		//0x43
	db bs_VolLab[11] = "1950641HD";	//0x47
	db bs_FSType[8] = "FAT32";
};
struct fat32_bootSector{
	fat32_bootSectorPre info;
	db prog[510 - sizeof(fat32_bootSectorPre)] = "This is not a bootable disk. "
		"Please insert a bootable floppy and press any key to try again";
	db endSign[2] = { 0x55,0xAA };
};
#pragma pack()   

//FAT32 保留扇区
struct fat32_rsvdSector {
	db extBootSign[4] = { 0x52,0x52,0x61,0x41 };
	db rsvd[480] = { 0 };
	db fsInfoSign[4] = { 0x72,0x72,0x41,0x61 };
	dd freeClus = 8888;
	dd nextFreeClus = 3;
	db rsvd2[14] = { 0 };
	db endSign[2] = { 0x55,0xAA };
};

//FAT32 表项
struct fat32_entry {
	dd nextClus;
};
//FAT32 文件名目录项
struct fat32_sFileDirEntry { //短文件名
	db fileName[8];
	db extName[3];
	db attr;
	db rsvd;
	db miliTime;
	dw createTime;
	dw createDate;
	dw lastVisitDate;
	dw startCluHigh;
	dw lastEditTime;
	dw lastEditDate;
	dw startCluLow;
	dd fileLen;
};

struct fat32_lFileDirEntry { //长文件名
	db attr;
	db fileName1[10];
	db lfileDirSign;
	db rsvd;
	db chkval;
	db fileName2[12];
	dw startClu;
	db fileName3[4];
};

