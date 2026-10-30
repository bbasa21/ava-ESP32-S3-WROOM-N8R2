#include "Ava_OTA_Display.h"
#include <U8g2lib.h>
#include "Ava_DisplayAdapter.h"
#include "Ava_OLED.h"
extern U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI avaDisplay;
static volatile bool active=false; static volatile int progress=0; static volatile AvaOTAUIStatus state=AVA_OTA_UI_IDLE;
void avaOTAUIBegin(){progress=0;state=AVA_OTA_UI_INSTALLING;active=true;}
void avaOTAUISetProgress(int p){progress=constrain(p,0,100);}
void avaOTAUISetStatus(AvaOTAUIStatus s){state=s;}
bool avaOTAUIIsActive(){return active;}
void avaOTAUIEnd(){active=false;state=AVA_OTA_UI_IDLE;progress=0;}
void avaOTAUIUpdate(){
 if(!active||!AvaDisplayAdapter::isReady()||!avaOLEDCanRender(AVA_OLED_OWNER_OTA))return;
 AvaDisplayAdapter::beginFrame();avaDisplay.setDrawColor(1);avaDisplay.setFont(u8g2_font_7x14B_tf);
 const char* title="AVA SYSTEM";avaDisplay.drawStr((128-avaDisplay.getStrWidth(title))/2,13,title);
 avaDisplay.setFont(u8g2_font_6x10_tf);const char* label="PREPARING";
 switch(state){case AVA_OTA_UI_UPDATING:label="DOWNLOADING";break;case AVA_OTA_UI_VERIFYING:label="VERIFYING IMAGE";break;case AVA_OTA_UI_INSTALLING:label="INSTALLING";break;case AVA_OTA_UI_RESTARTING:label="RESTARTING";break;case AVA_OTA_UI_ERROR:label="UPDATE FAILED";break;default:break;}
 avaDisplay.drawStr((128-avaDisplay.getStrWidth(label))/2,28,label);avaDisplay.drawFrame(10,36,108,10);
 int fill=104*progress/100;if(fill>0)avaDisplay.drawBox(12,38,fill,6);char pct[8];snprintf(pct,sizeof(pct),"%d%%",(int)progress);
 avaDisplay.setFont(u8g2_font_7x14B_tf);avaDisplay.drawStr((128-avaDisplay.getStrWidth(pct))/2,60,pct);AvaDisplayAdapter::endFrame();
}