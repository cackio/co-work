#include "http_req.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture", };

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };

void HttpRequest::Init(const std::string& srcDir) {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    srcDir_ = srcDir;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

// 解析处理
bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";      // 行结束符标志(回车换行)
    if(buff.Readable_Bytes() <= 0) { // 没有可读的字节
        return false;
    }
    // 读取数据
    while(buff.Readable_Bytes() && state_ != FINISH) {
        // 从buff中的读指针开始到读指针结束，这块区域是未读取得数据并去处"\r\n"，返回有效数据得行末指针
        const char* lineEnd = std::search(buff.Get_readpos_ch(), buff.Get_writepos_Const(), CRLF, CRLF + 2);
        // 转化为string类型
        std::string line(buff.Get_readpos_ch(), lineEnd);
        switch(state_)
        {
        /*
            有限状态机，从请求行开始，每处理完后会自动转入到下一个状态    
        */
        case REQUEST_LINE:
            if(!ParseRequestLine_(line)) {
                return false;
            }
            ParsePath_();   // 解析路径
            break;
        case HEADERS:
            ParseHeader_(line);
            if(buff.Readable_Bytes() <= 2) { 
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        if(lineEnd == buff.Get_writepos_Const()) { break; } // 读完了
        buff.Read_until_end(lineEnd + 2);        // 跳过回车换行
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

// 解析路径
void HttpRequest::ParsePath_() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine_(const std::string& line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$"); 
    std::smatch subMatch;
    // 在匹配规则中，以括号()的方式来划分组别 一共三个括号 [0]表示整体
    if(regex_match(line, subMatch, patten)) {      // 匹配指定字符串整体是否符合
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;   // 状态转换为下一个状态
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const std::string& line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        state_ = BODY;  // 状态转换为下一个状态
    }
}

void HttpRequest::ParseBody_(const std::string& line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;    // 状态转换为下一个状态
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

// 16进制转化为10进制
int HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

// 处理post请求
void HttpRequest::ParsePost_() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded_();     // POST请求体示例
        if(DEFAULT_HTML_TAG.count(path_)) { // 如果是登录/注册的path
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);  // 为1则是登录
                if(UserVerify(post_["username"], post_["password"], isLogin)) {
                    username = post_["username"];
                    path_ = "/welcome.html";
                }
                else {
                    path_ = "/error.html";
                }
            }
        }
    }
    if(method_.find("POST")!= std::string::npos&& header_["Content-Type"] == "application/json") {
        // 如果是购票请求
        if (path_=="/buy_tickets") {
            //std::string ticket_id = post_["ticket_id"];
            //int quantity = std::stoi(post_["quantity"]);
            username = "uestc";
            std::vector<int> ticket_ids;
            Parsejsondata(ticket_ids);
            // if(ticket_ids.empty()) {
            //     path_ = "/errorbuy.html";  // 购票失败
            // }
            for (int ticket_id : ticket_ids) {
                if (!BuyTicket(username, ticket_id)) {
                    path_ = "/errorticket.html";  // 购票失败（如票数不足）
                    break;
                }
            }
            path_ = "/index.html";  // 购票成功
        }
    }
        // 如果是查看已购票记录请求
        if (path_ == "/view_bookings") {
            std::string requestedFile = srcDir_ + path_;
            bookings = GetBookings(post_["username"]);  // 获取用户已购票记录
            SaveBookingsToFile(bookings,"data.json");  // 将用户已购票记录保存到文件
            path_ = "/view_bookings.html";  // 显示已购票记录
        }
}

/*std::string HttpRequest::GenerateBookingsHtml(const std::vector<Booking>& bookings) {
    std::string bookingsHtml;
    for (const auto& booking : bookings) {
        bookingsHtml += "<div class='booking-item'>";
        bookingsHtml += "<p>票名称: " + booking.ticket_name + "</p>";
        bookingsHtml += "<p>数量: " + std::to_string(booking.quantity) + "</p>";
        bookingsHtml += "<p>订票时间: " + booking.booking_time + "</p>";
        bookingsHtml += "</div>";
    }
    return bookingsHtml;
}*/

// 从url中解析编码
void HttpRequest::ParseFromUrlencoded_() {
    if(body_.size() == 0) { return; }

    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        // key
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        // 键值对中的空格换为+或者%20
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        // 键值对连接符
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}
 void HttpRequest::Parsejsondata(std::vector<int>& tickets_ids)
 {
    size_t tickets_pos = body_.find("\"tickets\":[");
if (tickets_pos == std::string::npos) {
    std::cerr << "Error: 'tickets' field not found!" << std::endl;
    return;
}

// 提取 "tickets" 字段的值（找到方括号内的内容）
tickets_pos += 11;  // 跳过 "\"tickets\":["
size_t end_pos = body_.find("]", tickets_pos);
std::string tickets_value = body_.substr(tickets_pos, end_pos - tickets_pos);

// 将数组字符串 "1","2" 拆分为单独的数字
std::stringstream ss(tickets_value);
std::string token;
while (std::getline(ss, token, ',')) {
    // 去除引号
    token.erase(std::remove(token.begin(), token.end(), '\"'), token.end());
    tickets_ids.push_back(std::stoi(token));  // 转换为整数并存储
}
 }

//验证用户的登录或注册请求
bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool isLogin) {
    if(name == "" || pwd == "") { return false; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);
    
    bool flag = false;
    unsigned int j = 0;
    char order[256] = { 0 };
    MYSQL_FIELD *fields = nullptr;  //用于描述查询结果中的字段（列）的元数据。它包含字段的名称、类型、长度等信息。
    MYSQL_RES *res = nullptr; //用于存储查询结果集，它包含查询返回的所有行和列的数据。
    
    if(!isLogin) { flag = true; }
    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res);
        return false; 
    }
    res = mysql_store_result(sql); //将查询结果存储在客户端内存中
    j = mysql_num_fields(res);       //获取结果集中字段（列）的数量
    fields = mysql_fetch_fields(res); //获取结果集中所有字段的元数据

    while(MYSQL_ROW row = mysql_fetch_row(res))        //用于从结果集中获取下一行数据用于从结果集中获取下一行数据
    {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        /* 注册行为且用户名未被使用*/
        if(isLogin) {
            if(pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_INFO("pwd error!");
            }
        } 
        else { 
            flag = false; 
            LOG_INFO("user used!");
        }
    }
    mysql_free_result(res);

    /* 注册行为且用户名未被使用*/
    if(!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    // SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    return flag;
}
bool HttpRequest::BuyTicket(const std::string &username, int ticket_id) {
    if (username.empty() || ticket_id<0) {
        return false;
    }
    LOG_INFO("Buy ticket, username:%s ticket_id:%d quantity:1", username.c_str(), ticket_id);
    
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());  // 获取数据库连接
    assert(sql);
    
    bool flag = false;
    unsigned int j = 0;
    char order[512] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    
    // 1. 查找用户ID
    snprintf(order, sizeof(order), "SELECT user_id FROM users WHERE username='%s' LIMIT 1", username.c_str());
    LOG_DEBUG("SQL Query: %s", order);

    if (mysql_query(sql, order)) {
        LOG_ERROR("Query failed: %s", mysql_error(sql));
        mysql_free_result(res);
        return false;
    }

    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    int user_id = -1;
    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        if (row[0]) {
            user_id = std::stoi(row[0]);
        }
    }
    
    mysql_free_result(res);

    if (user_id == -1) {
        LOG_ERROR("User not found: %s", username.c_str());
        return false;
    }

    // 2. 查找票务ID和剩余票数
    snprintf(order, sizeof(order), "SELECT ticket_id, available_count FROM tickets WHERE ticket_id=%d LIMIT 1", ticket_id);
    LOG_DEBUG("SQL Query: %s", order);

    if (mysql_query(sql, order)) {
        LOG_ERROR("Query failed: %s", mysql_error(sql));
        mysql_free_result(res);
        return false;
    }

    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    //int ticket_id = -1;
    int available_count = 0;
    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        if (row[0] && row[1]) {
            ticket_id = std::stoi(row[0]);
            available_count = std::stoi(row[1]);
        }
    }
    
    mysql_free_result(res);

    if (ticket_id == -1) {
        LOG_ERROR("Ticket not found: %d", ticket_id);
        return false;
    }

    // 3. 检查剩余票数是否足够
    if (available_count < 1) {
        LOG_ERROR("Not enough tickets available: available=%d", available_count);
        return false;
    }

    // 4. 更新票务剩余数量
    snprintf(order, sizeof(order), "UPDATE tickets SET available_count = available_count - %d WHERE ticket_id='%d'", 1, ticket_id);
    LOG_DEBUG("SQL Update: %s", order);
    if (mysql_query(sql, order)) {
        LOG_ERROR("Update failed: %s", mysql_error(sql));
        return false;
    }
    //5.查询是否已有订票记录
    snprintf(order, sizeof(order), 
          "SELECT COUNT(*) FROM booking_records WHERE user_id = %d AND ticket_id = %d", 
          user_id, ticket_id);
    if (mysql_query(sql, order)) {
    LOG_ERROR("Query failed: %s", mysql_error(sql));
    return false;  // 查询失败时返回 false
    }
    res = mysql_store_result(sql);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {
    // 检查 COUNT(*) 的结果
    int count = atoi(row[0]);  // row[0] 是查询结果中的第一列，即 COUNT(*) 的结果
    if (count > 0) {
        // 记录存在
        LOG_DEBUG("Record exists.");

        // 执行更新操作，将 quantity + 1
        snprintf(order, sizeof(order), 
                  "UPDATE booking_records SET quantity = quantity + 1 WHERE user_id = %d AND ticket_id = %d", 
                  user_id, ticket_id);
        LOG_DEBUG("SQL Update: %s", order);
        if (mysql_query(sql, order)) {
            LOG_ERROR("Update booking failed: %s", mysql_error(sql));
            mysql_free_result(res);  // 释放查询结果
            return false;
        }

    } 
    else {
    // 记录不存在 插入订票记录
    snprintf(order, sizeof(order), 
          "INSERT INTO booking_records(user_id, ticket_id, quantity) "
          "VALUES(%d, %d, %d)", 
          user_id, ticket_id, 1);
    LOG_DEBUG("SQL Insert: %s", order);
    if (mysql_query(sql, order)) {
        LOG_ERROR("Insert booking failed: %s", mysql_error(sql));
        return false;
    }
    }
    }
    // 购票成功
    LOG_DEBUG("Ticket bought successfully: username=%s, ticket_name=%d, quantity=%d", username.c_str(), ticket_id, 1);
    return true;
}

std::vector<Booking> HttpRequest::GetBookings(const std::string &username) {
    if (username.empty()) {
        return {};  // 返回空的查询结果
    }
    LOG_INFO("Get bookings for username: %s", username.c_str());
    
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);
    
    std::vector<Booking> bookings;
    unsigned int j = 0;
    char order[512] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    // 1. 查找用户ID
    snprintf(order, sizeof(order), "SELECT user_id FROM users WHERE username='%s' LIMIT 1", username.c_str());
    LOG_DEBUG("SQL Query: %s", order);

    if (mysql_query(sql, order)) {
        LOG_ERROR("Query failed: %s", mysql_error(sql));
        return bookings;  // 返回空的查询结果
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    int user_id = -1;
    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        if (row[0]) {
            user_id = std::stoi(row[0]);
        }
    }
    
    mysql_free_result(res);

    if (user_id == -1) {
        LOG_ERROR("User not found: %s", username.c_str());
        return bookings;
    }

    // 2. 查询该用户的所有订票记录
    snprintf(order, sizeof(order), 
          "SELECT tickets.day, tickets.number, tickets.ticket_name, booking_records.quantity "
          "FROM booking_records "
          "JOIN tickets ON booking_records.ticket_id = tickets.ticket_id "
          "WHERE booking_records.user_id = %d", 
          user_id);

    LOG_DEBUG("SQL Query: %s", order);

    if (mysql_query(sql, order)) {
        LOG_ERROR("Query failed: %s", mysql_error(sql));
        return bookings;
    }

    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        Booking booking;
        if (row[0] && row[1] && row[2] && row[3]) {
            booking.day = row[0];
            booking.number = row[1];
            booking.ticket_name = row[2];
            booking.quantity = std::stoi(row[3]);
            bookings.push_back(booking);
        }
    }
    
    mysql_free_result(res);
    return bookings;
}


std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path(){
    return path_;
}
std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}
void HttpRequest::SaveBookingsToFile(const std::vector<Booking>& bookings, const std::string& fileName) {
    // 清空文件内容（如果文件存在）
    std::ofstream outFile(fileName, std::ofstream::out | std::ofstream::trunc);  // 打开文件并清空内容
    if (!outFile.is_open()) {
        std::cerr << "无法打开文件进行清空。" << std::endl;
        return;
    }

    // 创建一个空的 JSON 数组
    nlohmann::json jsonBookings = nlohmann::json::array();

    // 将 bookings 转换为 JSON 格式
    for (const auto& booking : bookings) {
        nlohmann::json jsonBooking;
        jsonBooking["day"] = booking.day;
        jsonBooking["number"] = booking.number;
        jsonBooking["ticket_name"] = booking.ticket_name;
        jsonBooking["quantity"] = booking.quantity;
        jsonBookings.push_back(jsonBooking);
    }

    // 将 JSON 数据写入文件
    outFile << jsonBookings.dump(4);  // 格式化输出，使用 4 个空格缩进
    outFile.close();
}
