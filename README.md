## 演示视频。解封装、队列、解码
[点击查看解封装、队列、解码.mp4](解封装、队列、解码.mp4)

## 演示视频。队列（无stl）、旋转、编码mpeg4、封装avi
[点击查看队列（无stl）、旋转、编码mpeg4、封装avi.mp4](队列（无stl）、旋转、编码mpeg4、封装avi.mp4)

## 变速1
[点击查看变速1.mp4](变速1.mp4)

## 变速2
[点击查看变速2.mp4](变速2.mp4)

## libffmpeg编译  

make distclean清除缓存，export CFLAGS="-fPIC" CXXFLAGS="-fPIC" ASFLAGS="-fPIC" NASMFLAGS="-f elf64 -DPIC"设置变量  

解决导致can not be used when making a shared object; recompile with -fPIC 报错的部分  
./configure \
    --disable-shared \
    --enable-static \
    --disable-programs \
    --disable-doc \
    --enable-pic \
    --extra-cflags="-fPIC -fno-common" \
    --extra-cxxflags="-fPIC" \
    --extra-ldflags="-fPIC" \
    --disable-asm

make -j$(nproc)编译  

合成so  
gcc -shared -fPIC \
    -Wl,--whole-archive \
    libavcodec/libavcodec.a \
    libavformat/libavformat.a \
    libavutil/libavutil.a \
    libavfilter/libavfilter.a \
    libswscale/libswscale.a \
    libswresample/libswresample.a \
    libavdevice/libavdevice.a \
    -Wl,--no-whole-archive \
    -o libffmpeg-whp.so \
    -lz -lm -lpthread -ldl


## 解封装  

初始化FFmpeg库环境，avformat_open_input打开输入文件创建AVFormatContext结构体。调用avformat_find_stream_info函数来填充媒体流的编解码器类型、时长、比特率等信息，获取了流信息后遍历所有流，根据stream、codecpar、codec_type识别音频流和视频流，记录它们的索引号stream_index。创建一个线程来执行实际的解封装操作，循环不断调用av_read_frame函数从容器中读取一个完整的数据包AVPacket，每读取一个数据包就检查它的stream_index属性来确定它属于哪个流，av_packet_clone复制该数据包，将复制的数据包放入相应的队列。在操作队列时使用互斥锁进行保护。当读取到文件末尾或出现错误时av_read_frame返回负值，此时设置队列的完成标志，通知解码线程没有更多数据。资源清理要关闭AVFormatContext释放相关资源。

## 队列  

定义两个视频包队列和音频包队列，继承自一个基础的包队列类，std::queue容器存储AVPacket指针，互斥锁std::mutex保护队列的并发访问，条件变量std::condition_variable线程间的通知机制，一个布尔标志表示队列是否完成数据生产。队列类方法中push方法将数据包添加到队列尾部，先获取互斥锁以保护队列操作，将数据包推入队列，最后通知等待的消费者线程有新数据可用。pop方法从队列头部获取一个数据包，如果队列为空、未标记完成，阻塞当前线程直到有新数据或队列标记为完成，加入超时避免线程永久阻塞。size方法返回当前队列中的元素数量。clear方法清空队列中的所有元素，释放相应的内存。setFinished标记队列不再接收新数据，通知所有等待的消费者线程。isFinished检查队列是否已经完成数据生产。当解封装线程完成所有数据的读取后会调用setFinished方法通知解码线程，解码线程处理完队列中剩余的数据包后可以安全退出。

##  解码  

解码器初始化传入从解封装过程获取的编解码器参数AVCodecParameters，avcodec_find_decoder根据编解码器ID查找对应的解码器，分配解码器上下文AVCodecContext，avcodec_parameters_to_context将编码参数复制到上下文中，avcodec_open2打开解码器准备解码工作，分配用于存储解码后数据的帧结构体AVFrame。线程函数打开输出文件来存储解码后的原始数据，进入主循环不断从对应的数据包队列中获取编码数据包，当队列为空时会短暂等待或检查队列是否已标记完成，获取到数据包后调用avcodec_send_packet将其送入解码器，在内部循环中反复调用avcodec_receive_frame函数获取解码后的帧，每成功接收一个帧就将其转换为相应格式写入输出文件，视频是YUV格式，音频是PCM格式，视频帧写入使用av_image_get_buffer_size、av_image_copy_to_buffer将像素数据线性排列后写入文件，音频帧直接将采样数据写入文件。当达到预设的最大帧数或队列标记完成且无数据可读时结束循环，在主循环结束后发送一个空数据包、继续接收帧直到解码器无帧可提供，最后关闭输出文件，整个解码过程结束后释放所有分配的资源

##  不使用stl来实现队列  
使用基于数组的循环队列来替代STL的std::queue，由VPQ和APQ两个类实现，大小为MAX_QUEUE_SIZE的固定大小的数组存储AVPacket指针，两个索引hd和tl分别标记队列的头部和尾部位置。添加元素时，尾部索引递增，移除元素时，头部索引递增。索引循环使用，当达到数组末尾时回到开头。psh()方法在队列尾部添加元素，但只有队列未满(cnt < cap)时才添加。pp()方法从队列头部取出元素并返回，如果队列为空则返回nullptr。sz()方法返回当前队列中的元素数量。队列有一个自旋锁机制，volatile bool lck变量实现，每个操作前都会检查锁是否被占用while(lck) {}，然后设置锁为true，操作完成后释放锁。setF()和isF()方法维护一个标志f，即是否所有数据都已经入队，明确何时结束处理

##  OpenGL实现视频旋转  
GL类在初始化设置了一个不可见的OpenGL上下文和帧缓冲对象FBO，创建了三个纹理对象对应YUV的三个平面，那么可直接操作原始YUV数据，整个初始化要设置GLFW窗口、初始化GLEW、编译着色器程序、创建顶点数组、缓冲对象、设置帧缓冲和渲染缓冲。旋转功能接收输入AVFrame，它是YUV格式，执行旋转操作，将结果写入输出AVFrame，使用了2D旋转矩阵，顶点着色器应用到每个顶点来旋转整个图像。角度参数由弧度转换(angle * PI / 180)传递给着色器。顶点着色器应用旋转矩阵变换顶点位置、传递纹理坐标，片段着色器则从YUV纹理采样、转换为RGB颜色空间。色彩转换使用BT.709标准矩阵。渲染过程中将YUV平面作为三个独立纹理上传到GPU，然后绘制一个覆盖整个视口的四边形，应用旋转变换和YUV到RGB转换。渲染结果存储在帧缓冲中，glReadPixels读回CPU内存。最后代码将RGB数据转换回YUV格式，写入输出帧

##  视频编码格式为mpeg4格式  
编码器初始化由avcodec_find_encoder(AV_CODEC_ID_MPEG4)找到FFmpeg库中的MPEG4编码器，ID AV_CODEC_ID_MPEG4明确指定了输出格式为MPEG4，创建编码上下文、设置一些参数，比特率控制视频质量和文件大小、分辨率定义视频尺寸、时基和帧率设置为25fps、GOP设置为10，每10帧会有一个关键帧、允许1个B帧提高压缩效率、像素格式设置为YUV420P。编码在et线程函数中执行，读取YUV文件的每一帧，数据填充到AVFrame结构中，设置时间戳pts，avcodec_send_frame将帧送入编码器。编码器处理后，avcodec_receive_packet获取编码后的视频包。每个编码包都是MPEG4格式的一部分，这些包由VPQ队列传递给下游Muxer处理。编码结束时发送一个空帧nullptr给编码器，触发编码器内部缓冲区的刷新，保证所有待处理帧都编码。最后将队列标记为完成setF，编码过程结束

##  muxer封装成avi  
avformat_alloc_output_context2(&fmt_ctx, nullptr, "avi", out.c_str())创建输出格式上下文，"avi"指定了输出容器格式为AVI。创建一个新的视频流，设置其编解码参数，指定流类型为视频AVMEDIA_TYPE_VIDEO，编码格式为MPEG4(AV_CODEC_ID_MPEG4)，还有视频宽度、高度、像素格式比特率等。设置时基为{1, fps}。avio_open打开输出文件，关联到格式上下文，调用avformat_write_header写入AVI文件头，包含了文件类型、视频流信息、编解码器信息等元数据。实际的封装过程在mux线程函数中执行，从队列中不断获取编码后的视频包AVPacket，设置流索引，av_interleaved_write_frame函数将包写入AVI文件。所有视频包都处理完毕后，析构函数中调用av_write_trailer写入AVI文件尾部，完成文件结构，这个尾部有索引表和一些元数据。最后关闭输出文件并释放资源完成封装。

## 如何保证速度一致  
从解封装器Demuxer获取原始视频的帧率和时基信息，传递给编码器和封装器，在编码时保持原始帧的时间戳比例，OpenGL处理时不要改变帧速率，只处理空间变换，Muxer中确保使用与编码器匹配的时基

## 环形缓冲与变速  
环形缓冲区通过BUF类实现，使用固定大小内存块存储数据，读写指针追踪位置，当指针到达缓冲区末尾时自动循环回到开头，当数据需要跨越缓冲区末尾时分两部分处理，用互斥锁和条件变量令线程安全。音视频同步播放中时间戳在解封装过程中保留原始媒体的PTS/DTS信息，音视频帧正确时序，在编码和封装过程中使用相同时基，在变速处理时同步调整视频帧率和音频采样率。视频变速调整输出帧率实现，2倍速时设置为原始视频帧率的两倍，相同数量帧在更短时间内播放完成，音频变速不变调使用FFmpeg的atempo滤镜，超过2倍速的情况以滤镜链接方式，3倍速使用atempo=2.0和atempo=1.5的组合。