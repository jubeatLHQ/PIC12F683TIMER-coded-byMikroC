#include<stdint.h>
#include<string.h>
#include<stdlib.h>

sbit Soft_I2C_Scl_Direction at GP4_bit;
sbit Soft_I2C_Sda_Direction at GP5_bit;
sbit Soft_I2C_Scl at GP4_bit;
sbit Soft_I2C_Sda at GP5_bit;

int lcdIsOn = 0;///0:OFF 1:ON
int mode = 1;///0:clock 1:setting 2:stopwatch
int mode1At = 0;///0~2(秒、分、時)

void lcdData(unsigned char str){
     Soft_I2C_Start();
     Soft_I2C_Write(0x7C);
     Soft_I2C_Write(0xC0);
     Soft_I2C_Write(str);
     Soft_I2C_Stop();
}

///lcdCmd(0xC0);2行目の先頭に移動
///lcdCmd(0x80);1行目の先頭に移動
void lcdCmd(unsigned char cmd){
     Soft_I2C_Start();
     Soft_I2C_Write(0x7C);
     Soft_I2C_Write(0x80);
     Soft_I2C_Write(cmd);
     Soft_I2C_Stop();
}

void lcd_str_byCode(char cmd){
     Soft_I2C_Start();
     Soft_I2C_Write(0x7C);
     Soft_I2C_Write(0xC0);
     Soft_I2C_Write(cmd);
     Soft_I2C_Stop();
}
void lcd_str(int skip,char* ptr){
     if(skip==1){
          *ptr++;
     }
     while(*ptr != 0){
          lcdData(*ptr++);
     }
}
void singleHeight(void){
     lcdCmd(0x38);
}///N=1/DH=0
void doubleHeight(void){
     lcdCmd(0x34);
}///N=0/DH=1
void lcdClear(void){
     lcdCmd(0x01);
}

int h = 0;
int m = 0;
int s = 0;
void clockReload(void){
     if(lcdIsOn==1){
          char hour[4];
          char minute[4];
          char second[4];
          lcdCmd(0x80);///カーソルを先頭に移動
          ByteToStr(h,hour);
          if(h<10){
               hour[1]='0';
          }
          lcd_str(1,hour);
          ByteToStr(m,minute);
          if(m<10){
               minute[1]='0';
          }
          minute[0]=':';
          lcd_str(0,minute);
          ByteToStr(s,second);
          if(s<10){
               second[1]='0';
          }
          second[0]=':';
          lcd_str(0,second);
          
          if(mode==1){///lcd_str_byCode(0x5E);0x5E 上向き矢印
               lcdCmd(0xC0);///カーソルを２行目の先頭に移動
               if(mode1At==0){
                    lcd_str(0,"       ");
                    lcd_str_byCode(0x5E);
               }else if(mode1At==1){
                    lcd_str(0,"    ");
                    lcd_str_byCode(0x5E);
                    lcd_str(0,"   ");
               }else if(mode1At==2){
                    lcd_str(0," ");
                    lcd_str_byCode(0x5E);
                    lcd_str(0,"      ");
               }
          }
     }
}
int swMode = 0;///0:stop 1:starting
int sw_min = 0;
int sw_sec = 0;
int sw_down = 0;
void stopWatchReload(void){
     char min[4];
     char sec[4];
     char down[4];
     if(swMode==1){
          if(sw_down==99){
               if(sw_sec==59){
                    if(sw_min==59){
                    }else{
                         sw_down=0;
                         sw_sec=0;
                         sw_min++;
                    }
               }else{
                    sw_down=0;
                    sw_sec++;
               }
          }else{
               sw_down++;
          }
     }
     lcdCmd(0x80);///カーソルを先頭に移動
     ByteToStr(sw_min,min);
     if(sw_min<10){
          min[1]='0';
     }
     lcd_str(1,min);
     ByteToStr(sw_sec,sec);
     if(sw_sec<10){
          sec[1]='0';
     }
     sec[0]=':';
     lcd_str(0,sec);
     ByteToStr(sw_down,down);
     if(sw_down<10){
          down[1]='0';
     }
     down[0]='.';
     lcd_str(0,down);
}
void lcdOff(void){
     lcdCmd(0x08);
     lcdIsOn = 0;
}
void lcdOn(void){
     lcdCmd(0x0C);
     lcdIsOn = 1;
     if(mode==0){
          clockReload();
     }
}
void lcdOnEnable(void){
     Soft_I2C_Init();
     Delay_ms(40);
     lcdCmd(0x38);
     lcdCmd(0x39);///function IS=1
     lcdCmd(0x14);
     lcdCmd(0x70);///contrast
     lcdCmd(0x56);
     lcdCmd(0x6C);
     Delay_ms(300);
     lcdCmd(0x38);///function IS=0
     lcdOn();
     lcdClear();
}
///point 0~2(秒、分、時)
void countUp(int point,int up){
     if(point==0){
          s++;
     }else if(point==1){
          m++;
     }else if(point==2){
          h++;
     }
     if(s>59){
          s-=60;
          if(up==1){
               m++;
          }
     }
     if(m>59){
          m-=60;
          if(up==1){
               h++;
          }
     }
     if(h>23){
          h-=24;
     }
}
///if(Button(&GPIO,0,0,0)) GP0のボタンがONのときtrue
///if(Button(&GPIO,1,0,0)) GP1のボタンがONのときtrue
///Delay_ms(500) 0.5秒待機
int count = 0;
int last = 0;///0~1000tick;
int button1 = 0;///0~30tick
int button2 = 0;///0~30tick
void main() {
     OSCCON.IRCF2=1;//8Hz
     OSCCON.IRCF1=1;//...
     OSCCON.IRCF0=1;//...
     ANSEL = 0b00000000;///GP0～5をデジタル利用に設定
     GPIO = 0b00000000;///GPIOの中身を空に
     TRISIO = 0b00001111;///GP0～3を入力、4,5を出力
     CMCON0 = 0b00000111;///コンパレータを使わない
     /*OPTION_REG = 0b00000111;///プリスケーラ1:256
     INTCON = 0b10100100;///割り込み設定*/
     OPTION_REG.NOT_GPPU= 0;///ピンのプルアップ使用(スイッチ入力用)
     WPU = 0b00000011;///GP0,1をプルアップ
     lcdOnEnable();///lcd起動
     lcdClear();
     clockReload();
     while(1) {///1tick = 10ms(100回で１秒)
          if(count==100&&mode==0){
               countUp(0,1);
               clockReload();
               count=0;
          }else if(mode==2){
               if(count==100){
                    countUp(0,1);
                    count=0;
               }
               stopWatchReload();
               ///stopwatch
          }else if(mode==1){
               count=0;
          }
          //////////////////////////button
          if(Button(&GPIO,0,0,0)){
               button1++;
          }else{
               if(300>button1&&button1>5){
                    if(mode==0){
                         mode=2;
                         swMode=0;
                         sw_down=0;
                         sw_sec=0;
                         sw_min=0;
                         stopWatchReload();
                         ///stopwatch
                    }else if(mode==1){
                         countUp(mode1At,0);
                         clockReload();
                    }else if(mode==2){
                         mode=0;
                         swMode=0;
                         clockReload();
                    }
                    button1 = 0;
                    button2 = 0;
                    if(lcdIsOn==0){
                         lcdOn();
                    }
               }else if(button1>=300){
                    if(mode==0){
                         mode=1;
                         singleHeight();
                         button1 = 0;
                         button2 = 0;
                         if(lcdIsOn==0){
                              lcdOn();
                         }
                         mode1At=0;
                         clockReload();
                    }
               }
               button1 = 0;
          }
          if(Button(&GPIO,1,0,0)){
               button2++;
          }else{
               if(button2>5){
                    if(mode==0){
                         last = 0;
                    }else if(mode==1){
                         mode1At++;
                         if(mode1At==3){
                              mode1At=0;
                              mode=0;
                              doubleHeight();
                         }
                         clockReload();
                    }else if(mode==2){
                         swMode++;
                         if(swMode==2){
                              swMode=0;
                         }else{
                              sw_down=0;
                              sw_sec=0;
                              sw_min=0;
                         }
                         ///stopwatch
                    }
                    button1 = 0;
                    button2 = 0;
                    if(lcdIsOn==0){
                         lcdOn();
                    }
               }
               button2 = 0;
          }
          //////////////////////////////
          if(last==1000){
               lcdOff();
               last = 0;
          }
          Delay_ms(10);
          count++;
          if(lcdIsOn==1&&mode==0){
               last++;
          }else{
               last=0;
          }
     }
}

/*void interrupt(){
     if(INTCON.T0IF==1){
          INTCON.T0IF=0;
     }
}*/

/*
時計　ボタン１単押し＝ストップウォッチ移動
　　　ボタン１長押し＝時間設定画面(3秒ぐらい？)
　　　ボタン２単押し＝明るさ延長

設定　ボタン１単押し＝カウントアップ
　　　ボタン２単押し＝カーソル移動(秒、分、時の次は時計に移動)

SW　ボタン１単押し＝時計移動
　　ボタン２単押し＝スタート、ストップ
*/
