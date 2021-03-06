#include "OSHomework3.h"
# pragma execution_character_set("utf-8")
void OSHomework3::fsqRefreshWindow() {
    fb.fbGetDirList(&stModel, fb.currentDirCluster);
    ui.fileTable->setModel(stModel);
    fsqUpdateInfo();
}

void OSHomework3::fsqFormat() {
    fb.fs->fsDiskFormat(fb.vdisk);
    QMessageBox::information(0,
        "文件系统", "硬盘格式化完成",
        QMessageBox::Ok | QMessageBox::Default,
        0, 0);
}
void OSHomework3::fsqNewFileL() {
    fb.fdSelected->attr = 0;
    fb.fdSelected->fileLen = 0;
    if (!fb.fbCheckFileNameValid(ui.editorName->toPlainText()+ ui.editorExt->toPlainText())) {
        QMessageBox::information(0,
            "文件系统", "文件命名无效，不要输入*、\\以及中文字符等",
            QMessageBox::Ok | QMessageBox::Default,
            0, 0);
        return;
    }
    if (!fb.fbCheckFileNameValid(ui.editorName->toPlainText())) {
        QMessageBox::information(0,
            "文件系统", "文件命名无效，不要输入*、\\以及中文字符等",
            QMessageBox::Ok | QMessageBox::Default,
            0, 0);
        return;
    }
    //文件名
    QByteArray ba = ui.editorName->toPlainText().toLatin1();
    memset(fb.fdSelected->fileName, 0, 8);
    memcpy(fb.fdSelected->fileName, ba.data(), min(8, ba.length()));

    //文件内容
    char* chStr = NULL;
    QByteArray ba2 = ui.editorBody->toPlainText().toLatin1();
    chStr = new char[ba2.length() + 1];
    memset(chStr, 0, ba2.length());
    memcpy(chStr, ba2.data(), ba2.length());
    chStr[ba2.length()] = '\0';

    //文件长度
    fb.fdSelected->fileLen = ba2.length();
    //扩展名
    QByteArray ba3 = ui.editorExt->toPlainText().toLatin1();
    FSCore::fsMemset(fb.fdSelected->extName, 0, 3);
    memcpy(fb.fdSelected->extName, ba3.data(), min(ba3.length(), 3));

    //在FS中创建文件
    fb.fs->fsCreateFileFullL(fb.vdisk, fb.currentDirCluster, *(fb.fdSelected), (db*)chStr);
    delete[] chStr;

    QMessageBox::information(0,
        "文件系统", "文件已经保存",
        QMessageBox::Ok | QMessageBox::Default,
        0, 0);
    //刷新文件列表
    fsqRefreshWindow();
}

void OSHomework3::fsqNewFileFolder() {
    fb.fdSelected->attr = 0 | FAT_ATTR_DIR;
    fb.fdSelected->fileLen = 0;
    QString text;
    while (1) {
        bool ok;
        text = QInputDialog::getText(this, tr("新建文件夹"),
            tr("输入文件夹名称(小于8字节)"), QLineEdit::Normal,
            "Folder", &ok);
        if (!ok)
            return;
        ok = 0;
        QByteArray ba = text.toLatin1();
        memset(fb.fdSelected->fileName, 0, 8);
        memcpy(fb.fdSelected->fileName, ba.data(), min(8, ba.length()));
        if (!fb.fbCheckFileNameValid(text)) {
            QMessageBox::information(0,
                "文件系统", "文件名无效，不要输入*、\\以及中文字符等",
                QMessageBox::Ok | QMessageBox::Default,
                0, 0);
            continue;
        }
        if (fb.fs->fsCheckFileExist(fb.vdisk, fb.currentDirCluster, *(fb.fdSelected))) {
            QMessageBox::information(0,
                "文件系统", "文件夹不能重名",
                QMessageBox::Ok | QMessageBox::Default,
                0, 0);
            continue;
        }
        break;
    }
 
    //文件长度
    fb.fdSelected->fileLen = 0;

    //扩展名
    db n3[3] = { 0,0,0 };
    FSCore::fsMemset(fb.fdSelected->extName, 0, 3);
    memcpy(fb.fdSelected->extName, n3, 3);

    //在FS中创建文件
    char chStr[10];
    fb.fs->fsCreateFileFullL(fb.vdisk, fb.currentDirCluster, *(fb.fdSelected), (db*)chStr);

    //刷新文件列表
    fsqRefreshWindow();
}


void OSHomework3::fsqNewFile() {
    //刷新文件列表
    fsqRefreshWindow();
}

void OSHomework3::fsqOpenFile() {
    int x = ui.fileTable->currentIndex().row();
    if (x == -1)
        return;
    if (fb.fde[x].attr & FAT_ATTR_DIR) {
        ui_tree_node p;
        p.name = fb.fqConvert(fb.fde[x].fileName);
        p.clusterId = fb.fs->fsGetClusterFromDirEntry(fb.fde[x]);
        fb.fbGoto(p);
        fsqRefreshWindow();
        cout << "Folder Open Done" <<p.clusterId<< endl;
        fsqShowPath();
    }
    else {
        ui.tabWidget->setCurrentIndex(1);
        int clu = fb.fs->fsGetClusterFromDirEntry(fb.fde[x]);
       
        delete[] fb.fsBuffer;
        fat32_bootSector* bs = (fat32_bootSector*)fb.vdisk;
        int t = 0;
        for (t = 0; t < fb.fde[x].fileLen;) {
            t += bs->info.bpb_BytsPerSec;
            cout << "Attempt to allocate:" << t << endl;
        }
        t += bs->info.bpb_BytsPerSec;
        fb.fsBuffer = new db[t + 10];
        cout << "Allocate Memory: " << t + 10 << endl;
        FSCore::fsMemset(fb.fsBuffer, 0, t + 10);
        fb.fs->fsReadFileL(fb.vdisk, clu, fb.fsBuffer);

        QString fileBody;
        fileBody = fb.fqConvert(fb.fsBuffer);
        ui.editorBody->setPlainText(fileBody);
        ui.editorName->setPlainText(fb.fqConvert(fb.fde[x].fileName, 8));
        ui.editorExt->setPlainText(fb.fqConvert(fb.fde[x].extName, 3));
        cout << "File Open Done" << endl;
    }
}

void OSHomework3::fsqRename() {
    int x = ui.fileTable->currentIndex().row();
    if (x == -1)
        return;
    QString text;
    while (1) {
        bool ok;
        text = QInputDialog::getText(this, tr("重命名"),
            tr("输入文件名称(小于8字节),不包含扩展名"), QLineEdit::Normal,
            "", &ok);
        if (!ok)
            return;
        ok = 0;
        QByteArray ba = text.toLatin1();
        memset(fb.fdSelected->fileName, 0, 8);
        memcpy(fb.fdSelected->fileName, ba.data(), min(8, ba.length()));
        if (!fb.fbCheckFileNameValid(text)) {
            QMessageBox::information(0,
                "文件系统", "文件名无效，不要输入*、\\以及中文字符等",
                QMessageBox::Ok | QMessageBox::Default,
                0, 0);
            continue;
        }
        if (fb.fs->fsCheckFileExist(fb.vdisk, fb.currentDirCluster, *(fb.fdSelected))) {
            QMessageBox::information(0,
                "文件系统", "文件名冲突或没有修改",
                QMessageBox::Ok | QMessageBox::Default,
                0, 0);
            continue;
        }
        break;
    }
    fb.fs->fsRenameFileFullL(fb.vdisk, fb.currentDirCluster, fb.fde[x], (fb.fdSelected->fileName));
    QMessageBox::information(0,
        "文件系统", "重命名完成",
        QMessageBox::Ok | QMessageBox::Default,
        0, 0);
    fsqRefreshWindow();
}

void OSHomework3::fsqClose() {
    ui.tabWidget->setCurrentIndex(0);
}

void OSHomework3::fsqRoute() {
    fb.fbRouting(ui.txtPath->toPlainText().toStdString());
    fsqRefreshWindow();
}

void OSHomework3::fsqSave() {
    if (fb.fs->fsSave(fb.vdisk, "virtual-disk.img")) {
        QMessageBox::information(0,
            "信息转储", "文件存储器中内容已经存入virtual-disk.img中！",
            QMessageBox::Ok | QMessageBox::Default,
            0, 0);
    }
    else {
        QMessageBox::information(0,
            "信息转储", "保存信息时发生错误！",
            QMessageBox::Ok | QMessageBox::Default,
            0, 0);
    }
    
}

void OSHomework3::fsqRead() {
    if (fb.fs->fsRead(fb.vdisk, "virtual-disk.img")) {
        QMessageBox::information(0,
            "转储信息载入", "已经将virtual-disk.img的信息载入内存！",
            QMessageBox::Ok | QMessageBox::Default,
            0, 0);
        fsqRefreshWindow();
    }
    else {
        QMessageBox::information(0,
            "转储信息载入", "将virtual-disk.img的信息载入内存时发生错误！",
            QMessageBox::Ok | QMessageBox::Default,
            0, 0);
    }
}

void OSHomework3::fsqDeleteFile() {
    int x = ui.fileTable->currentIndex().row();
    if (x == -1)
        return;
    int clu = fb.fs->fsGetClusterFromDirEntry(fb.fde[x]);
    if (fb.fde[x].attr & FAT_ATTR_DIR) {
        fb.fs->fsRmdir(fb.vdisk, fb.currentDirCluster, fb.fde[x]);
        cout << "目录删除完成" << endl;
    }
    else {
        fb.fs->fsDeleteFileFullL(fb.vdisk, fb.currentDirCluster, fb.fde[x]);
        cout << "文件删除完成" << endl;
    }
    QMessageBox::information(0,
        "文件系统", "删除完成",
        QMessageBox::Ok | QMessageBox::Default,
        0, 0);
    fsqRefreshWindow();
}

void OSHomework3::fsqGotoNewFile() {
    ui.tabWidget->setCurrentIndex(1);
}

OSHomework3::OSHomework3(QWidget *parent): QMainWindow(parent){
    ui.setupUi(this);
    
    QPalette palette(this->palette());
    palette.setColor(QPalette::Background, QColor(100, 100, 100));
    this->setPalette(palette);

    fb.currentDirCluster = 2;
    FSCore::fsMemset((db*)fb.vdisk, 0, sizeof(fs_VirtualDisk));
    fb.fs->fsDiskFormat(fb.vdisk);
    //格式化结束！！！
    fat32_bootSector* bs = (fat32_bootSector*)fb.vdisk;
    fb.currentDirCluster = bs->info.bs_RootFirstClus;
    fb.fbGoto({ "Root",bs->info.bs_RootFirstClus });
    fsqShowPath();
    ui.fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(ui.btnRefresh, &QPushButton::clicked, this, &OSHomework3::fsqRefreshWindow);
    connect(ui.btnCreateFile, &QPushButton::clicked, this, &OSHomework3::fsqNewFileL);
    connect(ui.btnFormat, &QPushButton::clicked, this, &OSHomework3::fsqFormat);
    connect(ui.btnOpenFile, &QPushButton::clicked, this, &OSHomework3::fsqOpenFile);
    connect(ui.btnDeleteFile, &QPushButton::clicked, this, &OSHomework3::fsqDeleteFile);
    connect(ui.btnNewFolder, &QPushButton::clicked, this, &OSHomework3::fsqNewFileFolder);
    connect(ui.btnGoBack, &QPushButton::clicked, this, &OSHomework3::fsqGoParentFolder);
    connect(ui.btnSave, &QPushButton::clicked, this, &OSHomework3::fsqSave);
    connect(ui.btnRead, &QPushButton::clicked, this, &OSHomework3::fsqRead);
    connect(ui.btnRefDev, &QPushButton::clicked, this, &OSHomework3::fsqGetDevInf);
    connect(ui.btnGotoPath, &QPushButton::clicked, this, &OSHomework3::fsqRoute);
    connect(ui.btnRename, &QPushButton::clicked, this, &OSHomework3::fsqRename);
    connect(ui.btnCloseFile, &QPushButton::clicked, this, &OSHomework3::fsqClose);
    connect(ui.btnNewFile, &QPushButton::clicked, this, &OSHomework3::fsqGotoNewFile);

    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(this->width(), this->height());

    ui.fileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.fileTable->setSelectionMode(QAbstractItemView::SingleSelection);

    fsqRefreshWindow();
}

void OSHomework3::fsqShowPath() {
    QString x = "";
    for (int i = 0; i < fb.path_route->size(); i++) {
        x = x + (*(fb.path_route))[i].name + '\\';
    }
    ui.txtPath->setPlainText(x);
}

void OSHomework3::fsqGetDevInf() {
    string x = fb.fbGetDevInfo();
    ui.txtDev->setPlainText(QString::fromStdString(x));
    QMessageBox::information(0,
        "获取设备信息", "信息获取完成！",
        QMessageBox::Ok | QMessageBox::Default,
        0, 0);
}

void OSHomework3::fsqGoParentFolder() {
    fb.fbGoParent();
    fsqShowPath();
    fsqRefreshWindow();
}

void OSHomework3::fsqUpdateInfo() {
    stringstream oss;
    fat32_bootSector* bs = (fat32_bootSector*)fb.vdisk;
    for (int i = 0; i < 11; i++) {
        oss << bs->info.bs_VolLab[i];
    }
    ui.lblVolLab->setText(QString::fromStdString(oss.str()));

    oss.clear();
    oss.str("");
    oss << bs->info.bpb_BytsPerSec * bs->info.bpb_TotSec32 / 1024 << " KBytes";
    ui.lblTotCap->setText(QString::fromStdString(oss.str()));

    oss.clear();
    oss.str("");
    oss << fb.fbGetAvaSpace() / 1024 << " KBytes";
    ui.lblUsedCap->setText(QString::fromStdString(oss.str()));
}

