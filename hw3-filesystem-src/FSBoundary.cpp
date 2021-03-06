#include "FSBoundary.h"
#include <string>
#include <iomanip>
# pragma execution_character_set("utf-8")
using namespace std;
dd FSBoundary::currentDirCluster = 2;
fs_VirtualDisk* FSBoundary::vdisk = new fs_VirtualDisk;
fat32_sFileDirEntry* FSBoundary::fde = new fat32_sFileDirEntry[3000];
fat32_sFileDirEntry* FSBoundary::fde2 = new fat32_sFileDirEntry[3000];
fat32_sFileDirEntry* FSBoundary::fdSelected = new fat32_sFileDirEntry;
FSOperation* FSBoundary::fs = new FSOperation();
db* FSBoundary::fsBuffer = new db[4000];
vector<ui_tree_node>* FSBoundary::path_route = new vector<ui_tree_node>;

void FSBoundary::fbGoto(ui_tree_node x) {
    path_route->push_back(x);
    currentDirCluster = x.clusterId;
}

void FSBoundary::fbGoParent() {
    if (path_route->size() > 1) {
        path_route->pop_back();
        currentDirCluster = (*path_route)[path_route->size() - 1].clusterId;
    }
}

QString FSBoundary::fqConvert(db* p,dd digits) {
    if (digits == -1) {
        return QString(QLatin1String((char*)p));
    }
    else {
        db* q = new db[digits + 1];
        FSCore::fsMemcpy(p, q, digits);
        q[digits] = 0;
        return QString(QLatin1String((char*)q));
    }
}

dd FSBoundary::fbCheckFileNameValid(QString v) {
    string x = v.toStdString();
    if (x.size() == 0)
        return false;
    for (int i = 0; i < x.size(); i++) {
        if (x[i] == '*')
            return false;
        if (x[i] < 33 || x[i]>126)
            return false;
        if (x[i] == '/')
            return false;
        if (x[i] == '\\')
            return false;
        if ((unsigned char)x[i] > 127)
            return false;
    }
    return true;
}

string FSBoundary::fbGetDevInfo() {
    stringstream ss;
    fat32_bootSector* bs = (fat32_bootSector*)vdisk;
    ss << "[硬件信息]" << endl << endl;
    ss << setiosflags(ios::left);
    ss << "扇区字节数      :" << (dd)bs->info.bpb_BytsPerSec << endl;
    ss << "簇扇区数        :" << (dd)bs->info.bpb_SecPerClus << endl;
    ss << "保留扇区数      :" << (dd)bs->info.bpb_RsvdSecs << endl;
    ss << "FAT数           :" << (dd)bs->info.bpb_NumFATs << endl;
    ss << "媒介            :" << (dd)bs->info.bpb_Media << endl;
    ss << "磁道扇区数      :" << (dd)bs->info.bpb_SecPerTrk << endl;
    ss << "磁头数          :" << (dd)bs->info.bpb_NumHeads << endl;
    ss << "隐藏扇区数      :" << (dd)bs->info.bpb_HiddSec << endl;
    ss << "总扇区数        :" << (dd)bs->info.bpb_TotSec32 << endl;
    ss << "FAT扇区数       :" << (dd)bs->info.bs_SecPerFAT << endl;
    ss << "根目录扇区      :" << (dd)bs->info.bs_RootFirstClus << endl;
    ss << "文件信息系统扇区:" << (dd)bs->info.bs_FsInfoFirstClus << endl;
    ss << "DBR扇区         :" << (dd)bs->info.bs_DBRSecId << endl;
    ss << "卷编号          :" << (dd)bs->info.bs_VolId << endl;
    ss << "文件系统类型    :" <<bs->info.bs_FSType << endl;
    ss << "卷标            :" << bs->info.bs_VolLab << endl;
    return ss.str();
}

void FSBoundary::fbGetDirList(QStandardItemModel** p, dd fileCluster) {
    if (*p != nullptr)
        delete* p;
    int x = fs->fsGetDirEntry(vdisk, fileCluster, fde);
    *p = new QStandardItemModel(x, 5);
    (*p)->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("文件名")));
    (*p)->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("类型")));
    (*p)->setHorizontalHeaderItem(2, new QStandardItem(QObject::tr("大小")));
    (*p)->setHorizontalHeaderItem(3, new QStandardItem(QObject::tr("最后修改时间")));
    (*p)->setHorizontalHeaderItem(4, new QStandardItem(QObject::tr("描述")));
    for (int row = 0; row < x; ++row) {
        QStandardItem* item;
        cout << "文件属性-Row:" << row << "=" << (dd)fde[row].attr << endl;
        if (fde[row].attr & FAT_ATTR_DIR) {
            item = new QStandardItem(
                QString("%0").arg(fqConvert(fde[row].fileName, 8)));
            (*p)->setItem(row, 0, item);
            item = new QStandardItem(QString("目录"));
            (*p)->setItem(row, 1, item);
            item = new QStandardItem(
                QString("-"));
            (*p)->setItem(row, 2, item);
        }
        else {
            item = new QStandardItem(
                QString("%0.%1").arg(fqConvert(fde[row].fileName, 8)).
                    arg(fqConvert(fde[row].extName, 3)));
            (*p)->setItem(row, 0, item);
            item = new QStandardItem(
                QString("文件 (.%0)").arg(fqConvert(fde[row].extName, 3)));
            (*p)->setItem(row, 1, item);
            item = new QStandardItem(
                QString("%0 Bytes").arg(fde[row].fileLen));
            (*p)->setItem(row, 2, item);
        }
        dd H, Mi, S, Y, Mo, D;
        fs->fsGetEntryModTime(fde[row], H, Mi, S);
        fs->fsGetEntryModDate(fde[row], Y, Mo, D);
        Mo++;
        item = new QStandardItem(
            QString("%0-%1-%2 %3:%4:%5").arg(Y).arg(Mo).arg(D)
            .arg(H).arg(Mi).arg(S));
        (*p)->setItem(row, 3, item);

        item = new QStandardItem(
            QString("%0").arg(1));
        (*p)->setItem(row, 4, item);
    }
}

void FSBoundary::fbRouting(string path) {
    vector<string> x;
    vector<ui_tree_node> path_route2;
    FSCore::fsSplit(path, x, "\\");
    string hist;
    dd currentDir = 0;
    cout << x.size() << "---" << endl;
    if (x.size() == 0) {
        QMessageBox::critical(0,
            "文件系统", "无效路径",
            QMessageBox::Ok | QMessageBox::Default,
            0, 0);
        return;
    }
    if (path[x.size() - 1] != '\\') {
        path += '\\';
    }
    for (int i = 0; i < x.size(); i++) {
        if (i==0 && x[i] != "Root") {
            QMessageBox::critical(0,
                "文件系统", "无效的根目录名",
                QMessageBox::Ok | QMessageBox::Default,
                0, 0);
            return;
        }
        if (i > 0) {
            
            fat32_sFileDirEntry p;
            p.attr = 0 | FAT_ATTR_DIR;
            FSCore::fsMemset(p.fileName, 0, sizeof(p.fileName));
            for (int j = 0; j < min(8, (int)x[i].length());j++) {
                p.fileName[j] = x[i][j];
            }
            int t = fs->fsCheckFileExistCid(vdisk, currentDir, p);
            if (t == 0) {
                QMessageBox::critical(0,
                    "文件系统", QString::fromStdString("目录\""+x[i]+"\" 不存在于目录\""+hist+"\"下"),
                    QMessageBox::Ok | QMessageBox::Default,
                    0, 0);
                return;
            }
            else {
                fs->fsGetDirEntry(vdisk, t, fde2);
                currentDir = t;
            }
        }
        hist += x[i] + '\\';
        
        if (i == 0) {
            fat32_bootSector* bs = (fat32_bootSector*)vdisk;
            fs->fsGetDirEntry(vdisk, bs->info.bs_RootFirstClus, fde2);
            currentDir = bs->info.bs_RootFirstClus;
            path_route2.push_back({ QString::fromStdString(x[i]),currentDir });
            continue;
        }
        else {
            path_route2.push_back({ QString::fromStdString(x[i]),currentDir });
        }
    }
    QMessageBox::information(0,
        "文件系统", "已跳转到目标目录",
        QMessageBox::Ok | QMessageBox::Default,
        0, 0);
    path_route->clear();
    for (int i = 0; i < path_route2.size(); i++) {
        (*path_route).push_back((path_route2)[i]);
    }
    currentDirCluster = currentDir;

}

dd FSBoundary::fbGetAvaSpace() {
    db* fatst = fs->fsGetFATStart(vdisk);
    fat32_bootSector* bs = (fat32_bootSector*)vdisk;
    int ans = 0;
    int idx = 0;
    for (int i = 0; i < bs->info.bpb_BytsPerSec * bs->info.bs_SecPerFAT;) {
        dd* x = (dd*)(fatst + i );
        if (*x == 0 && idx >= 2) {
            ans++;
        }
        i += 4;
        idx++;
    }
    return ans * bs->info.bpb_BytsPerSec;
}