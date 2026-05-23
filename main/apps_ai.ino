// ═══════════════════════════════════════════════════════════
//  apps_ai.ino - AI Chat
// ═══════════════════════════════════════════════════════════

String aiLines[4];
String aiInput   = "";
String aiHistory = "";
bool   aiWaiting = false;

void aiAdd(const String& line) {
  for (int i=0;i<3;i++) aiLines[i]=aiLines[i+1];
  aiLines[3]=line;
}

void aiSend(const String& msg) {
  if (!wifiReady)           { aiAdd("No WiFi!"); return; }
  if (cfg.aiIP.length()==0) { aiAdd("Set AI IP in Settings"); return; }
  if (cfg.aiModel.length()==0){ aiAdd("Set model in Settings"); return; }

  aiWaiting=true;
  aiAdd("You:"+msg.substring(0,12));
  lcdPrint(0,"AI Thinking...  ");
  rgbPurple();

  String url="http://"+cfg.aiIP+":"+String(cfg.aiPort)+"/v1/chat/completions";
  DynamicJsonDocument req(2048);
  req["model"]  = cfg.aiModel;
  req["stream"] = (cfg.aiDispMode==0);
  JsonArray messages = req.createNestedArray("messages");
  JsonObject sys=messages.createNestedObject();
  sys["role"]="system"; sys["content"]=cfg.aiSysPrompt;
  if (aiHistory.length()>0) {
    JsonObject hist=messages.createNestedObject();
    hist["role"]="assistant"; hist["content"]=aiHistory;
  }
  JsonObject usr=messages.createNestedObject();
  usr["role"]="user"; usr["content"]=msg;
  String body; serializeJson(req,body);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type","application/json");
  http.setTimeout(30000);
  String response="";

  if (cfg.aiDispMode==0) {
    int code=http.POST(body);
    if (code>0) {
      WiFiClient* stream=http.getStreamPtr();
      String token="";
      while(http.connected()||stream->available()) {
        if(stream->available()) {
          String line=stream->readStringUntil('\n');
          if(line.startsWith("data: ")&&line!="data: [DONE]") {
            String json=line.substring(6);
            DynamicJsonDocument chunk(512);
            if(!deserializeJson(chunk,json)) {
              String t=chunk["choices"][0]["delta"]["content"].as<String>();
              if(t!="null"&&t.length()>0) {
                token+=t; response+=t;
                aiAdd(token.substring(max(0,(int)token.length()-14)));
                drawAIChat();
              }
            }
          }
        }
      }
    } else aiAdd("HTTP err:"+String(code));
  } else {
    int code=http.POST(body);
    if(code==200) {
      String res=http.getString();
      DynamicJsonDocument resp(4096);
      if(!deserializeJson(resp,res))
        response=resp["choices"][0]["message"]["content"].as<String>();
    } else aiAdd("HTTP err:"+String(code));
  }

  http.end();
  if(response.length()>0) {
    aiHistory+="User:"+msg+"\nAI:"+response+"\n";
    if(aiHistory.length()>600) aiHistory=aiHistory.substring(aiHistory.length()-600);
    int pos=0;
    while(pos<(int)response.length()) {
      aiAdd(response.substring(pos,pos+14)); pos+=14;
    }
    okBeep();
    if(cfg.aiDispMode==2) { lcdPrint(0,"AI:"); lcdPrint(1,response.substring(0,16)); }
  }
  aiWaiting=false; rgbCyan();
}

void drawAIChat() {
  oledHeader("> AI CHAT");
  for(int i=0;i<4;i++) oledPrint(0,12+i*10,aiLines[i]);
  oled.drawFastHLine(0,54,OLED_W,WHITE);
  oledPrint(0,56,(aiWaiting?"...":">") + aiInput+"_");
  oled.display();
  if(!aiWaiting) {
    lcdPrint(0,"AI CHAT         ");
    lcdPrint(1,aiInput.length()>0?aiInput:"Type message... ");
  }
}

void handleAIKey(char key) {
  if(aiWaiting) return;
  if(key=='\n'||key=='\r') {
    if(aiInput.length()>0) { aiSend(aiInput); aiInput=""; }
  } else if(key==8||key==127) {
    if(aiInput.length()>0) aiInput.remove(aiInput.length()-1);
  } else if(key==27) {
    aiHistory=""; aiAdd("History cleared");
  } else if(aiInput.length()<60) aiInput+=key;
}
