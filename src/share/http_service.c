#include "socket.h"

/* 三种robot类型
1. 不接收消息，也不监听事件，只挂在当前线程下，与同线程下的多个robot协同工作（通过调用robot_resume和robot_yield）
2. 接收消息，不监听事件，可以与同线程robot协同工作
3. 接收消息，也监听事件，也可以与通线程robot协同工作
*/

#define HTTP_LISTEN_BACKLOG (32)
#define HTTP_RX_BUFSIZE (1024)

struct httplisten {
  struct ccstring ip;
  ushort_int port;
  void (*connind)(struct ccstate*);
  void* ud;
  int backlog;
};

struct httpconnect {
  handle_int sock;
  struct ccstring localip;
  struct ccstring remoteip;
  ushort_int localport;
  ushort_int remoteport;
  nauty_byte method;
  nauty_byte httpver;
  struct ccstring rstr;
  nauty_byte* method;
};

#define HTTP_METHOD_GET  (1)
#define HTTP_METHOD_HEAD (2)
#define HTTP_METHOD_POST (3)

#define HTTP_VER_0_9 (1)
#define HTTP_VER_1_0 (2)
#define HTTP_VER_1_1 (3)

#define HTTP_SUCCESS (200)
#define HTTP_CREATED (201)
#define HTTP_ACCEPTED (202)
#define HTTP_BAD_REQUEST (400)
#define HTTP_UNAUTHORIZED (401)
#define HTTP_FORBIDDEN (403)
#define HTTP_NOT_FOUND (404)
#define HTTP_METHOD_NOT_ALLOWED (405)
#define HTTP_NOT_ACCEPTABLE (406)
#define HTTP_REQUEST_TIMEOUT (408)
#define HTTP_CONFLICT (409)
#define HTTP_LENGTH_REQUIRED (411)
#define HTTP_REQUEST_ENTITY_TOO_LARGE (413)
#define HTTP_REQUEST_URI_TOO_LONG (414)
#define HTTP_UNSUPPORTED_MEDIA_TYPE (415)
#define HTTP_SERVER_ERROR (500)
#define HTTP_NOT_IMPLEMENTED (501)
#define HTTP_BAD_GATEWAY (502)
#define HTTP_SERVICE_UNAVAILABLE (503)
#define HTTP_VERSION_NOT_SUPPORT (505)

#define HTTP_GIF_IMAGE (0x01) /* image/gif */
#define HTTP_JPG_IMAGE (0x02) /* image/jpeg */
#define HTTP_PNG_IMAGE (0x03) /* image/png */
#define HTTP_BMP_IMAGE (0x04) /* image/bmp */
#define HTTP_HTML_TEXT (0x05) /* text/html */
#define HTTP_CSS_TEXT  (0x06) /* text/css */
#define HTTP_PLAIN_TEXT (0x07) /* text/plain */
#define HTTP_PDF_FILE (0x09) /* application/pdf */
#define HTTP_JAVASCRIPT (0x10) /* application/x-javascript */
#define HTTP_GZIP_FILE (0x12) /* application/x-gzip */

/* Request headers */
#define HTTP_HEADER_FROM (0x01)
#define HTTP_HEADER_HOST (0x02)
#define HTTP_HEADER_REFERER (0x04)
#define HTTP_HEADER_USER_AGENT (0x08)
#define HTTP_HEADER_ACCEPT (0x10)
#define HTTP_HEADER_ACCEPT_CHARSET (0x20)
#define HTTP_HEADER_ACCEPT_ENCODING (0x40)
#define HTTP_HEADER_ACCEPT_LANGUAGE (0x80)
#define HTTP_HEADER_TE (0x0100)

/* Response headers */
#define HTTP_HEADER_RETRY_AFTER (0x1000)
#define HTTP_HEADER_CONTENT_ENCODING (0x2000)
#define HTTP_HEADER_CONTENT_LANGUAGE (0x4000)
#define HTTP_HEADER_CONTENT_LENGTH (0x8000)
#define HTTP_HEADER_CONTENT_MD5 (0x00010000)
#define HTTP_HEADER_CONTENT_RANGE (0x00010000)
#define HTTP_HEADER_CONTENT_TYPE (0x00020000)
#define HTTP_HEADER_SERVER (0x00040000)
#define HTTP_HEADER_LAST_MODIFIED (0x00080000)
#define HTTP_HEADER_EXPIRES (0x00100000)

/* Common headers */
#define HTTP_HEADER_CONNECTION (0x01000000)
#define HTTP_HEADER_KEEP_ALIVE (0x02000000)
#define HTTP_HEADER_TRANSFER_ENCODING (0x04000000)
#define HTTP_HEADER_TRAILER (0x08000000)

/** Method **
HEAD请求必须确保返回的首部与对应的GET请求的首部完全相同
HEAD请求可以在不获取资源的情况下了解资源情况，例如资源的类型，或对象是否存在，或通过查看首部资源是否修改
　** HTTP/<major>.<minor> **
在HTTP/1.0之前，并不要求请求行中包含HTTP/<major>.<minor>
并且请求报文中只包含方法和URL，响应报文中只包含实体，没有头部（如果确定实体的长度呢，必须在发送完实体后关闭连接）
 ** Status code **
100~199 信息
200~299 成功
300~399 资源已经被移走
400~499 客户端错误
500~599 服务器错误
　** Headers **
长的头部可分为多行书写以提高可读性，多出来的每行前至少要有一个空格或制表符tab
Date: Tue,3Oct 1997 02:16:03 GMT
Content-Length: 15040
Content-Type: image/gif
Content-Type: text/html; charset=utf-8
Host: www.joes-hardware.com
Accept: image/gif, image/jpeg, text/html
Accept: *
Accept-Encoding: gzip;q=1.0, idnetity;q=0.5     //q表示接受编码的优先级
Connection: Keep-Alive
Keep-Alive: max=5, timeout=120
TE: trailers, chunked
如果HTTP请求没有包含Accept-Encoding，服务器可以假设客户端能够接受任何编码方式，即等价于 Accept-Encoding:*
TE首部用在请求首部中，告知服务器可以使用哪些传输编码扩展，上面的 TE: trailers, chunked 告诉服务器它可以接受分块编码（如果是HTTP/1.1
程序的话，这是必须的）并且愿意接受附在分块编码报文结尾的拖挂
对它的响应中可使用Transfer-Encoding用于告诉接收方已经使用分块编码对报文进行了传输编码
与Accept-Encoding类似，TE首部也可以使用Q值来说明传输编码的优先顺序，不过HTTP/1.1规范中禁止将分块编码关联的Q值设为0.0
分块编码相当简单，它的每个块包含一个长度值和该分块的数据，长度值是一个十六进制形式的值并以<crlf>与数据分割开
长度值表示的该分块数据的长度，数据紧跟在n<crlf>之后，并且也以<crlf>结束;而分块编码的最后一块总是0<crlf>
如果客户端的TE首部说明它可以接受拖挂的话，就可以在分块的报文最后加上拖挂，拖挂的内容是可选的元数据，客户端不一定需要理解和使用
拖挂中可以包含附带的首部字段，它们的值在报文开始的时候可能无法确定，Content-MD5首部就是一个可以在拖挂中发送的首部
例如在头部包含Trailer: Content-MD5<crlf>，然后在分块编码最后包含Content-MD5:gjqei54p26tjisgj3p4utjgrj53<crlf>
除了Transfer-Encoding、Trailer、以及 Content-Length首部之外，其他HTTP首部都可以作为过挂发送
传输编码是HTTP1.1引入的一个相对较新的特性，实现传输编码的服务器必须特别注意不要把经传输编码后的报文发送给非HTTP/1.1的应用程序
同样地，如果服务器接收到无法理解的经传输编码的报文，应当回复501 Unimplemented状态码，不过所有HTTP/1.1应用程序至少都支持分块编码
 ** Nagle算法与TCP_NODELAY (4.2.6) **
Nagle算法使TCP尽量发送大块数据，减少小块数据的发送以提高发送性能
但是这会引发HTTP一些问题，首先小的HTTP报文可能无法填满一个分组，可能因为等待永远不会到来的额外数据而产生延时
其次，Nagle算法与延迟确认之间的交互存在问题－Nagle算法会阻止数据的发送，直到有确认分组到达，但确认分组自身会被延迟确认算法延迟100~200毫秒
因此HTTP程序常常设置参数TCP_NODELAY来禁用Nagle算法，如果要这么做到话，一定要确保一次性向TCP写入大块数据，这样就不会产生一堆小的分组了
　** 持久连接 **
HTTP/1.1（以及HTTP/1.0的各种增强版本）允许HTTP在失误处理结束之后将TCP连接保持在打开状态，以便未来的HTTP请求重用现在的连接
在事务处理结束后仍然保持在打开状态的TCP连接称为持久连接，非持久连接会在每个事务结束后关闭
持久连接会在不同事务之间保持打开状态，知道客户端或服务器决定将其关闭为止
重用已对目标服务器的空闲持久连接，可以避开缓慢的连接建立阶段，而且已经打开的连接还可以避免慢启动的拥塞适应阶段
管理持久连接时要特别小心，不然就会积累大量的空闲连接，耗费本地以及远程客户和服务器上的资源
持久连接有两种类型：比较老的HTTP/1.0+"keep-alive"连接，以及现代的HTTP/1.1 "persistent"连接 (4.5.2)
如果客户端与一台服务器对话，客户端可以发送Connection: Keep-Alive首部告诉服务器它希望保持连接的活跃状态
如果服务器支持keep-alive，就回送一个Connection: Keep-Alive首部，否则就不发送
问题处在代理上，尤其是那些不理解Connection首部，而且不知道在沿着转发链路将其发送出去之前，应该将该首部删除的代理
Connection首部是个逐条首部，只适用于单条传输链路，不应该沿着传输链路向下传递
HTTP/1.1逐渐停止了keep-alive连接的支持，用一种名为persistent connection的改进型涉及取代了它
持久连接的目的与keep-alive连接相同，但工作机制更优一些，HTTP/1.1持久连接默认情况下是激活的，除非特别说明，否则1.1假定所有连接都是持久的
要在事务处理结束之后将连接关闭，HTTP/1.1程序必须向报文中显示的添加一个Connection:close首部
但是客服端和服务器仍然可以随时关闭空闲连接，不发送Connection:close并不意味这服务器承诺永远将连接保持在打开状态
　** 关闭连接的奥秘 (4.7) **
连接管理，尤其是知道在什么时候以及如何去关闭连接，是HTTP的使用魔法之一
这个问题比很多开发者起初意识到的复杂一些，而且没有很多资料涉及这个问题
所有HTTP客服端、服务器或代理都可以在任意时刻关闭TCP连接，通常会在一条报文结束时关闭连接
但在出错的时候，也可能在首部行的中间或其他奇怪的地方关闭连接
但是，服务器永远都无法确定在它关闭“空闲”连接的那一刻，在线路那一头的客户端有没有数据要发送
如果出现这种情况，客户端会在写入半截请求报文时发现出现了连接错误
每条HTTP响应都应该有精确的Content-Length首部，用以描述响应主体的尺寸
一些老的HTTP服务器会省略Content-Length首部，或者包含错误的长度指示，这样就要依赖服务器发出的连接关闭来说明数据的真实末尾
客服端或代理收到一条随连接关闭而结束的HTTP响应，且实际传输的实体长度与Content-Length并不匹配时，接收端应该质疑长度的正确性
如果接收端是一个缓存代理，接收端就不应该缓存这条响应（以降低今后将潜在的错误报文混合起来的可能）
代理应该将有问题的报文原封不动的转发出去，而不应该试图去“校正”Content-Length，以维护语义的透明性
副作用是一个很重要的问题，如果在发送出一些请求数据之后，收到返回结果之前，连接关闭了，客户端无法百分之百确定服务器端实际激活了多少事务
有些事务，比如GET一个静态页面，可以反复执行多次，也不会有什么变化；而其他一些事务，比如向一个在线书店POST一张订单，就不能重复执行，不然就会有下多张订单的问题
关闭连接的输出信道总是很安全的，连接另一端的对等实体会在从其缓冲区中读出所有数据之后收到一条通知，说明流结束了，这样它就知道你将连接关闭了
但关闭连接的输入信道比较危险，除非你知道另一端不打算再发送其他数据了；如果另一端向你已关闭的输入先到发送数据，操作系统就会向另一端机器发送一条“连接被重置”TCP报文
HTTP规范建议当客户端或服务器突然要关闭一条连接时，应该“正常地关闭传输连接”，但它并没有说明应该如何去做
总之，实现正常关闭的应用程序首先应该关闭它们的输出信道，然后等待连接另一端的对等实体关闭它的输出信道
当两端都告诉对方它们不会再发送数据（比如关闭输出信道）之后，连接就会被完全关闭，而不会有重置的危险
因此，想要正常关闭连接的应用程序应该先半关闭其输出信道，然后周期性的检查其输入信道的状态（查找数据，或流的末尾）
如果在一定的时间内对端没有关闭输入信道，应用程序可以强制关闭连接，以节省资源
  ** 主体内容 **
首部内容以一个空白的CRLF行结束，随后就是实体主体的原始内容，不管内容是什么，文本或二进制、文档或图像、
压缩的或未压缩的、英语、法语或中文，都紧随在这个CRLF之后
Content-Length首部指出报文中实体主体的字节大小，这个大小是包含了所有内容编码的，比如对文本进行gzip压缩的化，是指压缩后的文本大小
除非使用了分块编码，否则Content-Length首部就是带有实体的报文必须使用的
使用Content-Length是为了能够检测出服务器崩溃而导致的报文截断，并对共享持久连接的多个报文进行正确分段
HTTP早期版本采用关闭连接的办法来划定报文的结束，但是没有Content-Length的话，客户端无法区分报文结束时到底是正常连接关闭
还是报文传输过程中由于服务器崩溃而导致的连接关闭，客户端需要通过Content-Length来检测报文截断
报文截断的问题对缓存代理服务器来说尤为严重，如果缓存服务器收到被截断的报文却没有识别出截尾的话，它可能存储不完整的内容
缓存代理服务器通常不会为没有显示Content-Length首部的HTTP主体做缓存，以此来减小缓存已截断报文的危险
错误的Content-Length比缺少Content-Length还要糟糕，因为某些早期的客户端和服务器在Content-Length计算上存在一些众所周知的错误
有些客户端、服务器以及代理中就包含了特别的算法，用来检测和纠正与有缺陷服务器的交互过程
HTTP/1.1规定用户Agent代理应该在接收且检测到无效长度时通知用户
有一种情况下，使用持久连接可以没有Content-Length首部，即采用分块编码（chunked encoding）
该情况下，数据是分为一系列的块来发送的，每块都有大小说明
哪怕服务器在生成首部的时候不知道整个实体的大小（通常是因为实体是动态生成的），仍然可以使用分块编码传输若干已知大小的块
HTTP允许对实体内容进行编码，比如可以使之更安全或进行压缩以节省空间，如果主体进行了编码，Content-Length应该说明编码后的主体字节长度
不幸的是，HTTP/1.1规范中没有首部可以用来说明原始未编码的主体长度，这就客户端难以验证解码过程的完整性　*/

nauty_bool ll_http_filter_connection(struct ccstate* state, handle_int sock, struct httpconnect* conn) {

}

int http_read_startline(struct ccstate* state, struct httprequest* r) {
  state->msg = (struct ccmessage*)r;
  return ll_http_read_startline(state);
}

int http_read_headers(struct ccstate* state, struct httprequest* r) {

}

void ll_http_connection_indication(struct ccstate* s) {
  struct httpconnect* conn = (struct httplisten*)robot_get_specific(s);
  nauty_byte rxbuf[HTTP_RX_BUFSIZE+1];
  /** http request packet formt
  <method> <request-url> HTTP/<major>.<minor>
  <headers> header: value<crlf>
  <crlf>
  <entity-body> */
  handle_int sock = robot_get_eventfd(s);
  sright_int len = HTTP_RX_BUFSIZE, n = 0;
  sright_int n = ccsocket_read(sock, rxbuf, len);
  ll_http_read_startline(state);
}

void　ll_http_receive_connection(struct ccstate* s) {
  struct httplisten* self = (struct httplisten*)robot_get_specific(s);
  struct ccmessage* msg = robot_get_message(s);
  struct httpconnect conn;
  struct httpconnect* specific = 0;
  struct ccrobot* robot = 0;
  ccassert(msg->type == MESSAGE_ID_CONNIND);
  if (!ll_http_filter_connection(s, msg->data->fd, &conn)) {
    return;
  }
  robot = robot_create_new(ll_http_connection_indication, ROBOT_YIELDABLE);
  specific = (struct httpconnect*)robot_set_allocated_specific(robot, sizeof(struct httpconnect));
  *specific = conn;
  robot_set_event(robot, conn.sock, IOEVENT_RDWR, 0);
  robot_start_run(robot);
}

struct ccrobot* http_listen(struct httplisten* self) {
  struct ccsockaddr sa;
  handle_int sock;
  struct ccrobot* robot;
  if (!ccsockaddr_init(&sa, ccstring_getfrom(&self->ip), self->port)) {
    return 0;
  }
  if (self->backlog <= 0) {
    self->backlog = HTTP_LISTEN_BACKLOG;
  }
  sock = ccsocket_listen(&sa, self->backlog);
  if (!ccsocket_isopen(sock)) {
    return 0;
  }
  robot = robot_create_new(ll_http_receive_connection, 0);
  robot->udata = self;
  robot_set_event(robot, sock, IOEVENT_READ, IOEVENT_FLAG_LISTEN);
  robot_start_run(robot);
  return robot;
}

void user_read_request_header(struct ccstate* state) {
  robot_yield(state, user_read_request_data);
}


#if 0
"<scheme>://<user>:<password>@<host>:<port>/<path>;<params>?<query>#<frag>"
/hammers;sale=0;color=red?item=1&size=m
/hammers;sale=false/index.html;graphics=true
/check.cgi?item=12731&color=blue&size=large
/tools.html#drills

URL编码机制，为了使用不安全字符，使用%和两个两个十六进制字符表示，例如空格可使用%20表示，根据浏览器的不同组合成的字符可能使用不同的编码
保留字符有 % / . .. # ? ; : $ + @ & =
受限字符有 {} | \ ^ ~ [ ] ' < > "　以及不可打印字符 0x00~0x1F >=0x7F，而且不能使用空格，通常使用+来替代空格
0~9A~Za~z_-

---
<method> <request-url> HTTP/<major>.<minor>
<headers> header: value<crlf>
<crlf>
<entity-body>
---
HTTP/<major>.<minor> <status-code> <status-text>
<headers> header: value<crlf>
<crlf>
<entity-body>
---
#endif

