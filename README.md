# 一个从零开始没有使用任何第三方库的流媒体服务器

### 项目特点

- 支持rtsp推流，http-flv，hls，rtsp拉流
- 支持linux、macos、windows平台
- 代码清晰，无任何依赖


### 编译代码
```console
$ git clone https://github.com/xfxhn/mediaServer
$ cd mediaServer
$ cmake -D CMAKE_BUILD_TYPE="Debug" -S . -B build
$ cmake --build build --target install
```


### 使用 FFmpeg 推流
现在暂时只支持rtsp over tcp的推流
```bash
ffmpeg -re -i "test.mp4" -vcodec h264 -acodec aac -f rtsp -rtsp_transport tcp rtsp://127.0.0.1/live/test
```


### 播放直播流
#### flv 流格式
```bash
ffplay http://localhost/live/test.flv
```

#### hls 流格式
```bash
ffplay http://localhost/live/test/test.m3u8
```

#### rtsp 流格式
```bash
ffplay rtsp://127.0.0.1/live/test -rtsp_transport tcp
```



****

**这个项目是我个人为了学习流媒体协议，和音视频知识所创建的项目，所以里面所有用到的封装和解封装都是自己编写， 现在还有很多协议和特性没支持，比如rtmp，比如接入https，rtsps，以后会慢慢添加。**

**并且由于我不是专门做后端的， 性能方面还有很多可以优化，比如我只是简单写了个线程池，来处理每个socket链接， 没有使用epoll，select等网络模型，这对于高并发肯定是不行的，并且代码还有很多可以优化的地方，如果有大佬，觉得
我某段代码实现不够效率，不够优雅，也欢迎加入来一起开发这个项目**

**不过这个项目作为学习流媒体服务器，确实是一个不错的项目，没有什么奇技淫巧的优化，也完全没有使用任何第三方库， 代码够简单清晰。 如果你有什么疑问，可以加qq群814462428，我的qq号是2456346488**


**我也会不断完善这个项目，并能够让其真正商用。**