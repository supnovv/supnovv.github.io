#include <stddef.h>
#include "net/http.h"

#define L_HTTP_METHOD_MAX_LEN (7)
#define L_NUM_OF_HTTP_METHODS (4)

enum l_http_methods_enum {
  L_HTTP_M_GET,
  L_HTTP_M_HEAD,
  L_HTTP_M_OPTIONS,
  L_HTTP_M_POST,
  L_HTTP_LAST_METHOD
};

static const l_strn l_http_methods[] = {
  l_literal_strn("GET"),
  l_literal_strn("HEAD"),
  l_literal_strn("OPTIONS"),
  l_literal_strn("POST")
};

#define L_HTTP_VER_MAX_LEN (8)
#define L_NUM_OF_HTTP_VERS (4)

enum l_http_versions_enum {
  L_HTTP_VER_0NN,
  L_HTTP_VER_10N,
  L_HTTP_VER_11N,
  L_HTTP_VER_2NN
};

static const l_strn l_http_versions[] = {
  l_literal_strn("HTTP/0"),
  l_literal_strn("HTTP/1.0"),
  l_literal_strn("HTTP/1.1"),
  l_literal_strn("HTTP/2")
};

typedef struct {
  int code;
  l_strn phrase;
} l_http_status;

enum l_http_status_enum {
  L_HTTP_100_CONTINUE,
  L_HTTP_101_SWITCHING_PROTOCOLS,
  L_HTTP_200_OK,
  L_HTTP_201_CREATED,
  L_HTTP_202_ACCEPTED,
  L_HTTP_203_NON_AUTHORITATIVE_INFORMATION,
  L_HTTP_204_NO_CONTENT,
  L_HTTP_205_RESET_CONTENT,
  L_HTTP_206_PARTIAL_CONTENT,
  L_HTTP_300_MULTIPLE_CHOICES,
  L_HTTP_301_MOVED_PERMANENTLY,
  L_HTTP_302_FOUND,
  L_HTTP_303_SEE_OTHER,
  L_HTTP_304_NOT_MODIFIED,
  L_HTTP_305_USE_PROXY,
  L_HTTP_307_TEMPORARY_REDIRECT,
  L_HTTP_400_BAD_REQUEST,
  L_HTTP_401_UNAUTHORIZED,
  L_HTTP_402_PAYMENT_REQUIRED,
  L_HTTP_403_FORBIDDEN,
  L_HTTP_404_NOT_FOUND,
  L_HTTP_405_METHOD_NOT_ALLOWED,
  L_HTTP_406_NOT_ACCEPTABLE,
  L_HTTP_407_PROXY_AUTHENTICATION_REQUIRED,
  L_HTTP_408_REQUEST_TIMEOUT,
  L_HTTP_409_CONFLICT,
  L_HTTP_410_GONE,
  L_HTTP_411_LENGTH_REQUIRED,
  L_HTTP_412_PRECONDITION_FAILED,
  L_HTTP_413_REQUEST_ENTITY_TOO_LARGE,
  L_HTTP_414_REQUEST_URI_TOO_LONG,
  L_HTTP_415_UNSUPPORTED_MEDIA_TYPE,
  L_HTTP_416_REQUESTED_RANGE_NOT_SATISFIABLE,
  L_HTTP_417_EXPECTATION_FAILED,
  L_HTTP_500_INTERNAL_SERVER_ERROR,
  L_HTTP_501_NOT_IMPLEMENTED,
  L_HTTP_502_BAD_GATEWAY,
  L_HTTP_503_SERVICE_UNAVAILABLE,
  L_HTTP_504_GATEWAY_TIMEOUT,
  L_HTTP_505_HTTP_VERSION_NOT_SUPPORTED,
  L_HTTP_LAST_STATUS_CODE
};

static const l_http_status l_status[] = { /* https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html */
  {100, l_literal_strn("Continue")}, /* 1xx - informational code */
  {101, l_literal_strn("Switching Protocols")},
  {200, l_literal_strn("OK")}, /* 2xx - success code */
  {201, l_literal_strn("Created")},
  {202, l_literal_strn("Accepted")},
  {203, l_literal_strn("Non-Authoritative Information")},
  {204, l_literal_strn("No Content")},
  {205, l_literal_strn("Reset Content")},
  {206, l_literal_strn("Partial Content")},
  {300, l_literal_strn("Multiple Choices")}, /* 3xx - redirection status */
  {301, l_literal_strn("Moved Permanently")},
  {302, l_literal_strn("Found")},
  {303, l_literal_strn("See Other")},
  {304, l_literal_strn("Not Modified")},
  {305, l_literal_strn("Use Proxy")},
  {307, l_literal_strn("Temporary Redirect")},
  {400, l_literal_strn("Bad Request")}, /* 4xx - request error */
  {401, l_literal_strn("Unauthorized")},
  {402, l_literal_strn("Payment Required")},
  {403, l_literal_strn("Forbidden")},
  {404, l_literal_strn("Not Found")},
  {405, l_literal_strn("Method Not Allowed")},
  {406, l_literal_strn("Not Acceptable")},
  {407, l_literal_strn("Proxy Authentication Required")},
  {408, l_literal_strn("Request Timeout")},
  {409, l_literal_strn("Conflict")},
  {410, l_literal_strn("Gone")},
  {411, l_literal_strn("Length Required")},
  {412, l_literal_strn("Precondition Failed")},
  {413, l_literal_strn("Request Entity Too Large")},
  {414, l_literal_strn("Request-URI Too Long")},
  {415, l_literal_strn("Unsupported Media Type")},
  {416, l_literal_strn("Requested Range Not Satisfiable")},
  {417, l_literal_strn("Expectation Failed")},
  {500, l_literal_strn("Internal Server Error")}, /* 5xx - server error */
  {501, l_literal_strn("Not Implemented")},
  {502, l_literal_strn("Bad Gateway")},
  {503, l_literal_strn("Service Unavailable")},
  {504, l_literal_strn("Gateway Timeout")},
  {505, l_literal_strn("HTTP Version Not Supported")}
};

#define HTTP_PDF_FILE (0x09) /* application/pdf */
#define HTTP_JAVASCRIPT (0x10) /* application/x-javascript */
#define HTTP_GZIP_FILE (0x12) /* application/x-gzip */

enum l_http_mime_types_enum {
  L_HTTP_MIME_HTML,
  L_HTTP_MIME_PLAIN,
  L_HTTP_MIME_CSS,
  L_HTTP_MIME_GIF,
  L_HTTP_MIME_JPEG,
  L_HTTP_MIME_PNG,
  L_HTTP_MIME_BMP,
  L_HTTP_MIME_SVG,
  L_HTTP_MIME_ICO,
  L_HTTP_MIME_PDF,
  L_HTTP_MIME_JSON,
  L_HTTP_MIME_JS,
  L_HTTP_MIME_GZIP,
  L_HTTP_MIME_ZIP,
  L_HTTP_LAST_MIME_TYPE
};

static const l_strn l_mime_types[] = {
  l_literal_strn("text/html"),
  l_literal_strn("text/plain"),
  l_literal_strn("text/css"),
  l_literal_strn("image/gif"),
  l_literal_strn("image/jpeg"),
  l_literal_strn("image/png"),
  l_literal_strn("image/bmp"),
  l_literal_strn("image/svg+xml"),
  l_literal_strn("image/x-icon"),
  l_literal_strn("application/pdf"),
  l_literal_strn("application/json"),
  l_literal_strn("application/x-javascript"),
  l_literal_strn("application/x-gzip"),
  l_literal_strn("application/zip")
};


#define L_HTTP_HEADER_MAX_LEN (27)
#define L_NUM_OF_COMMON_HEADERS (30)
#define L_NUM_OF_REQUEST_HEADERS (31)
#define L_NUM_OF_RESPONSE_HEADERS (25)

enum l_http_common_headers_enum {
  L_HTTP_H_AGE,
  L_HTTP_H_CACHE_CONTROL,
  L_HTTP_H_CONNECTION,
  L_HTTP_H_CONTENT_BASE,
  L_HTTP_H_CONTENT_DISPOSITION,
  L_HTTP_H_CONTENT_ENCODING,
  L_HTTP_H_CONTENT_LANGUAGE,
  L_HTTP_H_CONTENT_LENGTH,
  L_HTTP_H_CONTENT_LOCATION,
  L_HTTP_H_CONTENT_MD5,
  L_HTTP_H_CONTENT_RANGE,
  L_HTTP_H_CONTENT_TYPE,
  L_HTTP_H_DATE,
  L_HTTP_H_ETAG,
  L_HTTP_H_EXPIRES,
  L_HTTP_H_KEEP_ALIVE,
  L_HTTP_H_LAST_MODIFIED,
  L_HTTP_H_PRAGMA,
  L_HTTP_H_TRAILER,
  L_HTTP_H_TRANSFER_ENCODING,
  L_HTTP_H_UPGRADE,
  L_HTTP_H_VIA,
  L_HTTP_H_WARNING,
  L_HTTP_H_X_REQUEST_ID,
  L_HTTP_H_X_CORRELATION_ID,
  L_HTTP_H_FORWARDED,
  L_HTTP_H_MAX_FORWARDEDS,
  L_HTTP_H_X_FORWARDED_FOR,
  L_HTTP_H_X_FORWARDED_HOST,
  L_HTTP_H_X_FORWARDED_PROOTO,
  L_HTTP_LAST_COMMON_HEADER
};

static const l_strn l_http_common_headers[] = {
  l_literal_strn("age"),
  l_literal_strn("cache-control"),
  l_literal_strn("connection"),
  l_literal_strn("content-base"),
  l_literal_strn("content-disposition"),
  l_literal_strn("content-encoding"),
  l_literal_strn("content-language"),
  l_literal_strn("content-length"),
  l_literal_strn("content-location"),
  l_literal_strn("content-md5"),
  l_literal_strn("content-range"),
  l_literal_strn("content-type"),
  l_literal_strn("date"),
  l_literal_strn("etag"),
  l_literal_strn("expires"),
  l_literal_strn("keep-alive"),
  l_literal_strn("last-modified"),
  l_literal_strn("pragma"),
  l_literal_strn("trailer"),
  l_literal_strn("transfer-encoding"),
  l_literal_strn("upgrade"),
  l_literal_strn("via"),
  l_literal_strn("warning"),
  l_literal_strn("x-request-id"),
  l_literal_strn("x-correlation-id"),
  l_literal_strn("forwarded"),
  l_literal_strn("max-forwards"),
  l_literal_strn("x-forwarded-for"),
  l_literal_strn("x-forwarded-host"),
  l_literal_strn("x-forwarded-proto")
};

enum l_http_request_headers_enum {
  L_HTTP_H_ACCEPT,
  L_HTTP_H_ACCEPT_CHARSET,
  L_HTTP_H_ACCEPT_ENCODING,
  L_HTTP_H_ACCEPT_LANGUAGE,
  L_HTTP_H_ACCEPT_DATETIME,
  L_HTTP_H_AUTHORIZATION,
  L_HTTP_H_COOKIE,
  L_HTTP_H_COOKIE2,
  L_HTTP_H_DNT,
  L_HTTP_H_EXPECT,
  L_HTTP_H_FORM,
  L_HTTP_H_HOST,
  L_HTTP_H_IF_MATCH,
  L_HTTP_H_IF_MODIFIED_SINCE,
  L_HTTP_H_IF_NONE_MATCH,
  L_HTTP_H_IF_RANGE,
  L_HTTP_H_IF_UNMODIFIED_SINCE,
  L_HTTP_H_ORIGIN,
  L_HTTP_H_PROXY_AUTHORIZATION,
  L_HTTP_H_PROXY_CONNECTION,
  L_HTTP_H_RANGE,
  L_HTTP_H_REFERER,
  L_HTTP_H_TE,
  L_HTTP_H_UA_CPU,
  L_HTTP_H_UA_DISP,
  L_HTTP_H_UA_OS,
  L_HTTP_H_USER_AGENT,
  L_HTTP_H_X_REQUESTED_WITH,
  L_HTTP_H_X_HTTP_MOTHOD_OVERRIDE,
  L_HTTP_H_X_UIDH,
  L_HTTP_H_X_CSRF_TOKEN,
  L_HTTP_LAST_REQUEST_HEADER
};

static const l_strn l_http_request_headers[] = {
  l_literal_strn("accept"),
  l_literal_strn("accept-charset"),
  l_literal_strn("accept-encoding"),
  l_literal_strn("accept-language"),
  l_literal_strn("accept-datetime"),
  l_literal_strn("authorization"),
  l_literal_strn("cookie"),
  l_literal_strn("cookie2"),
  l_literal_strn("dnt"),
  l_literal_strn("expect"),
  l_literal_strn("from"),
  l_literal_strn("host"),
  l_literal_strn("if-match"),
  l_literal_strn("if-modified-since"),
  l_literal_strn("if-none-match"),
  l_literal_strn("if-range"),
  l_literal_strn("if-unmodified-since"),
  l_literal_strn("origin"),
  l_literal_strn("proxy-authorization"),
  l_literal_strn("proxy-connection"),
  l_literal_strn("range"),
  l_literal_strn("referer"),
  l_literal_strn("te"), /* client accept transfer encoding, TE: trailers, chunked */
  l_literal_strn("ua-cpu"),
  l_literal_strn("ua-disp"),
  l_literal_strn("ua-os"),
  l_literal_strn("user-agent"),
  l_literal_strn("x-requested-with"),
  l_literal_strn("x-http-method-override"),
  l_literal_strn("x-uidh"),
  l_literal_strn("x-csrf-token")
};

enum l_http_response_headers_enum {
  L_HTTP_H_ACCESS_CONTROL_ALLOW_ORIGIN,
  L_HTTP_H_ACCEPT_PATCH,
  L_HTTP_H_ACCEPT_RANGES,
  L_HTTP_H_ALLOW,
  L_HTTP_H_ALT_SVC,
  L_HTTP_H_LINK,
  L_HTTP_H_LOCATION,
  L_HTTP_H_P3P,
  L_HTTP_H_PROXY_AUTHENTICATE,
  L_HTTP_H_PUBLIC_KEY_PINS,
  L_HTTP_H_REFRESH,
  L_HTTP_H_RETRY_AFTER,
  L_HTTP_H_SERVER,
  L_HTTP_H_SET_COOKIE,
  L_HTTP_H_SET_COOKIE2,
  L_HTTP_H_STATUS,
  L_HTTP_H_STRICT_TRANSPORT_SECURITY,
  L_HTTP_H_TK,
  L_HTTP_H_VARY,
  L_HTTP_H_WWW_AUTHENTICATE,
  L_HTTP_H_X_FRAME_OPTIONS,
  L_HTTP_H_X_XSS_PROTECTION,
  L_HTTP_H_X_CONTENT_TYPE_OPTIONS,
  L_HTTP_H_X_POWERED_BY,
  L_HTTP_H_X_UA_COMPATIBLE,
  L_HTTP_LAST_RESPONSE_HEADER
};

static const l_strn l_http_response_headers[] = {
  l_literal_strn("access-control-allow-origin"),
  l_literal_strn("accept-patch"),
  l_literal_strn("accept-ranges"),
  l_literal_strn("allow"),
  l_literal_strn("alt-svc"),
  l_literal_strn("link"),
  l_literal_strn("location"),
  l_literal_strn("p3p"),
  l_literal_strn("proxy-authenticate"),
  l_literal_strn("public-key-pins"),
  l_literal_strn("refresh"),
  l_literal_strn("retry-after"),
  l_literal_strn("server"),
  l_literal_strn("set-cookie"),
  l_literal_strn("set-cookie2"),
  l_literal_strn("status"),
  l_literal_strn("strict-transport-security"),
  l_literal_strn("tk"),
  l_literal_strn("vary"),
  l_literal_strn("www-authenticate"),
  l_literal_strn("x-frame-options"),
  l_literal_strn("x-xss-protection"),
  l_literal_strn("x-content-type-options"),
  l_literal_strn("x-powered-by"),
  l_literal_strn("x-ua-compatible")
};

static l_stringmap l_method_map;
static l_stringmap l_httpver_map;
static l_stringmap l_common_map;
static l_stringmap l_request_map;
static l_stringmap l_response_map;

l_stringmap* l_http_method_map() {
  l_stringmap* map = &l_method_map;
  if (map->t) return map;
  *map = l_string_new_map(
      L_HTTP_METHOD_MAX_LEN,
      l_http_methods,
      L_NUM_OF_HTTP_METHODS,
      false);
  return map;
}

l_stringmap* l_http_version_map() {
  l_stringmap* map = &l_httpver_map;
  if (map->t) return map;
  *map = l_string_new_map(
      L_HTTP_VER_MAX_LEN,
      l_http_versions,
      L_NUM_OF_HTTP_VERS,
      false);
  return map;
}

l_stringmap* l_http_common_map() {
  l_stringmap* map = &l_common_map;
  if (map->t) return map;
  *map = l_string_new_map(
      L_HTTP_HEADER_MAX_LEN,
      l_http_common_headers,
      L_NUM_OF_COMMON_HEADERS,
      false);
  return map;
}

l_stringmap* l_http_request_map() {
  l_stringmap* map = &l_request_map;
  if (map->t) return map;
  *map = l_string_new_map(
      L_HTTP_HEADER_MAX_LEN,
      l_http_request_headers,
      L_NUM_OF_REQUEST_HEADERS,
      false);
  return map;
}

l_stringmap* l_http_response_map() {
  l_stringmap* map = &l_response_map;
  if (map->t) return map;
  *map = l_string_new_map(
      L_HTTP_HEADER_MAX_LEN,
      l_http_response_headers,
      L_NUM_OF_RESPONSE_HEADERS,
      false);
  return map;
}

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
不幸的是，HTTP/1.1规范中没有首部可以用来说明原始未编码的主体长度，这就客户端难以验证解码过程的完整性
  ** URL **
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
---　*/

#define L_HTTP_BACKLOG (32)
#define L_HTTP_RDREQ_STAGE (1)
#define L_HTTP_WRRES_STAGE (2)
#define L_HTTP_WRITE_STATUS (3)
#define L_HTTP_WRITE_HEADER (4)
#define L_HTTP_WRITE_BODY (5)

typedef struct {
  l_service head;
  l_string ip;
  l_ushort port;
  l_handle sock;
  int backlog;
  l_int tx_init_size;
  l_int rx_init_size;
  l_int rx_limit;
  l_string lua_module;
  int (*client_request_handler)(l_service*);
} l_http_server_service;

typedef struct {
  l_handle sock;
  l_int rx_limit;
  l_string* buf;
  l_int lstart; /* line start */
  l_int lnewline; /* newline pos */
  l_int lend; /* line end */
  l_int mstart; /* match start */
} l_http_read_common;

typedef struct {
  l_int start;
  l_int end;
} l_startend;

typedef struct {
  const l_byte* arg;
  const l_byte* mid;
  const l_byte* end;
} l_urlarg;

#define L_HTTP_URL_MAX_ARG_SIZE 32

typedef struct {
  l_service head;
  l_http_server_service* server;
  l_sockaddr remote;
  l_http_read_common comm;
  l_string rxbuf;
  int stage;
  l_byte method; /* method */
  l_byte httpver; /* http version */
  l_byte isfolder;
  l_byte urldepth;
  l_int suffix, suffixend; /* suffix pointer to '.' */
  l_int arg, argend; /* arg pointer to '?' */
  l_int argcount;
  l_urlarg arglist[L_HTTP_URL_MAX_ARG_SIZE];
  l_int url, uend; /* request url */
  l_int body, bodylen; /* body */
  l_umedit commasks;
  l_umedit reqmasks;
  l_startend comhead[32];
  l_startend reqhead[32];
  l_string txbuf;
  l_rune* txcur;
} l_http_server_receive_service;

#define L_HTTP_ESTIMATED_LINE_LENGTH (128)

static int l_http_read_length(l_http_read_common* comm, l_int len) {
  l_string* rxbuf = comm->buf;
  l_int count = 0, n = 0, status = 0;
  l_rune* buff_start = 0;

  comm->lstart = comm->lend; /* move forward to last end position */

ContinueCheckLength:
  if (l_string_size(rxbuf) - comm->lstart >= len) {
    comm->lend = comm->lstart + len;
    return 0;
  }

  if (count > 0 && n < count) return L_STATUS_WAITMORE; /* last time socket read indicates there is no more data */

  if (!l_string_ensure_capacity(rxbuf, comm->lstart + len + 1)) {
    return L_STATUS_ELIMIT;
  }

  buff_start = l_string_start(rxbuf);
  count = l_string_remain(rxbuf) - 1;
  n = l_socket_read(comm->sock, buff_start + l_string_size(rxbuf), count, &status);
  if (status < 0) return L_STATUS_EREAD;

  rxbuf->b->size += n; /* may read more data beyond newline */
  *(buff_start + rxbuf->b->size) = 0;
  goto ContinueCheckLength;
}

static int l_http_read_line(l_http_read_common* comm) {
  l_string* rxbuf = comm->buf;
  l_rune* buff_start = 0;
  const l_rune* match_end = 0;
  l_rune* last_match_start = 0;
  l_int count = 0, n = 0, status = 0;

  comm->lstart = comm->lend; /* move forward to last line end */

ContinueMatch:

  buff_start = l_string_start(rxbuf);
  match_end = l_string_match_until(l_string_newline_map(), l_strt_sft(buff_start, comm->mstart, l_string_size(rxbuf)), &last_match_start);
  if (match_end) { /* newline matched */
    comm->lnewline = last_match_start - buff_start;
    comm->lend = match_end - buff_start;
    comm->mstart = comm->lend;
    return 0;
  }

  comm->mstart = last_match_start - buff_start; /* next time match start here */
  if (count > 0 && n < count) return L_STATUS_WAITMORE; /* last time socket read indicates there is no more data */

  if (!l_string_ensure_remain(rxbuf, L_HTTP_ESTIMATED_LINE_LENGTH)) {
    return L_STATUS_ELIMIT;
  }

  buff_start = l_string_start(rxbuf);
  count = l_string_remain(rxbuf) - 1;
  n = l_socket_read(comm->sock, buff_start + l_string_size(rxbuf), count, &status);
  if (status < 0) return L_STATUS_EREAD;

  rxbuf->b->size += n; /* may read more data beyond newline */
  *(buff_start + rxbuf->b->size) = 0;
  goto ContinueMatch;
}

l_int l_http_match_header(l_stringmap* name, l_strt s, l_strt* value) {
  const l_rune* match_end = 0;
  l_int headid = 0;

  if ((match_end = l_string_match_ex(name, s, &headid, 0)) < s.start) {
    return L_STATUS_EMATCH;
  }

  s.start = match_end;
  if (!(match_end = l_string_skip_space_and_match_sub(l_literal_strt(":"), s))) {
    return L_STATUS_EMATCH;
  }

  s.start = match_end;
  if (!(value->start = l_string_trim_head(s))) return L_STATUS_EINVAL;
  value->end = s.end;
  return headid;
}

static int l_http_read_chunked_body(l_service* srvc) {
  /* Transfer-Encoding: chunked<CRLF>
     Trailer: Content-MD5<CRLF>
     <CRLF>
     f<CRLF>
     123456789abcdef<CRLF>
     2<CRLF>
     ab<CRLF>
     0<CRLF>
     Content-MD5:gjqei54p26tjisgj3p4utjgrj53<CRLF> */
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  (void)srvc;
  (void)ssrx;
  return 0;
}

static int l_http_read_body(l_service* srvc) {
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  l_http_read_common* comm = &ssrx->comm;
  int status = 0;

  if (ssrx->bodylen <= 0) return 0;

  if ((status = l_http_read_length(comm, ssrx->bodylen)) < 0) {
    return status;
  }

  if (status == L_STATUS_WAITMORE) {
    return l_service_yield(srvc, l_http_read_body);
  }

  return 0;
}

static int l_http_server_read_headers(l_service* srvc) {
  /* <name>: <value><crlf>
     ...
     <name>: <value><crlf>
     <crlf>
     <entity-body>  */
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  l_http_read_common* comm = &ssrx->comm;
  l_string* rxbuf = &ssrx->rxbuf;
  const l_rune* buff_start = 0;
  const l_rune* line_end = 0;
  const l_rune* match_end = 0;
  l_strt headval, cur_line;
  l_int headid = 0;
  int status = 0;

  for (; ;) {
    if ((status = l_http_read_line(comm)) < 0) {
      return status;
    }

    if (status == L_STATUS_WAITMORE) {
      return l_service_yield(srvc, l_http_server_read_headers);
    }

    buff_start = l_string_start(rxbuf);
    line_end = buff_start + comm->lnewline;
    cur_line = l_strt_e(buff_start + comm->lstart, line_end);

    match_end = l_string_match_repeat(l_string_space_map(), cur_line);
    if (match_end == line_end) {
      break; /* current line is empty line, headers ended here */
    }

    if ((headid = l_http_match_header(l_http_common_map(), cur_line, &headval)) >= 0) {
      ssrx->commasks |= (1 << headid);
      ssrx->comhead[headid].start = headval.start - buff_start;
      ssrx->comhead[headid].end = headval.end - buff_start;
      if (headid == L_HTTP_H_CONTENT_LENGTH) {
        ssrx->bodylen = l_string_parse_dec(headval);
      }
      continue;
    }

    if ((headid = l_http_match_header(l_http_request_map(), cur_line, &headval) >= 0)) {
      ssrx->reqmasks |= (1 << headid);
      ssrx->reqhead[headid].start = headval.start - buff_start;
      ssrx->reqhead[headid].end = headval.end - buff_start;
      continue;
    }

    l_logw_1("unknown or invalid header %strt", lstrt(&cur_line));
  }

  if (ssrx->method < L_HTTP_M_POST) {
    return 0; /* these method don't have body */
  }

  ssrx->body = comm->lend;

  if (ssrx->comhead[L_HTTP_H_TRANSFER_ENCODING].start) {
    return l_http_read_chunked_body(srvc);
  }

  return l_http_read_body(srvc);
}

static int l_http_server_read_request(l_service* srvc) {
  /* <method> <request-url> HTTP/<major>.<minor><crlf> #CarriageReturn(CR) 13 '\r' #LineFeed(LF) 10 '\n' */
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  l_http_read_common* comm = &ssrx->comm;
  l_string* rxbuf = &ssrx->rxbuf;
  const l_rune* buff_start = 0;
  const l_rune* line_end = 0;
  const l_rune* match_end = 0;
  l_rune* first_non_space_pos = 0;
  l_int strid = 0;
  int status = 0;

  if ((status = l_http_read_line(comm)) < 0) {
    return status;
  }

  if (status == L_STATUS_WAITMORE) {
    return l_service_yield(srvc, l_http_server_read_request);
  }

  /* a line is read, parse <method> first */
  buff_start = l_string_start(rxbuf);
  line_end = buff_start + comm->lnewline;
  match_end = l_string_skip_space_and_match(l_http_method_map(), l_strt_e(buff_start + comm->lstart, line_end), &strid, 0);
  if (!match_end) {
    l_logw_1("unsupported method %s", lp(buff_start + comm->lstart));
    return L_STATUS_EMATCH;
  }
  ssrx->method = strid;

  /* parse <request-url> */
  match_end = l_string_skip_space_and_match_until(l_string_blank_map(), l_strt_e(match_end, line_end), &first_non_space_pos);
  if (!match_end) return L_STATUS_EMATCH;
  ssrx->url = first_non_space_pos - buff_start;
  ssrx->uend = match_end - buff_start;

  /* parse <http-ver> */
  match_end = l_string_skip_space_and_match(l_http_version_map(), l_strt_e(match_end, line_end), &strid, 0);
  if (!match_end) {
    ssrx->httpver = L_HTTP_VER_0NN;
    return 0; /* v0.9请求报文只包括<method>和<request-url>，没有版本、头部、和主体；而响应报文只包含主体 */
  }
  ssrx->httpver = strid;

  /* start to parse headers */
  return l_http_server_read_headers(srvc);
}

#define L_HTTP_DISCARD_BUFFER_SIZE 1024

static void l_http_server_discard_data(l_http_server_receive_service* ssrx) {
  l_int n = 0;
  l_int status = 0;
  l_byte buffer[L_HTTP_DISCARD_BUFFER_SIZE+1];
ContinueRead:
  n = l_socket_read(ssrx->comm.sock, buffer, L_HTTP_DISCARD_BUFFER_SIZE, &status);
  if (status < 0) return; /* status >=0 success, <0 L_STATUS_ERROR */
  if (n == L_HTTP_DISCARD_BUFFER_SIZE) {
    goto ContinueRead;
  }
  return;
}

static int l_http_server_receive_service_proc(l_service* srvc, l_message* msg) {
  l_ioevent_message* ioev = (l_ioevent_message*)msg;
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  l_umedit masks = 0;
  int n = 0;

  if (msg->type != L_MESSAGE_IOEVENT) {
    l_loge_1("http_server_receive_service_proc msg %d", ld(msg->type));
    return L_STATUS_EINVAL;
  }

  masks = ioev->masks;
  if (masks & (L_IOEVENT_ERR | L_IOEVENT_HUP | L_IOEVENT_RDH)) {
    l_close_service(srvc);
    return 0;
  }

  if (ssrx->stage < L_HTTP_WRRES_STAGE) {
    int n = 0;
    if (!(masks & L_IOEVENT_READ)) {
      return 0;
    }
    if ((n = l_service_resume(srvc)) != 0) {
      if (n < 0) l_close_service(srvc);
      return 0;
    }
    ssrx->stage = L_HTTP_WRRES_STAGE;
    l_service_set_resume(srvc, ssrx->server->client_request_handler);
  } else {
    if (masks & L_IOEVENT_READ) {
      /* read event received at writing response stage, read it out and discard */
      l_loge_s("read and discard data at WRRES_STAGE");
      l_http_server_discard_data(ssrx);
    }
    if (!(masks & L_IOEVENT_WRITE)) {
      return 0;
    }
  }

  if ((n = l_service_resume(srvc)) != 0) {
    if (n < 0) l_close_service(srvc);
    return 0;
  }

  /* current request handled finished, prepare for next request or disconnect */
  ssrx->stage = L_HTTP_RDREQ_STAGE;
  if (ssrx->httpver <= L_HTTP_VER_10N) {
    l_close_service(srvc);
  }
  return 0;
}

static int l_http_server_service_proc(l_service* self, l_message* msg) {
  l_connind_message* connind = (l_connind_message*)msg;
  l_http_server_service* ss = (l_http_server_service*)self;
  l_http_server_receive_service* ssrx = 0;

  if (msg->type != L_MESSAGE_CONNIND || connind->fd == -1) {
    l_logw_2("http_server_service msg %d fd %d", ld(msg->type), ld(connind->fd));
    return L_STATUS_EINVAL;
  }

  /* TODO: implement destroy function */
  ssrx = (l_http_server_receive_service*)l_create_service(sizeof(l_http_server_receive_service), l_http_server_receive_service_proc, 0);
  ssrx->server = ss;
  ssrx->remote = connind->remote;
  ssrx->comm.sock = connind->fd;
  ssrx->comm.rx_limit = ss->rx_limit;
  ssrx->comm.buf = &ssrx->rxbuf;
  ssrx->rxbuf = l_thread_create_limited_string(self->thread, ss->rx_init_size, ss->rx_limit);
  ssrx->txbuf = l_thread_create_string(self->thread, ss->tx_init_size);

  l_service_set_resume(&ssrx->head, l_http_server_read_request);
  l_start_receiver_service(&ssrx->head, ssrx->comm.sock);
  return 0;
}

static int l_http_server_service_get_ip(void* self, l_strt str) {
  l_http_server_service* ss = (l_http_server_service*) self;
  if (l_strt_is_empty(&str)) return false;
  ss->ip = l_create_string_from(str);
  return true;
}

static int l_http_server_service_get_module(void* self, l_strt str) {
  l_http_server_service* ss = (l_http_server_service*) self;
  if (l_strt_is_empty(&str)) return false;
  ss->lua_module = l_create_string_from(str);
  return true;
}

#define L_HTTP_DEFAULT_PORT 80
#define L_HTTP_DEFAULT_TX_INIT_SIZE 1024
#define L_HTTP_DEFAULT_RX_INIT_SIZE 128
#define L_HTTP_DEFAULT_RX_LIMIT 1024*8

/* TODO: 如何释放特定service中的资源 */

int l_start_http_server(const void* http_conf_name, int (*client_request_handler)(l_service*)) {
  l_sockaddr sa;
  l_http_server_service* ss = 0;
  lua_State* L = l_get_luastate();

  /* TODO: implement destroy function */
  ss = (l_http_server_service*)l_create_service(sizeof(l_http_server_service), l_http_server_service_proc, 0);

  if (!http_conf_name) {
    http_conf_name = "http_default";
  }

  if (!lucy_strconf_n(L, l_http_server_service_get_ip, ss, 2, http_conf_name, "ip")) {
    ss->ip = l_create_string_from(l_literal_strt("0.0.0.0"));
  }

  if ((ss->port = lucy_intconf_n(L, 2, http_conf_name, "port")) == 0) {
    ss->port = L_HTTP_DEFAULT_PORT;
  }

  if ((ss->backlog = lucy_intconf_n(L, 2, http_conf_name, "backlog")) < L_HTTP_BACKLOG) {
    ss->backlog = L_HTTP_BACKLOG;
  }

  if ((ss->tx_init_size = lucy_intconf_n(L, 2, http_conf_name, "tx_init_size")) < L_HTTP_DEFAULT_TX_INIT_SIZE) {
    ss->tx_init_size = L_HTTP_DEFAULT_TX_INIT_SIZE;
  }

  if ((ss->rx_init_size = lucy_intconf_n(L, 2, http_conf_name, "rx_init_size")) < L_HTTP_DEFAULT_RX_INIT_SIZE) {
    ss->rx_init_size = L_HTTP_DEFAULT_RX_INIT_SIZE;
  }

  if ((ss->rx_limit = lucy_intconf_n(L, 2, http_conf_name, "rx_limit")) < L_HTTP_DEFAULT_RX_LIMIT) {
    ss->rx_limit = L_HTTP_DEFAULT_RX_LIMIT;
  }

  ss->client_request_handler = 0;
  if (!lucy_strconf_n(L, l_http_server_service_get_module, ss, 2, http_conf_name, "lua_module")) {
    ss->lua_module = l_empty_string();
    ss->client_request_handler = client_request_handler;
  }

  if (!l_sockaddr_init(&sa, l_string_strt(&ss->ip), ss->port)) {
    l_free_unstarted_service(&ss->head);
    return L_STATUS_ERROR;
  }

  ss->sock = l_socket_listen(&sa, ss->backlog);
  if (!l_socket_is_open(ss->sock)) {
    l_free_unstarted_service(&ss->head);
    return L_STATUS_ERROR;
  }

  l_logm_8("start http server: %s ip %s port %d backlog %d tx_init_size %d rx_init_size %d rx_limit %d module %s",
    ls(http_conf_name), ls(l_string_cstr(&ss->ip)), ld(ss->port), ld(ss->backlog), ld(ss->tx_init_size),
    ld(ss->rx_init_size), ld(ss->rx_limit), l_string_is_empty(&ss->lua_module) ? ls("n/a") : ls(l_string_cstr(&ss->lua_module)));

  l_start_listener_service(&ss->head, ss->sock);
  return 0;
}

int l_http_write_status(l_http_server_receive_service* ssrx, int code) {
  /* HTTP/1.1 200 OK<crlf>
     <name>: <value><crlf>
     ...
     <name>: <value><crlf>
     <crlf>
     <entity-body>   */
  l_string* txbuf = &ssrx->txbuf;
  int httpver = ssrx->httpver;

  if (httpver == L_HTTP_VER_0NN) {
    ssrx->stage = L_HTTP_WRITE_HEADER;
    return true;
  }

  if (ssrx->stage >= L_HTTP_WRITE_STATUS) {
    l_loge_1("status already written %d", ld(ssrx->stage));
    return false;
  }

  if (code < 0 || code >= L_HTTP_LAST_STATUS_CODE) {
    l_loge_1("invalid status code %d", ld(code));
    return false;
  }

  if (httpver > L_HTTP_VER_11N) {
    httpver = L_HTTP_VER_11N;
  }

  l_string_format_3(txbuf, "%strn %d %strn\r\n", lstrn(&l_http_versions[httpver]), ld(l_status[code].code), lstrn(&l_status[code].phrase));
  ssrx->stage = L_HTTP_WRITE_STATUS;
  return true;
}

#define L_HTTP_COMMON_HEADER_FLAG (1<<10)
#define L_HTTP_REQUEST_HEADER_FLAG (1<<11)
#define L_HTTP_RESPONSE_HEADER_FLAG (1<<12)
#define L_HTTP_HEADER_KEYINDEX_MASK (0x00ff)
#define L_HTTP_HEADER_TYPE_MASK (0xff00)

static int l_http_write_header_impl(l_http_server_receive_service* ssrx, int keyindex, l_strt value) {
  l_string* txbuf = &ssrx->txbuf;
  int type = 0;

  if (ssrx->httpver == L_HTTP_VER_0NN) {
    ssrx->stage = L_HTTP_WRITE_HEADER;
    return true;
  }

  if (ssrx->stage != L_HTTP_WRITE_STATUS && ssrx->stage != L_HTTP_WRITE_HEADER) {
    l_loge_1("cannot write header in stage %d", ld(ssrx->stage));
    return false;
  }

  ssrx->stage = L_HTTP_WRITE_HEADER;

  type = (keyindex & L_HTTP_HEADER_TYPE_MASK);
  keyindex = (keyindex & L_HTTP_HEADER_KEYINDEX_MASK);

  if ((type & L_HTTP_COMMON_HEADER_FLAG) && keyindex < L_HTTP_LAST_COMMON_HEADER) {
    l_string_format_2(txbuf, "%strn: %strt\r\n", lstrn(l_http_common_headers + keyindex), lstrt(&value));
    return true;
  }

  if ((type & L_HTTP_REQUEST_HEADER_FLAG) && keyindex < L_HTTP_LAST_REQUEST_HEADER) {
    l_string_format_2(txbuf, "%strn: %strt\r\n", lstrn(l_http_request_headers + keyindex), lstrt(&value));
    return true;
  }

  if ((type & L_HTTP_RESPONSE_HEADER_FLAG) && keyindex < L_HTTP_LAST_RESPONSE_HEADER) {
    l_string_format_2(txbuf, "%strn: %strt\r\n", lstrn(l_http_response_headers + keyindex), lstrt(&value));
    return true;
  }

  if (type == 0 && keyindex == 0) {
    l_string_format_1(txbuf, "%strt\r\n", lstrt(&value));
    return true;
  }

  l_loge_1("write invalid header of type %x", lx(type));
  return false;
}

int l_http_write_common_header(l_http_server_receive_service* ssrx, int keyindex, l_strt value) {
  return l_http_write_header_impl(ssrx, keyindex | L_HTTP_COMMON_HEADER_FLAG, value);
}

int l_http_write_request_header(l_http_server_receive_service* ssrx, int keyindex, l_strt value) {
  return l_http_write_header_impl(ssrx, keyindex | L_HTTP_REQUEST_HEADER_FLAG, value);
}

int l_http_write_response_header(l_http_server_receive_service* ssrx, int keyindex, l_strt value) {
  return l_http_write_header_impl(ssrx, keyindex | L_HTTP_RESPONSE_HEADER_FLAG, value);
}

int l_http_write_header(l_http_server_receive_service* ssrx, l_strt header) {
  return l_http_write_header_impl(ssrx, 0, header);
}

int l_http_write_body(l_http_server_receive_service* ssrx, l_strt body, int mime) {
  l_string* txbuf = &ssrx->txbuf;
  l_int len = body.end - body.start;

  if (ssrx->stage != L_HTTP_WRITE_STATUS && ssrx->stage != L_HTTP_WRITE_HEADER) {
    l_loge_1("cannot write body in stage %d", ld(ssrx->stage));
    return false;
  }

  if (mime < 0 || mime >= L_HTTP_LAST_MIME_TYPE) {
    l_loge_1("unsupported mime type %d", ld(mime));
    return false;
  }

  if (len <= 0) {
    l_logw_1("empty length %d", ld(len));
    return true;
  }

  if (ssrx->httpver > L_HTTP_VER_0NN) {
    l_string_format_2(txbuf, "Content-Type: %strn\r\nContent-Length: %d\r\n\r\n", lstrn(&l_mime_types[mime]), ld(len));
  }

  l_string_append(txbuf, body);
  ssrx->stage = L_HTTP_WRITE_BODY;
  return true;
}

int l_http_write_file(l_http_server_receive_service* ssrx, l_strt filename, int mime) {
  l_string* txbuf = &ssrx->txbuf;
  l_long filesize = 0;
  l_filestream file;
  l_int n = 0;

  if (ssrx->stage != L_HTTP_WRITE_STATUS && ssrx->stage != L_HTTP_WRITE_HEADER) {
    l_loge_1("cannot write body in stage %d", ld(ssrx->stage));
    return false;
  }

  if (mime < 0 || mime >= L_HTTP_LAST_MIME_TYPE) {
    l_loge_1("unsupported mime type %d", ld(mime));
    return false;
  }

  file = l_open_read_unbuffered(filename.start);
  if (file.stream == 0) return false;
  if (!(filesize = l_file_size(filename.start))) return false;
  if (!l_string_ensure_remain(txbuf, filesize + 1)) return false;

  if (ssrx->httpver > L_HTTP_VER_0NN) {
    l_string_format_2(txbuf, "Content-Type: %strn\r\nContent-Length: %d\r\n\r\n", lstrn(&l_mime_types[mime]), ld(filesize));
  }

  n = l_read_file(&file, l_string_end(txbuf), filesize);
  txbuf->b->size += n;
  ssrx->stage = L_HTTP_WRITE_BODY;

  if (n != filesize) {
    l_loge_2("read size %d doesn't match to file size %d", ld(n), ld(filesize));
    return false;
  }

  return true;
}

int l_http_write_css_file(l_http_server_receive_service* ssrx, l_strt filename) {
  return l_http_write_file(ssrx, filename, L_HTTP_MIME_CSS);
}

int l_http_write_js_file(l_http_server_receive_service* ssrx, l_strt filename) {
  return l_http_write_file(ssrx, filename, L_HTTP_MIME_JS);
}

int l_http_write_html_file(l_http_server_receive_service* ssrx, l_strt filename) {
  return l_http_write_file(ssrx, filename, L_HTTP_MIME_HTML);
}

int l_http_write_plain_file(l_http_server_receive_service* ssrx, l_strt filename) {
  return l_http_write_file(ssrx, filename, L_HTTP_MIME_PLAIN);
}

static int l_http_send_response_impl(l_service* srvc) {
  l_http_server_receive_service* ssrx = (l_http_server_receive_service*)srvc;
  l_rune* txcur = ssrx->txcur;
  l_rune* txend = l_string_end(&ssrx->txbuf);
  l_int count = txend - txcur, n = 0;
  l_int status = 0;

  if ((n = l_socket_write(ssrx->comm.sock, txcur, count, &status)) < 0) {
    return L_STATUS_EWRITE;
  }

  ssrx->txcur += n;

  if (n < count) {
    return l_service_yield(&ssrx->head, l_http_send_response_impl);
  }

  return 0;
}

int l_http_send_response(l_http_server_receive_service* ssrx) {
  l_string* txbuf = &ssrx->txbuf;

  if (l_string_is_empty(txbuf) || ssrx->stage < L_HTTP_WRITE_STATUS) {
    l_loge_1("no content need to send %d", ld(ssrx->stage));
    return false;
  }

  if (ssrx->httpver != L_HTTP_VER_0NN && ssrx->stage != L_HTTP_WRITE_BODY) {
    l_string_append(txbuf, l_literal_strt("Content-Length: 0\r\n\r\n"));
  }

  ssrx->txcur = l_string_start(txbuf);
  return l_http_send_response_impl(&ssrx->head);
}

#define L_HTTP_PATH_MAX_DEPTH 0xff

int l_http_check_url_path_sep(l_strt name) {
  if (l_strt_is_empty(&name)) return false;
  for (; name.start < name.end; ++name.start) { /* 0~9 a~z A~Z _ - */
    if (l_check_is_alphanum_underscore_hyphen(*name.start)) continue;
    return false;
  }
  return true;
}

int l_http_check_url_add_arg(l_http_server_receive_service* ssrx, const l_byte* arg, const l_byte* mid, const l_byte* end) {
  l_urlarg* curarg = 0;
  if (ssrx->argcount == L_HTTP_URL_MAX_ARG_SIZE) return false;
  curarg = ssrx->arglist + (ssrx->argcount++);
  curarg->arg = arg;
  curarg->mid = mid;
  curarg->end = end;
  return true;
}

int l_http_check_url_argument(l_http_server_receive_service* ssrx, l_strt arg) {
  /* golden color=blue item=  =value = */
  const l_byte* start = arg.start;
  const l_byte* pcur = arg.start;
  const l_byte* middle = 0;

  if (l_strt_is_empty(&arg)) {
    l_loge_s("current url arg is empty");
    return false;
  }

  for (; pcur < arg.end; ++pcur) {
    if (l_check_is_alphanum_underscore_hyphen(*pcur)) continue;
    break;
  }

  if (pcur == arg.end) { /* golden */
    return l_http_check_url_add_arg(ssrx, start, pcur, pcur);
  }

  if (pcur == start || *pcur != '=') { /* "=value" "=" */
    l_loge_1("argument name is empty or invalid %strt", lstrt(&arg));
    return false;
  }

  middle = pcur++; /* '=' */
  if (pcur == arg.end) {
    return l_http_check_url_add_arg(ssrx, start, middle, middle);
  }

  if (l_strt_equal(l_strt_e(start, middle), l_literal_strt("jump"))) {
    return l_http_check_url_add_arg(ssrx, start, middle, arg.end);
  }

  for (; pcur < arg.end; ++pcur) {
    if (l_check_is_alphanum_underscore_hyphen(*pcur)) continue;
    l_loge_1("argument value is invalid %strt", lstrt(&arg));
    return false;
  }

  return l_http_check_url_add_arg(ssrx, start, middle, arg.end);
}

int l_http_check_url_last_part(l_http_server_receive_service* ssrx, l_strt sep) {
  /* last part: /"file" or /"file.suffix" or /"filename?param=value&color=blue" */
  const l_byte* start = sep.start;
  const l_byte* pcur = sep.start;

  if (l_strt_is_empty(&sep)) return false;

  for (; pcur < sep.end; ++pcur) {
    if (l_check_is_alphanum_underscore_hyphen(*pcur)) continue;
    break;
  }

  if (pcur == sep.end) return true;

  if (*pcur == '.') {
    if (ssrx->suffix != 0) return false; /* cannot have two '.', such as "file.sub.fix" */

    if (pcur == sep.start || pcur + 1 == sep.end) {
      return false; /* "/.sub" "/file." is not allowed */
    }

    ssrx->suffix = pcur - l_string_cstr(&ssrx->rxbuf);

    start = ++pcur;
    for (; pcur < sep.end; ++pcur) {
      if (l_check_is_alphanum_underscore_hyphen(*pcur)) continue;
      break;
    }

    if (pcur == start) {
      return false; /* "file.#" is not allowed, # is not an alphanum_underscore_hyphen */
    }

    ssrx->suffixend = pcur - l_string_cstr(&ssrx->rxbuf);
    if (pcur == sep.end) return true;
  }

  if (*pcur != '?') {
    l_loge_1("invalid url last part %strt", lstrt(&sep));
    return false;
  }

  if (pcur + 1 == sep.end) { /* empty arg - no chars after '?' */
    return true;
  }

  ssrx->arg = pcur - l_string_cstr(&ssrx->rxbuf);

  start = ++pcur;
  for (; pcur < sep.end; ++pcur) {
    if (*pcur != '&') continue;
    if (!l_http_check_url_argument(ssrx, l_strt_e(start, pcur))) {
      return false;
    }
    start = pcur + 1;
  }

  if (pcur == start) { /* empty arg - no chars after '&' */
    return true;
  }

  return l_http_check_url_argument(ssrx, l_strt_e(start, pcur));
}

int l_http_handle_url_path_sep(l_http_server_receive_service* ssrx, int (*func)(void* self, l_strt sep, int depth), void* self) {
  int depth = 0;
  const l_byte* bufstart = l_string_cstr(&ssrx->rxbuf);
  l_strt url = l_strt_e(bufstart + ssrx->url, bufstart + ssrx->uend);
  const l_byte* pcur = url.start;
  l_strt sep = l_empty_strt();

  ssrx->isfolder = false;
  ssrx->urldepth = -1;
  ssrx->suffix = ssrx->suffixend = 0;
  ssrx->arg = ssrx->argend = 0;
  ssrx->argcount = 0;

  if (url.start >= url.end || *url.start != '/') {
    return false;
  }

  sep.start = ++pcur;
  for (; pcur < url.end; ++pcur) {
    if (*pcur != '/') continue;
    sep.end = pcur;
    if (!l_http_check_url_path_sep(sep) || !func(self, sep, ++depth)) {
      return false;
    }
    sep.start = pcur + 1;
  }

  /* last part: /"" or /"file" or /"file.suffix" or /"filename?param=value&color=blue" */
  ssrx->urldepth = depth;
  if (sep.start == pcur) {
    ssrx->isfolder = true;
    return true;
  }

  sep.end = pcur;
  if (!l_http_check_url_last_part(ssrx, sep)) {
    ssrx->urldepth = -1;
    return false;
  }

  return true;
}

void l_http_server_handle_get_method(l_http_server_receive_service* ssrx) {
  /** URL **
   "<scheme>://<user>:<password>@<host>:<port>/<path>;<params>?<query>#<frag>"
   /hammers;sale=0;color=red?item=1&size=m
   /hammers;sale=false/index.html;graphics=true
   /check.cgi?item=12731&color=blue&size=large
   /tools.html#drills

   *
   /
   /file     (file or path) ??? just parse as file
   /file?item=value
   /file?item=value&color=blue
   /path/
   /path/file
   /path/file?item=value
   /path/file?item=value&color=blue

   只能包含 /?=_-0~9a~zA~Z 除非最后一个参数是 jump=xxxxx

   URL编码机制，为了使用不安全字符，使用%和两个两个十六进制字符表示，例如空格可使用%20表示，根据浏览器的不同组合成的字符可能使用不同的编码
   保留字符有 % / . .. # ? ; : $ + @ & =
   受限字符有 {} | \ ^ ~ [ ] ' < > "　以及不可打印字符 0x00~0x1F >=0x7F，而且不能使用空格，通常使用+来替代空格
   0~9A~Za~z_- */
   (void)ssrx;
}

int l_http_server_handle_request(l_http_server_receive_service* ssrx) {
  switch (ssrx->method) {
  case L_HTTP_M_GET:
  case L_HTTP_M_HEAD:
    l_http_server_handle_get_method(ssrx);
    break;
  case L_HTTP_M_OPTIONS:
    l_http_write_status(ssrx, L_HTTP_200_OK);
    l_http_write_response_header(ssrx, L_HTTP_H_ALLOW, l_literal_strt("GET, HEAD, OPTIONS, POST"));
    break;
  case L_HTTP_M_POST:
  default:
    l_http_write_status(ssrx, L_HTTP_405_METHOD_NOT_ALLOWED);
    break;
  }
  return l_http_send_response(ssrx);
}
