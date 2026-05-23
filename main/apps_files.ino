// ═══════════════════════════════════════════════════════════
//  apps_files.ino - Full file browser with edit
// ═══════════════════════════════════════════════════════════

struct FileEntry { String name; bool isDir; size_t size; };
FileEntry fileEntries[20];
int   fileCount    = 0;
int   fileSelected = 0;
String filePath    = "/";
String fileStack[8]; // directory stack
int   fileStackTop = 0;

// File view/edit state
enum FileSubMode { FS_BROWSE, FS_VIEW, FS_EDIT, FS_CONFIRM_DELETE, FS_RENAME, FS_NEW };
FileSubMode fileSubMode = FS_BROWSE;
String fileViewContent  = "";
int    fileViewOffset   = 0;
String fileEditContent  = "";
int    fileEditCursor   = 0;
String fileRenameInput  = "";
String fileNewNameInput = "";
bool   fileNewIsDir     = false;

void fileLoadDir(const String& path) {
  fileCount=0; fileSelected=0;
  if (path!="/") {
    fileEntries[0].name=".."; fileEntries[0].isDir=true; fileEntries[0].size=0;
    fileCount=1;
  }
  File dir=SD.open(path.c_str());
  if (!dir) return;
  while(fileCount<20) {
    File e=dir.openNextFile(); if(!e) break;
    fileEntries[fileCount].name=String(e.name());
    fileEntries[fileCount].isDir=e.isDirectory();
    fileEntries[fileCount].size=e.isDirectory()?0:e.size();
    fileCount++; e.close();
  }
  dir.close();
}

void fileOpen(int idx) {
  if (idx<0||idx>=fileCount) return;
  String name=fileEntries[idx].name;
  if (name=="..") {
    if(fileStackTop>0) filePath=fileStack[--fileStackTop];
    else filePath="/";
    fileLoadDir(filePath); return;
  }
  String fullPath=(filePath=="/"?"/":filePath+"/")+name;
  if (fileEntries[idx].isDir) {
    if(fileStackTop<8) fileStack[fileStackTop++]=filePath;
    filePath=fullPath;
    fileLoadDir(filePath);
  } else {
    // Open file for viewing
    File f=SD.open(fullPath.c_str());
    if(f) {
      fileViewContent=f.readString();
      f.close();
      fileViewOffset=0;
      fileSubMode=FS_VIEW;
    }
  }
}

void fileSave(int idx) {
  String name=fileEntries[idx].name;
  String fullPath=(filePath=="/"?"/":filePath+"/")+name;
  File f=SD.open(fullPath.c_str(),FILE_WRITE);
  if(f) { f.print(fileEditContent); f.close(); okBeep(); }
  else errorBeep();
}

void fileDelete(int idx) {
  String name=fileEntries[idx].name;
  String fullPath=(filePath=="/"?"/":filePath+"/")+name;
  if(fileEntries[idx].isDir) SD.rmdir(fullPath.c_str());
  else SD.remove(fullPath.c_str());
  fileLoadDir(filePath);
  okBeep();
}

void fileRename(int idx, const String& newName) {
  // ESP32 SD has no rename — copy+delete
  String oldPath=(filePath=="/"?"/":filePath+"/")+fileEntries[idx].name;
  String newPath=(filePath=="/"?"/":filePath+"/")+newName;
  File src=SD.open(oldPath.c_str());
  File dst=SD.open(newPath.c_str(),FILE_WRITE);
  if(src&&dst) {
    while(src.available()) dst.write(src.read());
    src.close(); dst.close();
    SD.remove(oldPath.c_str());
    fileLoadDir(filePath); okBeep();
  } else errorBeep();
}

void fileCreateNew(const String& name, bool isDir) {
  String fullPath=(filePath=="/"?"/":filePath+"/")+name;
  if(isDir) SD.mkdir(fullPath.c_str());
  else { File f=SD.open(fullPath.c_str(),FILE_WRITE); if(f) f.close(); }
  fileLoadDir(filePath); okBeep();
}

void drawFileBrowse() {
  oledHeader(filePath.substring(0,14));
  if(!sdReady) { oledPrint(0,28,"SD not ready!"); oled.display(); return; }
  int start=(fileSelected/4)*4;
  for(int i=0;i<4&&(start+i)<fileCount;i++) {
    int idx=start+i;
    bool sel=(idx==fileSelected);
    String prefix=sel?">":" ";
    String icon=fileEntries[idx].isDir?"[D]":"[F]";
    String line=prefix+icon+fileEntries[idx].name;
    oledPrint(0,12+i*12,line.substring(0,16));
  }
  oledPrint(0,56,"ENT=open DEL=del N=new");
  oled.display();
  lcdPrint(0,filePath.substring(0,16));
  lcdPrint(1,fileCount>0?fileEntries[fileSelected].name:"Empty           ");
}

void drawFileView() {
  oledHeader("VIEW: "+fileEntries[fileSelected].name.substring(0,8));
  int charsPerLine=16;
  int linesPerPage=4;
  int offset=fileViewOffset*charsPerLine;
  for(int row=0;row<linesPerPage;row++) {
    int start=offset+row*charsPerLine;
    if(start>=(int)fileViewContent.length()) break;
    oledPrint(0,12+row*12,fileViewContent.substring(start,start+charsPerLine));
  }
  oledPrint(0,56,"W/S:scroll E=edit ESC=back");
  oled.display();
  lcdPrint(0,"VIEWING FILE    ");
  lcdPrint(1,fileEntries[fileSelected].name.substring(0,16));
}

void drawFileEdit() {
  oledHeader("EDIT: "+fileEntries[fileSelected].name.substring(0,8));
  int charsPerLine=16;
  int cursorLine=fileEditCursor/charsPerLine;
  int startLine=max(0,cursorLine-1);
  for(int row=0;row<3;row++) {
    int start=(startLine+row)*charsPerLine;
    if(start>(int)fileEditContent.length()) break;
    String line=fileEditContent.substring(start,start+charsPerLine);
    oledPrint(0,12+row*12,line);
  }
  oledPrint(0,54,"CTRL+S=save ESC=cancel");
  oled.display();
  lcdPrint(0,"EDITING FILE    ");
  lcdPrint(1,"CTRL+S to save  ");
}

void drawFileConfirmDelete() {
  oledHeader("DELETE?");
  oledPrint(0,20,fileEntries[fileSelected].name.substring(0,16));
  oledPrint(0,36,"Y=yes  N=no");
  oled.display();
  lcdPrint(0,"DELETE FILE?    ");
  lcdPrint(1,"Y=yes  N=cancel ");
}

void drawFileRename() {
  oledHeader("RENAME TO:");
  oledPrint(0,24,fileRenameInput+"_");
  oledPrint(0,48,"ENTER=ok ESC=cancel");
  oled.display();
  lcdPrint(0,"RENAME FILE     ");
  lcdPrint(1,fileRenameInput.substring(0,16));
}

void drawFileNew() {
  oledHeader(fileNewIsDir?"NEW FOLDER":"NEW FILE");
  oledPrint(0,24,fileNewNameInput+"_");
  oledPrint(0,36,"TAB=toggle dir/file");
  oledPrint(0,48,"ENTER=ok ESC=cancel");
  oled.display();
  lcdPrint(0,fileNewIsDir?"NEW FOLDER      ":"NEW FILE        ");
  lcdPrint(1,fileNewNameInput.substring(0,16));
}

void drawFiles() {
  switch(fileSubMode) {
    case FS_BROWSE:         drawFileBrowse(); break;
    case FS_VIEW:           drawFileView();   break;
    case FS_EDIT:           drawFileEdit();   break;
    case FS_CONFIRM_DELETE: drawFileConfirmDelete(); break;
    case FS_RENAME:         drawFileRename(); break;
    case FS_NEW:            drawFileNew();    break;
  }
}

void handleFilesKey(char key) {
  switch(fileSubMode) {
    case FS_BROWSE:
      if(key=='w'||key=='W') { if(fileSelected>0) fileSelected--; }
      else if(key=='s'||key=='S') { if(fileSelected<fileCount-1) fileSelected++; }
      else if(key=='\n'||key=='\r') { fileOpen(fileSelected); }
      else if(key==127||key==8) { fileSubMode=FS_CONFIRM_DELETE; }
      else if(key=='r'||key=='R') { fileRenameInput=""; fileSubMode=FS_RENAME; }
      else if(key=='n'||key=='N') { fileNewNameInput=""; fileNewIsDir=false; fileSubMode=FS_NEW; }
      else if(key==27) { currentMode=MODE_CLOCK; }
      break;

    case FS_VIEW:
      if(key=='w'||key=='W') { if(fileViewOffset>0) fileViewOffset--; }
      else if(key=='s'||key=='S') fileViewOffset++;
      else if(key=='e'||key=='E') {
        fileEditContent=fileViewContent;
        fileEditCursor=0;
        fileSubMode=FS_EDIT;
      }
      else if(key==27) fileSubMode=FS_BROWSE;
      break;

    case FS_EDIT:
      if(key==19) { fileSave(fileSelected); fileSubMode=FS_VIEW; } // CTRL+S
      else if(key==27) fileSubMode=FS_VIEW;
      else if((key==8||key==127)&&fileEditCursor>0) {
        fileEditContent.remove(--fileEditCursor,1);
      } else if(key=='\n'||key=='\r') {
        fileEditContent=fileEditContent.substring(0,fileEditCursor)+"\n"+
                        fileEditContent.substring(fileEditCursor);
        fileEditCursor++;
      } else {
        fileEditContent=fileEditContent.substring(0,fileEditCursor)+
                        String(key)+fileEditContent.substring(fileEditCursor);
        fileEditCursor++;
      }
      break;

    case FS_CONFIRM_DELETE:
      if(key=='y'||key=='Y') { fileDelete(fileSelected); fileSubMode=FS_BROWSE; }
      else fileSubMode=FS_BROWSE;
      break;

    case FS_RENAME:
      if(key=='\n'||key=='\r') {
        if(fileRenameInput.length()>0) fileRename(fileSelected,fileRenameInput);
        fileSubMode=FS_BROWSE;
      } else if(key==27) fileSubMode=FS_BROWSE;
      else if((key==8||key==127)&&fileRenameInput.length()>0)
        fileRenameInput.remove(fileRenameInput.length()-1);
      else if(fileRenameInput.length()<20) fileRenameInput+=key;
      break;

    case FS_NEW:
      if(key=='\t') fileNewIsDir=!fileNewIsDir;
      else if(key=='\n'||key=='\r') {
        if(fileNewNameInput.length()>0) fileCreateNew(fileNewNameInput,fileNewIsDir);
        fileSubMode=FS_BROWSE;
      } else if(key==27) fileSubMode=FS_BROWSE;
      else if((key==8||key==127)&&fileNewNameInput.length()>0)
        fileNewNameInput.remove(fileNewNameInput.length()-1);
      else if(fileNewNameInput.length()<20) fileNewNameInput+=key;
      break;
  }
}
