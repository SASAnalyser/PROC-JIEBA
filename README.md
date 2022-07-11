# PROC-JIEBA
Make a SAS procuedure to call Jieba. Jieba is a Chinese word segmentation library, refer to: https://github.com/yanyiwu/cjieba

Build for linux:

1. Install SAS 9.4 Foundation (include unicode support) to /opt/SASHome

2. Unzip <SASRepo>/products/toolkit__94xxx__lax__en__sp0__1/en_misc.zip/toolkit.zip to /opt/SASToolkit

3. Download https://github.com/yanyiwu/cjieba, unzip it then copy it to /opt/SASToolkit, the directory structure looks like:
/opt/SASToolkit
...c
...cjieba
...global

4. Donwload cjieba.c, jieba.grm, prcjieba.make, from this repo, copy cjieba.c to /opt/SASToolkit/c/src, copy jieba.grm to /opt/SASToolkit/global/grm,
copy prcjieba.make to /opt/SAStoolkit/c/cntl

5. Edit /opt/SAStoolkit/c/cntl/allmakec, let it just contains:
PATH=../../global/load:$PATH
export PATH
make -f prcjieba.make

6. Edit /opt/SAStoolkit/c/cntl/tlkthost.make, change UWCOPTS to:
UWCOPTS         = -c -I$(MACLIB) -I/opt/SAStoolkit/cjieba -fPIC -m64

change UWLINKOPTS to:
UWLINKOPTS      = -shared -Bsymbolic -e mcn_main \
                  -lc -lm -lpthread -L/opt/SAStoolkit/cjieba -ljieba
                  
change SASCMD to (suppose /opt/SASHome/SASFoundation/9.4/sas link to /opt/SASHome/SASFoundation/9.4/bin/sas_u8):
SASCMD = /opt/SASHome/SASFoundation/9.4/sas -path ../../global/load

7. Make cjieba:
cd /opt/SASToolkit/cjieba
make demo

8. Make PROC JIEBA:
cd /opt/SAStoolkit/c/cntl
./allmakec

9. Copy /opt/SAStoolkit/c/load/jieba to folder /opt/SASHome/SASFoundation/9.4/sasexe

10. Run PROC JIEBA:
PROC JIEBA;
dictpath "/opt/SAStoolkit/cjieba/dict";
instr "我是蓝翔技工拖拉机学院手扶拖拉机专业的";
RUN;

Output:
 14? PROC JIEBA;
dictpath "/opt/SAStoolkit/cjieba/dict";
instr "我是蓝翔技工拖拉机学院手扶拖拉机专业的";
 17? RUN;

我
是
蓝翔
技工
拖拉机
学院
手扶拖拉机
专业
的
NOTE: PROCEDURE JIEBA used (Total process time):
      real time           6.86 seconds
      cpu time            1.50 seconds
      




