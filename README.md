# ChatRoom
epoll_LT模式架构实现聊天室

## 服务器
使用epoll多路复用的LT模式<br>
服务器默认端口为4507，可在**server.c**文件的**line:3**修改

## 客户端
运行时需要给两个参数
  > -p:服务器端口


  > -a:服务器地址
  > 
  > 本机测试默认为：./client -p 4507 -a 127.0.0.1
    
联机测试时，终端输入：
  > **ip addr**
  > 获得自己的ip地址


## 使用
~~### 服务器端：~~
  > ~~gcc -o server server.c ser.c group.c func.c mysql.c -pthread -lmysqlclient~~



~~### 客户端：~~
  > ~~gcc -o client client.c func.c -pthread -lmysqlclient~~


  > cd make
  
  > make

### 关于mysql数据库
  **请务必在使用前更改成自己的MYSQL**
  有关服务器的设置在**server.c**文件的**line:46**<br>
  **请手动建库"testdb":** 
  
    create database testdb default charset=utf8;

### 关于文件传送功能
  **服务器**的文件暂存地址为：  **./../file_buf/**
    可在**ser.c**文件的**line:1657**进行修改.  
    
  **客户端**的文件存放地址为：**/home/crushbb/Desktop/**
    **请务必在使用前更改为自己的地址**：在**client.c**文件的**line:116**进行修改
