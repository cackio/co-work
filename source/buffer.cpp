#include "buffer.h"

Buffer::Buffer(int initBuffSize) : buffer(initBuffSize), read_Pos(0), write_Pos(0)  {}  

// 返回可写的数量：buffer大小 - 写下标
size_t Buffer::Writable_Bytes() const {
    return buffer.size() - write_Pos;
}

// 可读的数量：写下标 - 读下标
size_t Buffer::Readable_Bytes() const {
    return write_Pos - read_Pos;
}

// 可预留空间：已经读过的就没用了，等于读下标
size_t Buffer::Prependable_Bytes() const {
    return read_Pos;
}

const char* Buffer::Get_readpos_ch() const {
    
    return &buffer[read_Pos];
}

// 确保可写的长度
void Buffer::Is_eWriteable(size_t len) {
    if(len > Writable_Bytes()) {
        MakeSpace_(len);
    }
    assert(len <= Writable_Bytes());
}

// 移动写下标，在Append中使用
void Buffer::Move_write_pos(size_t len) {
    write_Pos += len;
}

// 读取len长度，移动读下标
void Buffer::Move_read_pos(size_t len) {
    read_Pos += len;
}

// 读取到end位置
void Buffer::Read_until_end(const char* end) {
    assert(Get_readpos_ch() <= end );
    Move_read_pos(end - Get_readpos_ch()); // end指针 - 读指针 长度
}

// 取出所有数据，buffer归零，读写下标归零,在别的函数中会用到
void Buffer::Read_All() {
    bzero(&buffer[0], buffer.size()); // 覆盖原本数据
    read_Pos = write_Pos = 0;
}

// 取出剩余可读的str
std::string Buffer::Read_leftdata() {
    std::string str(Get_readpos_ch(), Readable_Bytes());
    Read_All();
    return str;
}

// 写指针的位置
const char* Buffer::Get_writepos_Const() const {
    return &buffer[write_Pos];
}

char* Buffer::Get_writepos() {
    return &buffer[write_Pos];
}

// 添加str到缓冲区
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    Is_eWriteable(len);   // 确保可写的长度
    std::copy(str, str + len, Get_writepos());    // 将str放到写下标开始的地方
    Move_write_pos(len);    // 移动写下标
}

void Buffer::Append(const std::string& str) {
    Append(str.c_str(), str.size());             
}

void Buffer::Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

// 将buffer中的读下标的地方放到该buffer中的写下标位置
void Buffer::Append(const Buffer& buff) {
    Append(buff.Get_readpos_ch(), buff.Readable_Bytes());           //不太清楚为什么会需要这个重载
}

// 将fd的内容读到缓冲区，即writable的位置
ssize_t Buffer::Read_Fd(int fd, int* Errno) {
    char buff[65535];   
    struct iovec iov[2];                
    size_t writeable = Writable_Bytes(); // 先记录能写多少
    // 分散读，要保证数据全部读完
    iov[0].iov_base = Get_writepos();    
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *Errno = errno;
    } else if(static_cast<size_t>(len) <= writeable) {   // 若len小于writable，说明写区可以容纳len
        Move_write_pos(len);              // 移动写下标
    } else {    
        write_Pos = buffer.size(); // 写区写满了,下标移到最后
        Append(buff, static_cast<size_t>(len - writeable)); // 剩余的长度
    }
    return len;
}

// 将buffer中可读的区域写入fd中
ssize_t Buffer::Write_Fd(int fd, int* Errno) {
    ssize_t len = write(fd, Get_readpos_ch(), Readable_Bytes());
    if(len < 0) {
        *Errno = errno;
        return len;
    } 
    Move_read_pos(len);
    return len;
}

char* Buffer::BeginPtr_() {
    return &buffer[0];
}

const char* Buffer::BeginPtr_() const{
    return &buffer[0];
}

// 扩展空间
void Buffer::MakeSpace_(size_t len) {
    if(Writable_Bytes() + Prependable_Bytes() < len) {
        buffer.resize(write_Pos + len + 1);
    } else {
        size_t readable_byte = Readable_Bytes();
        std::copy(BeginPtr_() + read_Pos, BeginPtr_() + write_Pos, BeginPtr_());    //将未读数据移动到buffer开头，从而腾出更多的写空间
        read_Pos = 0;          
        write_Pos = readable_byte;
        assert(readable_byte == Readable_Bytes());
    }
}
