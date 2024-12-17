#pragma once

#include <cstring>   
#include <iostream>
#include <unistd.h>  
#include <sys/uio.h> 
#include <vector> 
#include <atomic>
#include <assert.h>
class Buffer {
public:
    Buffer(int Init_BuffSize = 1024);
    ~Buffer() = default;
    //函数后的const的意思是该函数不能改变成员变量的值
    size_t Writable_Bytes() const;    //返回可写的数量        
    size_t Readable_Bytes() const ;    //返回可读的数量
    size_t Prependable_Bytes() const;  //返回已经读过的数量
    
    const char* Get_readpos_ch() const;   //返回read_pos指针的位置     
    void Is_eWriteable(size_t len);       //确认可写长度     
    
    const char* Get_writepos_Const() const;     //返回write_pos的位置
    char* Get_writepos();  

    void Move_write_pos(size_t len);      //移动写下标
    void Move_read_pos(size_t len);       //移动读下标         
    
    void Read_until_end(const char* end); //一直读到end
    void Read_All();                 //读出所有数据
    std::string Read_leftdata();         //取出剩余可读的string
                 
 
    void Append(const char* str, size_t len);       //添加str到缓冲区
    void Append(const std::string& str);            //添加c++风格的字符串
    void Append(const void* data, size_t len);      //添加其它类型的数据
    void Append(const Buffer& buff);                

    ssize_t Read_Fd(int fd, int* Errno);           //读Fd的数据到缓冲区
    ssize_t Write_Fd(int fd, int* Errno);          //将缓冲区的数据写到Fd

private:
    char* BeginPtr_();  // buffer开头
    const char* BeginPtr_() const;  //返回buffer[0]指针
    void MakeSpace_(size_t len);    //扩展buffer空间

    std::vector<char> buffer;  
    std::atomic<std::size_t> read_Pos;  // 读的下标
    std::atomic<std::size_t> write_Pos; // 写的下标
    //std::atomic 提供了原子操作的支持，这意味着对这个变量的读取和修改不会被其他线程干扰，保证了数据的一致性和完整性。
};

