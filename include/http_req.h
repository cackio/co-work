#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>    // 正则表达式
#include <errno.h>     
#include <mysql/mysql.h>  //mysql
#include <algorithm>
#include <fstream>
#include "buffer.h"
#include "log.h"
#include "conn_pool.h"
#include <nlohmann/json.hpp>
struct Booking {
    std::string day;         // 日期
    std::string number;      // 票号
    std::string ticket_name;  // 票名称
    std::string kind;         // 票种
    int quantity;             // 票的数量
    };
class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };
    
    HttpRequest() =default;
    ~HttpRequest() = default;

    void Init(const std::string& srcDir);
    bool parse(Buffer& buff);   

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;
    void SaveBookingsToFile(const std::vector<Booking>& bookings, const std::string& fileName);
    bool IsKeepAlive() const;
    std::vector<Booking>bookings;  // 订票记录
private:
    bool ParseRequestLine_(const std::string& line);    // 处理请求行
    void ParseHeader_(const std::string& line);         // 处理请求头
    void ParseBody_(const std::string& line);           // 处理请求体

    void ParsePath_();                                  // 处理请求路径
    void ParsePost_();                                  // 处理Post事件
    void ParseFromUrlencoded_();                        // 从url种解析编码
    void Parsejsondata(std::vector<int>& tickets_ids);   // 解析json数据
    std::string GenerateBookingsHtml(const std::vector<Booking>& bookings);  // 生成订票记录的html
    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);  // 用户验证
    static bool BuyTicket(const std::string &username, int tickets_id);  // 购票
    std::vector<Booking> GetBookings(const std::string &username);  // 获取用户订票记录
    PARSE_STATE state_;
    std::string username; 
    std::string method_, path_, version_, body_;
    std::string srcDir_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);  // 16进制转换为10进制
};


