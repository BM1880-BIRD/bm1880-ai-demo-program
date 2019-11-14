# bm mpp system
# BM MPP System

## 目錄

1. [簡介](#簡介)
2. [系統架構](#系統架構)
3. [如何編譯](#如何編譯)
4. [啟動](#啟動)

## 簡介
抓取Hisi3516Av100的jpeg圖像，並透過libusb輸入至nnm做yolo, 從usb讀取結果並將圖像串流導出, 可輸出至網頁後台, 也可以儲存起來

## 系統架構

我們設計一個物件, bm_mpp_system, 這個物件本身帶有一些海思mpp, 系統自身會帶一個Queue, 主要就是用來存放frame的data, queue的大小可以在一開始初始化去做配置, 建議不要過大, 否則發現影像串流會有些延遲, 這邊設計是當queue滿了時候就會丟掉frame直到有空間為止

系統在初始化之後會啟動一個線程, 負責將當前攝像圖像不斷的輸入到queue裡, 直到queue滿了, 我們可以通過get()這個函式來讀取queue的資料

可以參考demo.cpp裡面, 可以看到一旦get取到圖像後, 我們這邊便會將圖送至nnm執行yolo, 並且在收取結果, 整個過程淺顯易懂

![](https://i.imgur.com/eV2bsol.png)


## 如何編譯
請先確定要有hisi3516Av100 ARM的交叉編譯器, 編譯CMakeLists.txt的預設路徑放在
```
 /opt/hisi-linux/x86-arm/arm-hisiv300-linux/bin/
 ```
 確定後, 新建一個新的資料夾為build後開始編譯
 ```
 mkdir build
 cd build
 cmake ..
 make
 ```

 這邊相關的so檔, 包含海思的sdk, 都會放在prebuilt資料夾底下,

 編譯完後會看到資料夾底下有一個bm_mpp_system,
 麻煩跟prebuilt的library 一起放到arm Host上

## 啟動
./bm_mpp_system 0/1 [web]

這邊 0 表示不進行yolo, 單純串流影像, 1 表示圖像要先經過nnm執行yolo
後方的web表示輸出的後台網址

```
ex:
 ./bm_mpp_system 1 http://10.34.36.152:5000/api/v1/streams/front_in
```

這邊後面的web是可以將影像串流傳到webserver, 透過網路頁面去顯示結果輸出

## Web 後台輸出

至 http://gitlab-ai.bitmain.vip/sys_app/attendance_system

抓下專案後, 在專案下執行
```
sudo docker-compose up --build
```
後台就會起來了, 相對應的docker-compose 必須先安裝好

啟動之後至
```
http://{your_web_server_ip}:5000/streams
```
點選front_in, 在執行bm_mpp_system

```
./bm_mpp_system 1 http://{your_web_server_ip}:5000/api/v1/streams/front_in
```

就會看到影像輸出到web後台了
