#pragma once
#include "FSDefs.h"
#include "FSCore.h"
#include <iostream>
#include <ctime>
#include <fstream>
using namespace std;

class FSOperation{
public:
	//硬盘格式化为FAT32
	static void fsDiskFormat(
		fs_VirtualDisk* disk,
		dd fatCounts=2,
		dd fatSector=616,
		dd secPerClus=1,
		dd bytsPerSec=512
	);

	//获得FAT起始项
	static db* fsGetFATStart(fs_VirtualDisk* disk);

	//在FAT表中设定簇状态
	static void fsSetClusterAttr(
		fs_VirtualDisk* disk,
		dd clusterId,
		dd clusterStatus,
		dd nextCluster= FAT_CLUS_LAST
	);
	
	//获取FAT表状态
	static dd fsGetClusterAttr(
		fs_VirtualDisk* disk,
		dd clusterId
	);

	//获得数据区第一个簇的地址
	static db* fsGetClusterAddr(
		fs_VirtualDisk* disk,
		dd clusterId
	);

	//读取簇
	static void fsReadCluster(
		fs_VirtualDisk* disk,
		dd clusterId,
		db* recp
	);

	//清空簇
	static void fsResetCluster(
		fs_VirtualDisk* disk,
		dd clusterId
	);

	//读取文件
	static void fsReadFileL(
		fs_VirtualDisk* disk,
		dd clusterId,
		db* recp
	);

	//删除文件
	static void fsDeleteFileL(
		fs_VirtualDisk* disk,
		dd clusterId
	);

	//查找第一个空闲簇
	static dd fsFindFreeClus(
		fs_VirtualDisk* disk
	);

	//添加目录项
	static void fsCreateDirEntry(
		fs_VirtualDisk* disk,
		dd clusterDirId,
		fat32_sFileDirEntry fileEntry
	);

	//返回目录项列表
	static dd fsGetDirEntry(
		fs_VirtualDisk* disk,
		dd clusterDirId,
		fat32_sFileDirEntry* fileEntry,
		dd filterFolder=0
	);

	//连接簇链
	static void fsLinkCluster(
		fs_VirtualDisk* disk,
		dd predCluster,
		dd succCluster
	);

	//写入簇
	static void fsWriteCluster(
		fs_VirtualDisk* disk,
		dd clusterId,
		db* wr
	);

	//创建一般文件
	static dd fsCreateFileFullL(
		fs_VirtualDisk* disk,
		dd clusterDirId,
		fat32_sFileDirEntry attrs,
		db* content
	);

	//比较两个目录项是否相同
	static dd fsIsSameFile(
		fat32_sFileDirEntry entryA,
		fat32_sFileDirEntry entryB
	);

	//删除一般文件
	static dd fsDeleteFileFullL(
		fs_VirtualDisk* disk,
		dd clusterDirId,
		fat32_sFileDirEntry attrs
	);

	static dd fsRenameFileFullL(
		fs_VirtualDisk* disk,
		dd clusterDirId,
		fat32_sFileDirEntry attrs,
		db* newName
	);

	//获取目录项中的簇
	static dd fsGetClusterFromDirEntry(
		fat32_sFileDirEntry entry
	);

	//检查文件存在
	static dd fsCheckFileExist(
		fs_VirtualDisk* disk,
		dd clusterDirId,
		fat32_sFileDirEntry attrs
	);

	static dd fsCheckFileExistCid(
		fs_VirtualDisk* disk,
		dd clusterDirId,
		fat32_sFileDirEntry attrs
	);

	//删除目录
	static dd fsRemoveDirL(
		fs_VirtualDisk* disk,
		dd clusterId
	);
	static dd fsRmdir(
		fs_VirtualDisk* disk,
		dd clusterDirId,
		fat32_sFileDirEntry attrs
	);

	//设定时间
	static void fsSetEntryModTime(
		fat32_sFileDirEntry& ent,
		dd hour,
		dd minute,
		dd sec
	);

	//设定日期
	static void fsSetEntryModDate(
		fat32_sFileDirEntry& ent,
		dd year,
		dd mon,
		dd day
	);

	static void fsGetEntryModDate(
		fat32_sFileDirEntry ent,
		dd& year,
		dd& mon,
		dd& day
	);

	static void fsGetEntryModTime(
		fat32_sFileDirEntry ent,
		dd& hour,
		dd& minute,
		dd& sec
	);

	//将内存数据转存为文件
	static dd fsSave(
		fs_VirtualDisk* disk,
		string fileName
	);

	//读取IMG
	static dd fsRead(
		fs_VirtualDisk* disk,
		string fileName
	);
};

