#include <WiFiClient.h>
#include <ArduinoJson.h>
// ═══════════════════════════════════════════════════════════
//  apps_transfer.ino - WiFi file transfer on port 11211
//  JSON protocol: upload/download/msg/sync/ls/mkdir/rm/rmdir
// ═══════════════════════════════════════════════════════════

String transferLog[4];
int    transferSelected = 0;
bool   transferServerOn = false;
String transferMessages[8];
int    transferMsgCount = 0;
enum TransferSubMode { TR_STATUS, TR_MESSAGES, TR_BROWSE };
TransferSubMode transferSubMode = TR_STATUS;

void transferLogAdd(const String& line) {
  for(int i=0;i<3;i++) transferLog[i]=transferLog[i+1];
  transferLog[3]=line;
}

// Base64 decode helper
String base64Decode(const String& encoded) {
  const String chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String decoded="";
  int val=0, bits=-8;
  for(int i=0;i<(int)encoded.length();i++) {
    int c=chars.indexOf(encoded[i]);
    if(c<0) continue;
    val=(val<<6)+c; bits+=6;
    if(bits>=0) { decoded+=(char)((val>>bits)&0xFF); bits-=8; }
  }
  return decoded;
}

// Recursive directory delete
void sdRmDir(const String& path) {
  File dir=SD.open(path.c_str());
  if(!dir) return;
  while(true) {
    File f=dir.openNextFile(); if(!f) break;
    String fp=path+"/"+String(f.name());
    if(f.isDirectory()) { f.close(); sdRmDir(fp); SD.rmdir(fp.c_str()); }
    else { f.close(); SD.remove(fp.c_str()); }
  }
  dir.close();
  SD.rmdir(path.c_str());
}

// List directory recursively into JSON array
void sdListDir(JsonArray& arr, const String& path, int depth=0) {
  File dir=SD.open(path.c_str()); if(!dir) return;
  while(true) {
    File f=dir.openNextFile(); if(!f) break;
    JsonObject obj=arr.createNestedObject();
    obj["name"]=String(f.name());
    obj["path"]=(path=="/"?"/":path+"/")+String(f.name());
    obj["isDir"]=f.isDirectory();
    obj["size"]=(int)f.size();
    if(f.isDirectory()&&depth<3) {
      JsonArray sub=obj.createNestedArray("children");
      f.close();
      sdListDir(sub,(path=="/"?"/":path+"/")+obj["name"].as<String>(),depth+1);
    } else f.close();
  }
  dir.close();
}

void handleTransferClient(WiFiClient& client) {
  String raw=client.readStringUntil('\n');
  raw.trim();
  if(raw.length()==0) { client.stop(); return; }

  DynamicJsonDocument req(8192);
  DynamicJsonDocument resp(8192);

  if(deserializeJson(req,raw)) {
    resp["ok"]=false; resp["error"]="Invalid JSON";
    String out; serializeJson(resp,out);
    client.println(out); client.stop(); return;
  }

  String cmd=req["cmd"].as<String>();
  transferLogAdd("CMD:"+cmd);

  if(cmd=="ls") {
    String path=req.containsKey("path")?req["path"].as<String>():"/";
    resp["ok"]=true;
    JsonArray files=resp.createNestedArray("files");
    sdListDir(files,path);

  } else if(cmd=="upload") {
    String name=req["name"].as<String>();
    String data=req["data"].as<String>();
    String path=req.containsKey("path")?req["path"].as<String>()+"/"+name:"/"+name;
    String decoded=base64Decode(data);
    File f=SD.open(path.c_str(),FILE_WRITE);
    if(f) { f.print(decoded); f.close(); resp["ok"]=true; resp["size"]=decoded.length(); }
    else  { resp["ok"]=false; resp["error"]="Write failed"; }
    transferLogAdd("UP:"+name);

  } else if(cmd=="download") {
    String path=req["path"].as<String>();
    File f=SD.open(path.c_str());
    if(f) {
      // Base64 encode
      String content=f.readString(); f.close();
      const String chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      String encoded="";
      int i=0;
      while(i<(int)content.length()) {
        uint32_t b=((uint8_t)content[i])<<16;
        if(i+1<(int)content.length()) b|=((uint8_t)content[i+1])<<8;
        if(i+2<(int)content.length()) b|=((uint8_t)content[i+2]);
        encoded+=chars[(b>>18)&63];
        encoded+=chars[(b>>12)&63];
        encoded+=(i+1<(int)content.length())?chars[(b>>6)&63]:'=';
        encoded+=(i+2<(int)content.length())?chars[b&63]:'=';
        i+=3;
      }
      resp["ok"]=true; resp["data"]=encoded;
      resp["name"]=path.substring(path.lastIndexOf('/')+1);
    } else { resp["ok"]=false; resp["error"]="File not found"; }
    transferLogAdd("DL:"+path);

  } else if(cmd=="msg") {
    String text=req["text"].as<String>();
    if(transferMsgCount<8) transferMessages[transferMsgCount++]=text;
    else {
      for(int i=0;i<7;i++) transferMessages[i]=transferMessages[i+1];
      transferMessages[7]=text;
    }
    resp["ok"]=true; resp["received"]=text;
    transferLogAdd("MSG:"+text.substring(0,10));

  } else if(cmd=="mkdir") {
    String path=req["path"].as<String>();
    SD.mkdir(path.c_str());
    resp["ok"]=true;
    transferLogAdd("MKDIR:"+path);

  } else if(cmd=="rm") {
    String path=req["path"].as<String>();
    bool ok=SD.remove(path.c_str());
    resp["ok"]=ok;
    transferLogAdd("RM:"+path);

  } else if(cmd=="rmdir") {
    String path=req["path"].as<String>();
    sdRmDir(path);
    resp["ok"]=true;
    transferLogAdd("RMDIR:"+path);

  } else if(cmd=="sync") {
    // Sync: receive array of {name,path,data} objects
    JsonArray files=req["files"].as<JsonArray>();
    int synced=0;
    for(JsonObject f:files) {
      String path=f["path"].as<String>();
      String data=base64Decode(f["data"].as<String>());
      // Create parent dirs if needed
      int slash=path.lastIndexOf('/');
      if(slash>0) SD.mkdir(path.substring(0,slash).c_str());
      File sf=SD.open(path.c_str(),FILE_WRITE);
      if(sf) { sf.print(data); sf.close(); synced++; }
    }
    resp["ok"]=true; resp["synced"]=synced;
    transferLogAdd("SYNC:"+String(synced)+" files");

  } else if(cmd=="rename") {
    // Copy + delete since no SD rename
    String from=req["from"].as<String>();
    String to=req["to"].as<String>();
    File src=SD.open(from.c_str());
    File dst=SD.open(to.c_str(),FILE_WRITE);
    if(src&&dst) {
      while(src.available()) dst.write(src.read());
      src.close(); dst.close(); SD.remove(from.c_str());
      resp["ok"]=true;
    } else { resp["ok"]=false; resp["error"]="Rename failed"; }
    transferLogAdd("REN:"+from);

  } else if(cmd=="ping") {
    resp["ok"]=true; resp["pong"]=true;
    resp["ip"]=WiFi.localIP().toString();
    resp["free"]=ESP.getFreeHeap();

  } else {
    resp["ok"]=false; resp["error"]="Unknown command: "+cmd;
  }

  String out; serializeJson(resp,out);
  client.println(out);
  client.stop();
  okBeep();
}

void transferLoop() {
  if(!transferServerOn||!wifiReady) return;
  WiFiClient client=transferServer.available();
  if(client) {
    transferLogAdd("Client connected");
    handleTransferClient(client);
  }
}

void drawTransferStatus() {
  oledHeader("> WIFI TRANSFER");
  oledPrint(0,12,wifiReady?"WiFi: "+WiFi.localIP().toString():"WiFi: Not connected");
  oledPrint(0,24,transferServerOn?"Server: ON :"+String(TRANSFER_PORT):"Server: OFF");
  for(int i=0;i<3;i++) oledPrint(0,34+i*10,transferLog[i+1]);
  oledPrint(0,56,"S=toggle M=msgs ESC=bk");
  oled.display();
  lcdPrint(0,transferServerOn?"TRANSFER ON     ":"TRANSFER OFF    ");
  lcdPrint(1,wifiReady?WiFi.localIP().toString():"No WiFi         ");
}

void drawTransferMessages() {
  oledHeader("MESSAGES("+String(transferMsgCount)+")");
  int start=max(0,transferMsgCount-4);
  for(int i=0;i<4&&(start+i)<transferMsgCount;i++) {
    oledPrint(0,12+i*12,transferMessages[start+i].substring(0,16));
  }
  oledPrint(0,56,"ESC=back C=clear");
  oled.display();
  lcdPrint(0,"MESSAGES        ");
  lcdPrint(1,transferMsgCount>0?transferMessages[transferMsgCount-1].substring(0,16):"No messages     ");
}

void drawTransfer() {
  switch(transferSubMode) {
    case TR_STATUS:   drawTransferStatus();   break;
    case TR_MESSAGES: drawTransferMessages(); break;
    default: break;
  }
}

void handleTransferKey(char key) {
  switch(transferSubMode) {
    case TR_STATUS:
      if(key=='s'||key=='S') {
        transferServerOn=!transferServerOn;
        if(transferServerOn&&wifiReady) { transferServer.begin(); transferLogAdd("Server started"); }
        else { transferServer.end(); transferLogAdd("Server stopped"); }
      }
      else if(key=='m'||key=='M') transferSubMode=TR_MESSAGES;
      else if(key==27) currentMode=MODE_CLOCK;
      break;
    case TR_MESSAGES:
      if(key=='c'||key=='C') { transferMsgCount=0; }
      else if(key==27) transferSubMode=TR_STATUS;
      break;
    default: break;
  }
}
