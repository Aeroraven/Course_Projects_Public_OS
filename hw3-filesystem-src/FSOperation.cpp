#include "FSOperation.h"
#define _CRT_SECURE_NO_WARNINGS
# pragma execution_character_set("utf-8")
# pragma execution_character_set("utf-8")
void FSOperation::fsDiskFormat(fs_VirtualDisk* disk,dd fatCounts,
	dd fatSector ,dd secPerClus,dd bytsPerSec) {
	//建立引导扇区
	fat32_bootSector* bs = new fat32_bootSector;
	bs->info.bpb_BytsPerSec = bytsPerSec;
	bs->info.bpb_NumFATs = fatCounts;
	bs->info.bs_SecPerFAT = fatSector;
	bs->info.bpb_SecPerClus = secPerClus;
	bs->info.bpb_RsvdSecs = 2;
	bs->info.bs_RootFirstClus = 2;
	bs->info.bpb_TotSec32 = sizeof(fs_VirtualDisk) / bytsPerSec;

	//将引导扇区写入磁盘
	FSCore::fsMemcpy((db*)bs, (db*)disk, sizeof(fat32_bootSector));
	
	//建立保留扇区
	fat32_rsvdSector* rs = new fat32_rsvdSector;
	for (int i = 1; i < bs->info.bpb_RsvdSecs; i++) {
		FSCore::fsMemcpy((db*)rs, (db*)disk + (i * bytsPerSec), sizeof(fat32_rsvdSector));
	}
	//建立FAT表
	for (int i = 0; i < bs->info.bpb_NumFATs; i++) {//处理每一个FAT
		db* offset = (db*)disk;
		offset += (bytsPerSec * (0 + bs->info.bpb_RsvdSecs));
		offset += i * fatSector * bytsPerSec;
		FSCore::fsMemset(offset, 0, fatSector * bytsPerSec);
	}
	//清空根目录簇并占用
	fsSetClusterAttr(disk, bs->info.bs_RootFirstClus, FAT_CLUS_LAST);
	fsResetCluster(disk, bs->info.bs_RootFirstClus);
}

db* FSOperation::fsGetFATStart(fs_VirtualDisk* disk) {
	db* offset = (db*)disk;
	fat32_bootSector* bs = (fat32_bootSector*)offset;
	offset += (bs->info.bpb_BytsPerSec * (0 + bs->info.bpb_RsvdSecs));
	return offset;
}

void FSOperation::fsSetClusterAttr(fs_VirtualDisk* disk, dd clusterId,
	dd clusterStatus, dd nextCluster) {
	db* fatStart = fsGetFATStart(disk);//获得FAT起始地址
	fatStart += clusterId * sizeof(fat32_entry);//获得簇对应FAT项
	if (clusterStatus == FAT_CLUS_USED) {
		*((dd*)fatStart) = nextCluster;
	}
	else {
		*((dd*)fatStart) = clusterStatus;
	}
}

dd FSOperation::fsGetClusterAttr(fs_VirtualDisk* disk, dd clusterId) {
	db* fatStart = fsGetFATStart(disk);//获得FAT起始地址
	fatStart += clusterId * sizeof(fat32_entry);//获得簇对应FAT项
	return *((dd*)fatStart);
}

db* FSOperation::fsGetClusterAddr(fs_VirtualDisk* disk, dd clusterId) {
	db* offset = (db*)disk;
	fat32_bootSector* bs = (fat32_bootSector*)offset;
	//跳过保留扇区
	offset += bs->info.bpb_BytsPerSec * (0 + bs->info.bpb_RsvdSecs);
	//跳过FAT表
	offset += bs->info.bpb_NumFATs * bs->info.bs_SecPerFAT * bs->info.bpb_BytsPerSec;
	//跳过已有簇
	offset += bs->info.bpb_SecPerClus * bs->info.bpb_BytsPerSec * (clusterId - 2);
	return offset;
}

void FSOperation::fsReadCluster(fs_VirtualDisk* disk, dd clusterId, db* recp) {
	db* data = fsGetClusterAddr(disk, clusterId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	cout << "Read Cluster:" << clusterId << "Size=" << bs->info.bpb_SecPerClus * bs->info.bpb_BytsPerSec << endl;

	for (int i = 0; i < bs->info.bpb_SecPerClus * bs->info.bpb_BytsPerSec; i++) {
		*recp++ = *data++;
	}
}

void FSOperation::fsResetCluster(fs_VirtualDisk* disk, dd clusterId) {
	db* data = fsGetClusterAddr(disk, clusterId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	for (int i = 0; i < bs->info.bpb_SecPerClus * bs->info.bpb_BytsPerSec; i++) 
		*data++ = 0;
}

void FSOperation::fsReadFileL(fs_VirtualDisk* disk, dd clusterId, db* recp) {
	int idx = 0;
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	while (1) {
		fsReadCluster(disk, clusterId, recp + idx * bs->info.bpb_SecPerClus * bs->info.bpb_BytsPerSec);
		dd cattr = fsGetClusterAttr(disk, clusterId);
		idx++;
		if (cattr >= 0x0FFFFFF0 || cattr <= 0x00000001) {
			return;
		}
		clusterId = cattr;
	}
}

void FSOperation::fsDeleteFileL(fs_VirtualDisk* disk, dd clusterId) {
	int idx = 0;
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	while (1) {
		dd cattr = fsGetClusterAttr(disk, clusterId);
		fsSetClusterAttr(disk, clusterId, FAT_CLUS_FREE);
		if (cattr >= 0x0FFFFFF0 || cattr <= 0x00000001) {
			return;
		}
		clusterId = cattr;
	}
}

dd FSOperation::fsFindFreeClus(fs_VirtualDisk* disk) {
	db* fatTable = fsGetFATStart(disk);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	dd fatSize = bs->info.bs_SecPerFAT * bs->info.bpb_BytsPerSec;
	dd sectors = fatSize / sizeof(fat32_entry);
	for (int i = 0; i < sectors; i++) {
		db* cur = fatTable + i * sizeof(fat32_entry);
		if (i <= 1)
			continue;
		dd val = *((dd*)cur);
		if (val == FAT_CLUS_FREE)
			return i;
	}
	return -1;
}
void FSOperation::fsCreateDirEntry(fs_VirtualDisk* disk, dd clusterDirId, fat32_sFileDirEntry fileEntry) {
	db* offset = fsGetClusterAddr(disk, clusterDirId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	dd entryPerClus = bs->info.bpb_BytsPerSec * bs->info.bpb_SecPerClus / sizeof(fat32_lFileDirEntry);
	dd begin = 0;
	while (1) {
		begin = -1;
		for (int i = 0; i < entryPerClus; i++) {
			int isEmpty = true;
			for (int j = 0; j < sizeof(fat32_sFileDirEntry); j++) {
				if (offset[i * sizeof(fat32_sFileDirEntry) + j]) {
					isEmpty = false;
					break;
				}
			}
			if (isEmpty) {
				begin = i;
				break;
			}
		}
		if (begin != -1) {	//创建目录项
			FSCore::fsMemcpy((db*)&fileEntry, (db*)(&offset[begin * sizeof(fat32_sFileDirEntry)]), sizeof(fat32_sFileDirEntry));
			return;
		}
		dd cattr = fsGetClusterAttr(disk, clusterDirId);
		if (cattr < 0x0FFFFFF0 && cattr > 0x00000001) {
			offset = fsGetClusterAddr(disk, cattr);
		}
		else {
			//创建新簇
			dd newClu = fsFindFreeClus(disk);
			if (newClu == -1)
				break;
			fsLinkCluster(disk, clusterDirId, newClu);
			offset = fsGetClusterAddr(disk, newClu);
		}
	}

}
dd FSOperation::fsGetDirEntry(fs_VirtualDisk* disk, dd clusterDirId, fat32_sFileDirEntry* fileEntry,dd filterFolder) {
	db* offset = fsGetClusterAddr(disk, clusterDirId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	dd entryPerClus = bs->info.bpb_BytsPerSec * bs->info.bpb_SecPerClus / sizeof(fat32_lFileDirEntry);
	dd idx = 0;
	while (1) {
		for (int i = 0; i < entryPerClus; i++) {
			int isEmpty = true;
			for (int j = 0; j < sizeof(fat32_sFileDirEntry); j++) {
				if (offset[i * sizeof(fat32_sFileDirEntry) + j]) {
					isEmpty = false;
					break;
				}
			}
			dd isFolder = 1;
			if (filterFolder) {
				fat32_sFileDirEntry* ent = (fat32_sFileDirEntry*)&offset[i * sizeof(fat32_sFileDirEntry)];
				isFolder = (ent->attr & FAT_ATTR_DIR) > 0;
			}
			if (!isEmpty) {
				FSCore::fsMemcpy(&offset[i * sizeof(fat32_sFileDirEntry)], (db*)(fileEntry + idx++), sizeof(fat32_sFileDirEntry));
			}
		}
		dd cattr = fsGetClusterAttr(disk, clusterDirId);
		if (cattr < 0x0FFFFFF0 && cattr > 0x00000001) {
			clusterDirId = cattr;
			offset = fsGetClusterAddr(disk, cattr);
			cout << "Next Cluster:" <<hex<< (dd*)offset << "/" << cattr << endl;
		}
		else {
			break;
		}
	}
	return idx;
}
void FSOperation::fsLinkCluster(fs_VirtualDisk* disk, dd predCluster, dd succCluster) {
	fsSetClusterAttr(disk, predCluster, FAT_CLUS_USED, succCluster);
	fsSetClusterAttr(disk, succCluster, FAT_CLUS_LAST);
	fsResetCluster(disk, succCluster);
}

void FSOperation::fsWriteCluster(fs_VirtualDisk* disk, dd clusterId,db* wr) {
	cout << "Write cluter:" << clusterId << endl;
	db* data = fsGetClusterAddr(disk, clusterId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	for (int i = 0; i < bs->info.bpb_SecPerClus * bs->info.bpb_BytsPerSec; i++)
		*data++ = *wr++;
}

dd FSOperation::fsCreateFileFullL(fs_VirtualDisk* disk, dd clusterDirId, fat32_sFileDirEntry attrs, db* content) {
	//处理目录
	if (attrs.attr & FAT_ATTR_DIR) {
		FSCore::fsMemset(attrs.extName, 0, sizeof(attrs.extName));
		if (fsCheckFileExist(disk, clusterDirId, attrs)) {
			return 0;
		}
	}
	//预先检查重名文件，有则进行替换
	fsDeleteFileFullL(disk, clusterDirId, attrs);

	fat32_bootSector* bs = (fat32_bootSector*)disk;
	//查找可用簇
	dd avaClu = fsFindFreeClus(disk);
	attrs.startCluHigh = (avaClu & 0xffff0000) >> 4;
	attrs.startCluLow = avaClu & 0xffff;
	
	//填入时间
	tm* timeStru = nullptr;
	time_t curTime;
	time(&curTime);
	curTime += 8 * 3600;

	timeStru = gmtime(&curTime);
	fsSetEntryModDate(attrs, timeStru->tm_year + 1900, timeStru->tm_mon, timeStru->tm_mday);
	fsSetEntryModTime(attrs, timeStru->tm_hour, timeStru->tm_min, timeStru->tm_sec);
	//在目录下创建目录项
	fsCreateDirEntry(disk, clusterDirId, attrs);

	//占用该簇并在其中写入内容
	int idx = 0;
	while (1) {
		fsSetClusterAttr(disk, avaClu, FAT_CLUS_LAST);
		if (attrs.attr & FAT_ATTR_DIR) {
			fsResetCluster(disk, avaClu);
		}
		else {
			fsWriteCluster(disk, avaClu, content + idx);
		}
		if (idx >= attrs.fileLen)
			break;
		dd last = avaClu;
		avaClu = fsFindFreeClus(disk);
		fsLinkCluster(disk, last, avaClu);
		idx += bs->info.bpb_SecPerClus * bs->info.bpb_BytsPerSec;
	}
	return 1;
}
dd FSOperation::fsGetClusterFromDirEntry(fat32_sFileDirEntry entry) {
	return ((entry.startCluHigh << 4) & 0xffff0000) | (entry.startCluLow & 0xffff);
}
dd FSOperation::fsIsSameFile(fat32_sFileDirEntry entryA, fat32_sFileDirEntry entryB) {
	if ((entryA.attr & FAT_ATTR_DIR) != (entryB.attr & FAT_ATTR_DIR)) {
		return false;
	}
	for (int i = 0; i < 8; i++) {
		if (entryA.fileName[i] != entryB.fileName[i]) {
			return false;
		}
	}
	if ((entryA.attr & FAT_ATTR_DIR) == 0) {
		for (int i = 0; i < 3; i++) {
			if (entryA.extName[i] != entryB.extName[i]) {
				return false;
			}

		}
	}
	return true;
}
dd FSOperation::fsDeleteFileFullL(fs_VirtualDisk* disk, dd clusterDirId, fat32_sFileDirEntry attrs) {
	db* offset = fsGetClusterAddr(disk, clusterDirId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	dd entryPerClus = bs->info.bpb_BytsPerSec * bs->info.bpb_SecPerClus / sizeof(fat32_lFileDirEntry);
	dd idx = 0;
	while (1) {
		for (int i = 0; i < entryPerClus; i++) {
			if (fsIsSameFile(*((fat32_sFileDirEntry*)(offset + i * sizeof(fat32_sFileDirEntry))), attrs)) {
				int clu = fsGetClusterFromDirEntry(*((fat32_sFileDirEntry*)(offset + i * sizeof(fat32_sFileDirEntry))));
				fsDeleteFileL(disk, clu);
				//清空目录项
				for (int j = 0; j < sizeof(fat32_sFileDirEntry); j++) {
					offset[i * sizeof(fat32_sFileDirEntry) + j] = 0;
				}
				return 1;
			}
		}
		dd cattr = fsGetClusterAttr(disk, clusterDirId);
		if (cattr < 0x0FFFFFF0 && cattr > 0x00000001) {
			clusterDirId = cattr;
			offset = fsGetClusterAddr(disk, cattr);
		}
		else {
			break;
		}
	}
	return 0;
}

dd FSOperation::fsRenameFileFullL(fs_VirtualDisk* disk, dd clusterDirId, fat32_sFileDirEntry attrs, db* newName) {
	db* offset = fsGetClusterAddr(disk, clusterDirId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	dd entryPerClus = bs->info.bpb_BytsPerSec * bs->info.bpb_SecPerClus / sizeof(fat32_lFileDirEntry);
	dd idx = 0;
	while (1) {
		for (int i = 0; i < entryPerClus; i++) {
			if (fsIsSameFile(*((fat32_sFileDirEntry*)(offset + i * sizeof(fat32_sFileDirEntry))), attrs)) {
				fat32_sFileDirEntry* filep = (fat32_sFileDirEntry*)(offset + i * sizeof(fat32_sFileDirEntry));
				for (int j = 0; j < 8; j++) {
					filep->fileName[j] = newName[j];
				}
			}
		}
		dd cattr = fsGetClusterAttr(disk, clusterDirId);
		if (cattr < 0x0FFFFFF0 && cattr > 0x00000001) {
			clusterDirId = cattr;
			offset = fsGetClusterAddr(disk, cattr);
		}
		else {
			break;
		}
	}
	return 0;
}

dd FSOperation::fsCheckFileExist(fs_VirtualDisk* disk, dd clusterDirId, fat32_sFileDirEntry attrs) {
	db* offset = fsGetClusterAddr(disk, clusterDirId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	dd entryPerClus = bs->info.bpb_BytsPerSec * bs->info.bpb_SecPerClus / sizeof(fat32_lFileDirEntry);
	dd idx = 0;
	while (1) {
		cout << "FileCheck-Find cluster:" << clusterDirId << endl;
		for (int i = 0; i < entryPerClus; i++) {
			if (fsIsSameFile(*((fat32_sFileDirEntry*)(offset + i * sizeof(fat32_sFileDirEntry))), attrs)) {
				return 1;
			}
		}
		dd cattr = fsGetClusterAttr(disk, clusterDirId);
		if (cattr < 0x0FFFFFF0 && cattr > 0x00000001) {
			clusterDirId = cattr;
			offset = fsGetClusterAddr(disk, cattr);
			cout << "Next cluster:" << hex << (dd*)offset << "/" << cattr << endl;
		}
		else {
			break;
		}
	}
	return 0;
}

dd FSOperation::fsCheckFileExistCid(fs_VirtualDisk* disk, dd clusterDirId, fat32_sFileDirEntry attrs) {
	db* offset = fsGetClusterAddr(disk, clusterDirId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	dd entryPerClus = bs->info.bpb_BytsPerSec * bs->info.bpb_SecPerClus / sizeof(fat32_lFileDirEntry);
	dd idx = 0;
	while (1) {
		cout << "FileCheck-Find cluster:" << clusterDirId << endl;
		for (int i = 0; i < entryPerClus; i++) {
			if (fsIsSameFile(*((fat32_sFileDirEntry*)(offset + i * sizeof(fat32_sFileDirEntry))), attrs)) {
				return fsGetClusterFromDirEntry(*((fat32_sFileDirEntry*)(offset + i * sizeof(fat32_sFileDirEntry))));
			}
		}
		dd cattr = fsGetClusterAttr(disk, clusterDirId);
		if (cattr < 0x0FFFFFF0 && cattr > 0x00000001) {
			clusterDirId = cattr;
			offset = fsGetClusterAddr(disk, cattr);
			cout << "Next cluster:" << hex << (dd*)offset << "/" << cattr << endl;
		}
		else {
			break;
		}
	}
	return 0;
}

dd FSOperation::fsRmdir(fs_VirtualDisk* disk, dd clusterDirId, fat32_sFileDirEntry attrs) {
	db* offset = fsGetClusterAddr(disk, clusterDirId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	dd entryPerClus = bs->info.bpb_BytsPerSec * bs->info.bpb_SecPerClus / sizeof(fat32_lFileDirEntry);
	dd idx = 0;
	while (1) {
		for (int i = 0; i < entryPerClus; i++) {
			if (fsIsSameFile(*((fat32_sFileDirEntry*)(offset + i * sizeof(fat32_sFileDirEntry))), attrs)) {
				int clu = fsGetClusterFromDirEntry(*((fat32_sFileDirEntry*)(offset + i * sizeof(fat32_sFileDirEntry))));
				fsRemoveDirL(disk, clu);
				//清空目录项
				for (int j = 0; j < sizeof(fat32_sFileDirEntry); j++) {
					offset[i * sizeof(fat32_sFileDirEntry) + j] = 0;
				}
				return 1;
			}
		}
		dd cattr = fsGetClusterAttr(disk, clusterDirId);
		if (cattr < 0x0FFFFFF0 && cattr > 0x00000001) {
			clusterDirId = cattr;
			offset = fsGetClusterAddr(disk, cattr);
		}
		else {
			break;
		}
	}
	return 0;
}
dd FSOperation::fsRemoveDirL(fs_VirtualDisk* disk, dd clusterId) {
	db* offset = fsGetClusterAddr(disk, clusterId);
	fat32_bootSector* bs = (fat32_bootSector*)disk;
	while (1) {
		dd entryPerClus = bs->info.bpb_BytsPerSec * bs->info.bpb_SecPerClus / sizeof(fat32_sFileDirEntry);
		for (int i = 0; i < entryPerClus; i++) {
			fat32_sFileDirEntry* p = (fat32_sFileDirEntry*)&offset[i * sizeof(fat32_sFileDirEntry)];
			if (p->attr & FAT_ATTR_DIR) {
				fsRemoveDirL(disk, fsGetClusterFromDirEntry(*p));
			}
			else {
				fsDeleteFileL(disk, fsGetClusterFromDirEntry(*p));
			}
		}
		dd cattr = fsGetClusterAttr(disk, clusterId);
		fsSetClusterAttr(disk, clusterId, FAT_CLUS_FREE);
		if (cattr < 0x0FFFFFF0 && cattr > 0x00000001) {
			clusterId = cattr;
			offset = fsGetClusterAddr(disk, cattr);
		}
		else {
			break;
		}
	}
	return 1;
}

void FSOperation::fsSetEntryModTime(fat32_sFileDirEntry& ent, dd hour, dd minute, dd sec) {
	ent.lastEditTime = 0;
	ent.lastEditTime |= (0x1f & (sec >> 1));
	ent.lastEditTime |= (minute << 5);
	ent.lastEditTime |= (hour << 11);
}

void FSOperation::fsSetEntryModDate(fat32_sFileDirEntry& ent, dd year, dd mon, dd day) {
	ent.lastEditDate = 0;
	ent.lastEditDate |= day;
	ent.lastEditDate |= (mon << 5);
	ent.lastEditDate |= ((year - 1980) << 9);
}

void FSOperation::fsGetEntryModDate(fat32_sFileDirEntry ent, dd& year, dd& mon, dd& day) {
	year = (ent.lastEditDate >> 9) + 1980;
	mon = (ent.lastEditDate >> 5) & 0xf;
	day = (ent.lastEditDate) & 0x1f;
}

void FSOperation::fsGetEntryModTime(fat32_sFileDirEntry ent, dd& hour, dd& minute, dd& sec) {
	sec = (ent.lastEditTime & 0x1f) << 1;
	minute = (ent.lastEditTime >> 5) & 0x3f;
	hour = (ent.lastEditTime >> 11);
}

dd FSOperation::fsSave(fs_VirtualDisk* disk, string fileName) {
	fstream op;
	op.open(fileName, ios::out | ios::binary);
	if (!op.good())
		return 0;
	op.write((const char*)disk, sizeof(fs_VirtualDisk));
	op.close();
	return 1;
}

dd FSOperation::fsRead(fs_VirtualDisk* disk, string fileName) {
	fstream op;
	op.open(fileName, ios::in | ios::binary);
	if (!op.good())
		return 0;
	op.read((char*)disk, sizeof(fs_VirtualDisk));
	cout << "Read Completed" << endl;
	op.close();
	return 1;
}