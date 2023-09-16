# Operating-System

使用語言 : C 

複雜度 : *****

## 開發背景

在作業系統這堂課，實作三個項目：Shell、Multi-threaded and kernel module programming與Scheduler Simulator，過程中學到了許多底層系統程式的寫法，也對作業系統的運作有了初步的瞭解。

### Shell
![Alt text](<Screenshot from 2023-09-17 00-08-34.png>)
#### Shell功能描述
![image](https://user-images.githubusercontent.com/90430653/221482731-92cd492a-0948-4e7a-9d91-e2a2bcece4f8.png)

#### pipeline與dup關係圖
![image](https://user-images.githubusercontent.com/90430653/221482824-d1df599c-c4d2-4c65-a70c-9f70df878141.png)

### Multi-threaded and kernel module programming
#### Multi-threaded and kernel module programming功能描述
![image](https://user-images.githubusercontent.com/90430653/221483226-828b39db-6ccb-4e72-b2e4-fc2a6f5e3b2c.png)

#### Multi-threaded & kernel module programming流程圖
![image](https://user-images.githubusercontent.com/90430653/221483029-a5699aa1-f677-4f78-b558-033f8f33812e.png)

#### Report
Q : You have to explain how you dispatch works to the worker threads.
A : 分三部分解釋
(一) : 分配方法 : by element dispatch
    我將最後result的矩陣的元素個數平均分給每個thread，第一個thread會多分到餘數的元素個數。其他就是所有的元素的個數直接除以thread的數量。
    至於為什麼要用元素個數去分是因為測資3與測資4的result的row 或column有可能為1。所以我才不選擇將row的數量或column的數量去分，因為可能還要試試調整。
分給每個thread的元素數量，由displacement 與 counts去計算每個thread要負責算那區塊的元素，而那些區塊的元素起始位址在哪? 我矩陣1、2與3都是用一維陣列做儲存(其實無論1維還是2維方法都一樣)。
(圖 : displacement 與 counts的意義)
 
( 二 ) : 矩陣是哪列哪行做內積
 
紅色17 是由這第一個矩陣的第3(從0開始)列 內積 第2(從0開始)行得到的。
要如何知道其實由 17在result矩陣的位置(列與行)得到， 17在 result矩陣的一維陣列的index 維 17 。 
17 / col2(5) (result矩陣的列數量) = 3 (=第3列)。
17%col2(5)(result矩陣的行數量) = 2(=第2 行) 。
因此知道17在result的第三列第二行。那依照矩陣乘法就是由矩陣1的第3列內積矩陣2的第2行 得到。

(三) : 內積算法。
 
內積總共要加總多少個元素相成的數量?  A : 矩陣1的col的數量 或 矩陣2的row的數量。
至於要內積哪行哪列 
```
11/  (3 = result矩陣的行數) = 3 (第3列)
11%  (3 = result 矩陣的行數 ) = 2(第2行)
所以就是矩陣1的第3行與矩陣2的第2行內積。
For(x  in 矩陣1的col的數量){
矩陣result的第11個元素(index from 0)
元素( 矩陣1的 第3列 * 矩陣1的col的數量 + x ) *
元素( 矩陣2的 第x列 * 矩陣2的col的數量 + 2) 
}
```
Code 
 ![image](https://github.com/weihsinyeh/Operating-System/assets/90430653/7adc938c-4bff-470a-b095-ea3ccf6e5c08)
 
Q : You are given four test cases. For each test case, you have to plot the matrix multiplication execution time with the following worker thread numbers. Worker thread number: 1,2,3,4,8,16,24,32. 

You have to summarize the four charts you plot. ( 詳見附錄 )
![image](https://github.com/weihsinyeh/Operating-System/assets/90430653/7b5a4e6e-d2c8-4408-b4f3-414437096f45)

![image](https://github.com/weihsinyeh/Operating-System/assets/90430653/38e5d67d-b4f9-48f2-8a3c-a10037055168)

![image](https://github.com/weihsinyeh/Operating-System/assets/90430653/0725fdab-1734-4d8f-9481-51e53f2e13d3)

![image](https://github.com/weihsinyeh/Operating-System/assets/90430653/44497daf-9399-48f0-947c-ac2e9f086ebb)

1. What happen if the number of threads is less than the number of cores. Why ? 
A : 
   core的數量小於thread的數量時，隨著thread增加，elapsed time減少的幅度的變化量比較大。在test1這個減少的幅度還是線性減少的。
因為矩陣計算屬於CPU bound的task，透過multi thread 主要是提升CPU 的使用率。對於虛擬機的4core來說，其實用4個thread 就可達到很好的效果讓一個thread 使用1個core。所以我們也看到4個test執行時間最短的都大概為4個thread。

2. What happen if the number of threads is greater than the number of cores. Why ? 
A:  core的數量大於thread的數量時，隨著thread的數量增加，elapsed time 並沒有像thread的數量小於number of cores時有大幅度下降，反而趨近於平緩，甚至有些還一點點增加了。因為當thread的數量超過core時，每個thread在執行部分時間就要將cpu 的control交給其他thread。而這個交換稱為context switch。Context switch is not free。必須要花時間儲存CPU的狀態，包刮暫存器的值得保存與更新新的暫存器的值，Program counter的切換...等。這些都需要花時間。這樣的overhead可能 將 多平行處理帶來的效益降低，甚至超過，所以才有上述的現象發生。


3.  Anything else you observe ?
觀察四筆測資元素相對於cache access的排列方式，比較4筆測資所花的時間，測資1因為處理數量(2048 * 2048 )遠小於 測資2的處理數量(4096 * 4096 。所以用時比較少。而測資3(1*4096) * (4096*4096)與測資4(4096*4096) *(4096*1 )。

測資3示意 :
![image](https://github.com/weihsinyeh/Operating-System/assets/90430653/e4afd4bb-ee49-4aad-b76d-09864e906e25)
測資4示意 :
![image](https://github.com/weihsinyeh/Operating-System/assets/90430653/9b1387d9-68ec-44b3-be15-0b2b95d8de18)
測資3 可以看到當計算一個元素時，要存取第二個矩陣的column需要跳到下一行。

而相比測資4計算一個元素時，因為第二個元素為row * 1，所以相當於存取的資料都在cache中(只有第一筆資料會cache miss 其他都會cache hit)，充分利用 space locality。

而測資3(則是當矩陣2的行數夠多，每一次存取都會是cache miss)。

而我們比較兩個測資的時間，也是跟想的一樣。測資4花的時間都較測資3少。



### Scheduler Simulator
#### Scheduler Simulator功能描述
![image](https://user-images.githubusercontent.com/90430653/221483300-d673ce80-1b04-4db1-8678-6d663e60f9f9.png)

#### Scheduler Simulator流程圖
![image](https://user-images.githubusercontent.com/90430653/221483352-0adfc013-fde6-4a3a-8639-72b0d9367619.png)
