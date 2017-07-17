#include "osi/linuxpref.h"
#include "osi/platsock.h"
#include "lucycore.h"

#define L_SOCKET_BACKLOG  (32)
#define L_SOCKET_IPSTRLEN (48)

int l_sockaddr_init(l_sockaddr* self, l_strt ip, l_ushort port) {
  /** inet_pton htons/l ntohs/l **
  #include <arpa/inet.h> // some systems require <netinet/in.h> instead of <arpa/inet.h>
  int inet_pton(int family, const char* str, void* dest); // convert ipv4 and ipv6 from text to binary
  uint32_t htonl(uint32_t hostlong);  // host to net (32-bit)
  uint16_t htons(uint16_t hostshort); // host to net (16-bit)
  uint32_t ntohl(uint32_t netlong);   // net to host (32-bit)
  uint16_t ntohs(uint16_t netshort);  // net to host (16-bit)
  AF_INET - src contains an ipv4 network address in dotted-decimal format,
  "ddd.ddd.ddd.ddd", where ddd is a decimal number of up to three digits in
  the range 0 to 255. The address is convered to a struct in_addr and copied
  to dest, which must be sizeof(struct in_addr) long.
  AF_INET6 - src contains an ipv6 network address. The address is cnverted to
  and struct in6_addr and copied to dest, which must be sizeof(struct in6_addr)
  long. The allowed formats for ipv6 addresses are: x:x:x:x:x:x:x:x, each x can
  be up to 4 hex digits; A series of contiguous zero values in the preferred
  format can be abbreviated to ::, only one instance of :: can occur in an
  address. For example, the loopback address 0:0:0:0:0:0:0:1 can be abbreviated
  as ::1, the wildcard address, consisting of all zeros, can be written as ::;
  An alternate format is useful for expressing ipv4-mapped ipv6 address. This
  form is written as x:x:x:x:x:x:d.d.d.d, an example of such an address is
  ::FFFF:204.152.189.116.
  inet_pton() returns 1 on success, 0 is returned if src does not contain a
  character string representing a valid network in the specified address family.
  If family does not contain a valid address family, -1 is returned and errno is
  set to EAFNOSUPPORT. */
  int n = 0;
  llsockaddr* sa = (llsockaddr*)self;
  if (l_strt_contain(ip, ':')) {
    if ((n = inet_pton(AF_INET6, (const char*)ip.start, &(sa->addr.in6.sin6_addr))) != 1) {
      goto errorlabel;
    }
    sa->len = sizeof(struct sockaddr_in6);
    sa->addr.in6.sin6_family = AF_INET6;
    sa->addr.in6.sin6_port = htons(port);
    return true;
  }
  if ((n = inet_pton(AF_INET, (const char*)ip.start, &(sa->addr.in.sin_addr))) != 1) {
    goto errorlabel;
  }
  sa->len = sizeof(struct sockaddr_in);
  sa->addr.in.sin_family = AF_INET;
  sa->addr.in.sin_port = htons(port);
  return true;
errorlabel:
  if (n == 0) {
    l_loge_s("inet_pton invalid ip");
  } else {
    l_loge_1("inet_pton %s", lserror(errno));
  }
  l_zero_l(sa, sizeof(llsockaddr));
  return false;
}

l_ushort l_sockaddr_port(l_sockaddr* self) {
  llsockaddr* sa = (llsockaddr*)self;
  return ntohs(sa->addr.in.sin_port);
}

int l_sockaddr_ipstring(l_sockaddr* self, l_string* out) {
  /** inet_ntop - convert ipv4 and ipv6 addresses from binary to text form **
  #include <arpa/inet.h>
  const char* inet_ntop(int family, const void* src, char* dest, socklen_t size);
  This function converts the network address src in the family into a character
  string. The resulting is copied to the buffer pointed to by dest, which must
  be a non-null pointer. The caller specifies the number of bytes available in
  this buffer in size.
  AF_INET - src points to a struct in_addr (in network byte order) which is
  converted to an ipv4 network address in the dotted-decimal format, "ddd.ddd.ddd.ddd".
  The buffer dest must be at least INET_ADDRSTRLEN bytes long.
  AF_INET6 - src points to a struct in6_addr (in network byte order) which is
  converted to a representation of this address in the most appropriate ipv6
  network address format for this address. The buffer dst must be at least
  INET6_ADDRSTRLEN bytes long.
  On success, inet_ntop() returns a non-null pointer to dst. NULL is returned if
  there was an error, with errno set to indicate the error. */
  llsockaddr* sa = (llsockaddr*)self;
  if (sa->addr.sa.sa_family == AF_INET) {
    char ipstrbuf[INET_ADDRSTRLEN+1];
    if (inet_ntop(AF_INET, &(sa->addr.in.sin_addr), ipstrbuf, INET_ADDRSTRLEN) == 0) {
      l_loge_1("inet_ntop %s", lserror(errno));
      l_string_clear(out);
      return false;
    }
    l_string_set(out, l_strt_c(ipstrbuf));
    return true;
  }
  if (sa->addr.sa.sa_family == AF_INET6) {
    char ipv6strbuf[INET6_ADDRSTRLEN+1];
    if (inet_ntop(AF_INET6, &(sa->addr.in6.sin6_addr), ipv6strbuf, INET6_ADDRSTRLEN) == 0) {
      l_loge_1("inet_ntop %s", lserror(errno));
      l_string_clear(out);
      return false;
    }
    l_string_set(out, l_strt_c(ipv6strbuf));
    return true;
  }
  l_loge_s("invalid address family");
  l_string_clear(out);
  return false;
}

static int llsetnonblock(l_handle fd) {
  /** fcntl - manipulate file descriptor **
  #include <unistd.h>
  #include <fcntl.h>
  int fcntl(int fd, int cmd, ...);
  On error, this function returns -1, and errno is set appropriately.
  File status flags: each open fd has certain associated status flags,
  initialized by open(2) and possibly modified by fcntl(). duplicated
  fds refer to the same open fd, and thus share the same file status
  flags. The file status flags and their semantics are described in open(2).
  F_GETFL (void) - return (as the function result) the file access mode
  and the file status flags, arg is ignored.
  F_SETFL (int) - set the file status flags to the value specified by arg.
  File access mode (O_RDONLY, O_WRONLY, O_RDWR) and file creation flags (i.e.,
  O_CREATE,O_EXCL, O_NOCTTY, O_TRUNC) in arg are ignored. On linux, this
  command can change only the O_APPEND, O_ASYNC, O_DIRECT, O_NOATIME, and
  O_NONBLOCK flags. It is not possible to change the O_DSYNC and O_SYNC flags. */
  int flag = 0;
  if ((flag = fcntl(fd, F_GETFL)) == -1) {
    l_loge_1("fcntl getfl %s", lserror(errno));
    return false;
  }
  if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) {
    l_loge_1("fcntl setfl %s", lserror(errno));
    return false;
  }
  return true;
}

static int llsocketcreate(int domain, int type, int protocol, l_handle* out) {
  /** socket - create an endpoint for communication **
  #include <sys/types.h>
  #include <sys/socket.h>
  int socket(int domain, int type, int protocol);
  It returns a file descriptor that refers to the communication endpoint.
  The domain specifies a communication domain, it selects the protocol
  family which will be used for communication.
  AF_UNIX, AF_LOCAL - local communication - man unix(7)
  AF_INET - ipv4 internet protocols - man ip(7)
  AF_INET6 - ipv6 internet protocols - man ipv6(7)
  AF_NETLINK - kernel user interface device - man netlink(7)
  AF_APPLETALK - AppleTalk - man ddp(7), ...
  ---
  The socket 'type' specifies the communication semantics.
  SOCK_STREAM - provides sequenced, reliable, two-way, connection-based
  byte streams. an out-of-band data transmission mechanism may be supported.
  SOCK_DGRAM - supports datagrams (connectionless, unreliable messages
  of a fixed maximum length).
  SOCK_SEQPACKET - provides a sequenced, reliable, two-way connection-based
  data transmission path for datagrams of fixed maximum length; a consumer
  is required to read an entire packet with each input system call.
  SOCK_RAW - provides raw network protocol access.
  SOCK_RDM - provides a reliable datagram layer that does not guarantee
  ordering.
  SOCK_PACKET - obsolete and should not be used. see packet(7).
  Since Linux 2.6.27, the 'type' argument serves a second purpose: in
  addition to specifying a socket type, it may include the bitwise OR
  of any of the following values, to modify the behavior of socket().
  SOCK_NONBLOCK - set the O_NONBLOCK file status flag on the new open fd.
  using this flag saves extra calls to fcntl(2) to achieve the same result.
  SOCK_CLOEXEC - set the close-on-exec (FD_CLOEXEC) flag on the new fd.
  see the description of the O_CLOEXEC flag in open(2) for reasons.
  The SOCK_NONBLOCK and SOCK_CLOEXEC flags are Linux-specific.
  ---
  The 'protocol' specifies a particular protocol to be used with the socket.
  Normally only a single protocol exists to support a particular socket type
  within a given protocol family, in which case protocol can be specified
  as 0. However, it is possible that many protocols may exist, in which case
  a particular protocol must be specified in this manner. protocol number is
  defined in http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml,
  and also can be seen in '/etc/protocols'.
  ---
  The communications protocols which implement a SOCK_STREAM ensure that data
  is not lost and duplicated. If a piece of data for which the peer protocol
  has buffer space cannot be successfully thransmitted within a reasonable length
  of time, then the connection is considered to be dead. When SO_KEEPALIVE is
  enabled on the socket the protocol checks in a protocol-specific manner if the
  other end is still alive. A SIGPIPE signal is raised if a process sends or
  receives on a broken stream; this causes naive processes, which do not handle
  the signal, to exit. SOCK_SEQPACKET sockets employ the same system calls as
  SOCK_STREAM sockets. The only difference is that read(2) calls will return
  only the amount of data requested, and any data remaining in the arriving packet
  will be discarded. Also all message boundaries in incoming datagrams are preserved.
  SOCK_DGRAM and SOCK_RAW sockets allow sending of datagrams to correspondents
  named in sendto(2) calls. Datagrams are generally received with recvfrom(2), which
  returns the next datagram along with the address of its sender.
  ---
  An fcntl(2) F_SETOWN operation can be used to specify a process or process
  group to receive a SIGURG signal when the out-of-band data arrives or SIGPIPE
  signal when a SOCK_STREAM connection breaks unexpectedly. This operation may
  also be used to set the process or process group that receives the I/O and
  asynchronous notification of I/O events via SIGIO. Using F_SETOWN is equivalent
  to an ioctl(2) call with the FIOSETOWN or SIOCSPGRP argument.
  When the network signals an error condition to the protocol module (e.g., using
  an ICMP message for IP) the pending error flag is set for the socket. The next
  operation on this socket will return the error code of the pending error. For
  some protocols it is possible to enable a per-socket error queue to retrieve
  detailed information about the error; see IP_RECVERR in ip(7).
  ---
  On success, a fd for the new socket is returned. On error, -1 is returned,
  and errno is set appropriately.
  EACCES - permission to create a socket of the specified type/protocol is denied
  EAFNOSUPPORT - doesn't support the specified address family.
  EINVAL - unknown protocol, or protocol family not available, or invalid falgs in type.
  EMFILE - the per-process limit on the number of open fds has been reached.
  ENFILE - the system-wide limit on the total number of open files has been reached.
  ENOBUFS or ENOMEM - insufficient memory is available.
  EPROTONOSUPPORT - the protocol type or the specified protocol is not supported.
  Other errors may be generated by the underlying protocol modules. */
  if ((*out = socket(domain, type, protocol)) == -1) {
    l_loge_1("socket %s", lserror(errno));
    return false;
  }
  llsetnonblock(*out);
  return true;
}

int l_socket_is_open(l_handle sock) {
  return (sock != -1);
}

void l_socket_close(l_handle sock) {
  /** close - close a file descriptor **
  #include <unistd.h>
  int close (int fd);
  It closes a fd, so that it no longer refers to any file and may be reused.
  Any record locks held on the file it was associated with, and owned by the
  process, are removed (regardless of the fd that was used to obtain the lock).
  If fd is the last fd referring to the underlying open fd, the resources
  associated with the open fd are freed.
  It returns zero on success. On error, -1 is returned, and errno is set.
  EBADF - fd isn't a valid open fd.
  EINTR - the close() call was interrupted by a signal.
  EIO - an I/O error occurred.
  close() should not be retried after an error. Retrying the close() after a
  failure return is the wrong thing to do, since this may cause a reused fd
  from another thread to be closed. This can occur because the Linux kernel
  always releases the fd early in the close operation, freeing it for resue.
  关闭一个TCP套接字的默认行为是把该套接字标记为已关闭，然后立即返回到调用进程。
  该套接字不能再由调用进程使用，也就是说不能再作为read或write的第一个参数。然而
  TCP将尝试发送已排队等待发送到对端的任何数据，发送完毕后发生正常的TCP断连操作。
  我们将在7.5节介绍的SO_LINGER套接字选项可以用来改变TCP套接字的这种默认行为。我
  们将在那里介绍TCP应用进程必须怎么做才能确信对端应用进程已收到所有排队数据。
  关闭套接字只是导致相应描述符的引用计数减1，如果引用计数仍大于0，这个close调用
  并不会引发TCP断连流程。如果我们确实想在某个TCP连接上发送一个FIN，可以调用
  shutdown函数以代替close()，我们将在6.5节阐述这么做的动机。*/
  if (sock == -1) return;
  if (close(sock) != 0) {
    l_loge_1("close socket %d", lserror(errno));
  }
}

void l_socket_shutdown(l_handle sock, l_rune r_w_a) {
  /** shutdown - shut down part of a full-duplex connection **
  #include <sys/socket.h>
  int shutdown(int sockfd, int how);
  This call causes all or part of a full-duplex connection on the socket
  associated with sockfd to be shut down. If how is SHUT_RD, further
  receptions will be disallowed. If how is SHUT_WR, further transmissions
  will be disallowed. If how is SHUT_RDWR, further receptions and
  transmissions will be disallowed.
  On success, zero is returned. On error, -1 is returned, and errno is set.
  EBADF - sockfd is not a valid fd.
  EINVAL - an invalid value was specified in how.
  ENOTCONN - the specified socket is not connected.
  ENOTSOCK - the sockfd is not refer to a socket. */
  int flag = 0;
  switch (r_w_a) {
  case 'r': case 'R': flag = SHUT_RD; break;
  case 'w': case 'W': flag = SHUT_WR; break;
  case 'a': case 'A': flag = SHUT_RDWR; break;
  default:
    l_loge_s("shutdown invalid argument");
    return;
  }
  if (shutdown(sock, flag) != 0) {
    l_loge_1("shutdown %s", lserror(errno));
  }
}

l_sockaddr l_socket_getlocaladdr(l_handle sock) {
  /** getsockname **
  #include <sys/socket.h>
  int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
  returns the current address the sock is bound. the addrlen should be
  initialized to indicate the amount of space in bytes pointed to by addr.
  on return it contains the actual size of the socket address.
  the returned address is truncated if the buffer provided is too small; in
  this case, addrlen will return a value greater than was supplied to the call. */
  l_sockaddr addr;
  llsockaddr* sa = (llsockaddr*)&addr;
  socklen_t providedlen = sizeof(ll_sock_addr);
  sa->len =  providedlen;
  if (getsockname(sock, &(sa->addr.sa), &(sa->len)) != 0) {
    l_loge_1("getsockname %s", lserror(errno));
    sa->len = 0;
  }
  if (sa->len > providedlen) {
    l_loge_s("getsockname truncated");
    sa->len = providedlen;
  }
  return addr;
}

static int llsocketbind(l_handle sock, const l_sockaddr* addr) {
  /** bind - bind a address to a socket **
  #include <sys/types.h>
  #include <sys/socket.h>
  int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
  It is normally necessary to assign a local address using bind() before
  SOCK_STREAM socket may receive connections. The actual structure passed
  to the addr argument will depend on the address family. The only purpose
  of struct sockaddr is to do the cast.
  On success, zero is returned. On error, -1 is returned and errno is set.
  EACCES - the address is portected, and the user is not the superuser.
  EADDRINUSE - the given address is already in use.
  EBADF - sockfd is not a valid fd.
  EINVAL - the socket is already bound to an address, or addrlen is wrong
  addr is not a valid address for this socket's domain.
  ENOTSOCK - the sockfd is not refer to a socket.
  And there are other errors specific to UNIX domain (AF_UNIX) sockets.
  如果一个TCP客户或服务器未曾调用bind绑定一个端口，当使用connect或listen
  时，内核会为相应的套接字选择一个临时端口。当然这对于TCP服务器是罕见的，
  服务器通常会绑定一个众所周知的端口。一个例外是RPC服务器，它们通常由内核
  为它们的监听套接字选择一个临时端口，然后该端口会通过RPC端口映射器进行注
  册。客户在connect这些服务器之前，必须与端口映射器联系以获取它们的临时
  端口。如果由内核选择临时端口，需要调用函数getsockname()来获取。
  进程可以把一个特定的IP地址绑定到它的套接字上，这个IP地址必须属于其所在
  主机的网络接口之一。对于TCP客户，这相当于给在该套接字上发送的IP数据报
  指派了源IP地址；但客户端通常不绑定IP地址，当连接套接字时，内核根据所用
  外出网络接口来选择源IP地址，而所用外出接口则取决于到达服务器所需路径（见
  TCPv2第737页）。对于TCP服务器，这就限定该套接字只接收那些目的地址为这个
  IP地址的客户连接；如果服务器不绑定IP地址，内核就把客户发送的SYN的目的IP
  地址作为服务器的源IP地址（TCPv2第943页）。
  我们必须区分分组的的到达地址和该分组的目的IP地址，一般大多数实现都使用弱
  端系统模型，这意味着一个分组只要其目的IP地址能够匹配目的主机的某个网络接
  口就行，这个目的地址不一定是它的到达地址。绑定非通配IP地址可以根据数据包
  使用的目的IP地址来确定接下来发送数据包的源IP地址，而对到达接口不做限制，
  除非主机采用强端系统模型。
  如果绑定的IP地址是通配地址，就是告诉系统要是系统是多缩主机（有多个网络接口），
  我们将接受目的地址为任何本地接口的连接。如果绑定的端口为0则系统会自动分配
  一个临时端口使用。如果自行设置一个临时端口使用，它应该比1023大（不应该是保留
  端口），比5000大（以免与许多源自Berkeley的实现分配临时端口的范围冲突），比
  49152小（以免与临时端口号的“正确”范围冲突）。
  从bind返回的一个常见错误是EADDRINUSE，到7.5节讨论SO_REUSEADDR和SO_REUSEPORT
  这两个套接字选项时在详细讨论。*/
  llsockaddr* sa = (llsockaddr*)addr;
  if (sa->len == 0) {
    l_loge_s("bind invalid address");
    return false;
  }
  if (bind(sock, &(sa->addr.sa), sa->len) != 0) {
    l_loge_1("bind %s", lserror(errno));
    return false;
  }
  return true;
}

static int llsocketlisten(l_handle sock, int backlog) {
  /** listen - listen for connections on a socket **
  #include <sys/types.h>
  #include <sys/socket.h>
  int listen(int sockfd, int backlog);
  It marks the socket referred to by sockfd as a passive socket,
  that is, as a socket that will be used to accept incoming
  connection requests using accept. The sockfd is a fd that refers
  to a socket of type SOCK_STREAM or SOCK_SEQPACKET.
  The backlog defines the maximum length to which the queue of
  pending connections for sockfd may grow. If a connection request
  arrives when the queue is full, the client may receive an error
  with an indication of ECONNREFUSED or, if the underlying protocol
  supports retransmission, the request may be ignored so that a
  later reattempt at connection succeeds.
  The behavior of the backlog on TCP sockets changed with Linux 2.2.
  Now it specifies the queue length for completely established sockets
  waiting to be accepted, instead of the number of incomplete connection
  requests. The maximum length of the queue for incomplete sockets can
  be set using /proc/sys/net/ipv4/tcp_max_syn_backlog. When syncookies
  are enabled there is no logical maximum length and this setting is
  ignored. See tcp(7) for more information.
  If the backlog is greater than the value in /proc/sys/net/core/somaxconn,
  then it is silently truncated to that value; the default value in this
  file is 128. In kernels before 2.4.25, this limit was a hard coded value,
  SOMAXCONN, with the value 128.
  EADDRINUSE - Another socket is already listening on the same port.
  EBADF - the sockfd is not a valid fd.
  ENOTSOCK - the sockfd is not refer to a socket.
  EOPNOTSUPP - the socket is not of a type that supports the listen() op.
  当socket函数创建一个套接字时，它被假设为一个主动套接字，即调用connect发起
  连接的客户套接字。listen函数把一个为连接的套接字转换成一个被动套接字，指示
  内核应该接受指向该套接字的连接请求。根据TCP状态图，调用listen后导致套接字
  从CLOSED状态转换成LISTEN状态。
  内核为任何给定的监听套接字维护了两个队列。一个是未完成连接队列(incomplete
  connection queue），该队列里的套接字处于SYN_RCVD状态，表示已经收到了客户发
  的SYN但是TCP三次握手还没有完成；一个是已完成队列（completed connection
  queue），其中的套接字都处于ESTABLISHED状态，TCP三次握手的连接过程已成功完成。
  当来自客户SYN到达时，TCP在未完成连接队列中创建一个新项，然后发送SYN和ACK到
  客户端。该新建项会保持在队列中直到客户端ACK到来或等待超时（源自Berkeley的
  实现这个超时值为75s）。如果成功收到客户的ACK，该项会从未完成队列移到已完成
  队列的队尾。当进程调用accept时，已完成队列中的对头项将返回给进程，或者如果
  该队列为空，那么进程将被投入睡眠直到TCP在该队列放入一项才唤醒。
  backlog的含义从未有过正式的定义，4.2BSD宣称它的定义是由未处理连接构成的队列
  可能增长到的最大长度。该定义并未解释未处理连接是处于SYNC_RCVD状态的连接，还
  是尚未由进程接受的处于ESTABLISHED状态的连接，亦或两者皆可。源自Berkeley的实现
  给backlog增设了一个模糊因子，它乘以1.5得到未处理队列最大长度。不要把backlog
  设为0，因为不同的实现对此有不同的解释，如果你不想让任何客户连接到你的监听套
  接字上，那就关闭该监听套接字。指定一个比内核能够支持的值大的backlog是可以接受
  的，内核会把所指定的偏大值截断成自己支持的最大值而不返回错误。
  当一个客户SYN到达时，或这些队列是满的，TCP就忽略该SYN，不应该发送RST。因为
  这种情况是暂时的，客户TCP将重发SYN。要是服务器TCP立即响应一个RST，客户的
  connect就会立即返回一个错误，强制客户应用程序处理这种状况而不是让TCP的重传机制
  来处理。另外，客户无法区分响应SYN的RST究竟意味着“该端口没有服务器在监听”，还是
  “该端口有服务器在监听，不过它的队列已经满了”。
  在三次握手之后，但在服务器accept之前到达的数据应该有服务器TCP排队，最大数据量
  为相应已连接套接字的接收缓冲区大小。*/
  if (listen(sock, backlog) != 0) {
    l_loge_1("listen %s", lserror(errno));
    return false;
  }
  return true;
}

l_handle l_socket_listen(const l_sockaddr* addr, int backlog) {
  l_handle sock = -1;
  const llsockaddr* sa = (const llsockaddr*)addr;
  int domain = (addr == 0 ? AF_INET : sa->addr.sa.sa_family);
  if (domain != AF_INET && domain != AF_INET6) {
    l_loge_s("invalid address family");
    return -1;
  }
  if (!llsocketcreate(domain, SOCK_STREAM, IPPROTO_TCP, &sock)) {
    return -1;
  }
  /* 如果一个TCP客户或服务器未曾调用bind绑定一个端口，当使用connect或
  listen 时，内核会为相应的套接字选择一个临时端口 */
  if (addr && !llsocketbind(sock, addr)) {
    l_socket_close(sock);
    return -1;
  }
  if (!llsocketlisten(sock, (backlog <= 0 ? L_SOCKET_BACKLOG : backlog))) {
    l_socket_close(sock);
    return -1;
  }
  return sock;
}

/** POSIX signal interrupt process's execution **
信号（signal）是告知某个进程发生了某个事件的通知，也称谓软中断
（software interrupt）。信号通常是异步发生的，也就是说进程预先
不知道信号的准确发生时机。信号可以有一个进程发送给另一个进程或
进程自身，或者由内核发给某个进程。例如SIGCHLD信号是由内核在任
何子进程终止时发给它的父进程的一个信号。
每个信号都有一个与之关联的处置（disposition），也称为行为
（action），我们通过调用sigaction函数来设定一个信号的处置，并有
三种选择。其一，可以提供一个函数，只要有特定信号发生时它就会调用。
这样的函数称为信号处理函数（signal handler），这种行为称为捕获
（catching）信号。有两个信号不能被捕获，它们是SIGKILL和SIGSTOP。
信号处理函数的原型为 void (*handle)(int signo) 。对于大多数信号
来说，调用sigaction函数并指定信号发生时所调用的函数就是捕获信号
所需要的全部工作。不过稍后可以看到，SIGIO、SIGPOLL和SIGURG这些
信号还要求捕获它们的进程做些额外工作。
其二，可以把某个信号的处置设定为SIG_IGN来忽略它，SIGKILL和SIGSTOP
这两个信号不能被忽略。其三，可以把某个信号的处置设定为SIG_DFL启用
默认处置。默认处置通常是在收到信号后终止进程，某些信号还在当前工作
目录产生一个core dump。另有个别信号的默认处置是忽略，SIGCHLD和
SIGURG（带外数据到达时发送）是默认处置为忽略的两个信号。
POSIX允许我们指定一组信号，它们在信号处理函数被调用时阻塞，任何阻塞
的信号都不能递交给进程。如果将sa_mask成员设为空集，意味着在该信号处
理期间，不阻塞额外的信号。POSIX会保证被捕获的信号在其信号处理期间总
是阻塞的。因此在信号处理函数执行期间，被捕获的信号和sa_mask中的信号
是被阻塞的。如果一个信号在被阻塞期间产生了一次或多次，那么该信号被
解阻塞之后通常只递交一次，即Unix信号默认是不排队的。利用sigprocmask
函数选择性的阻塞或解阻塞一组信号是可能的。这使得我们可以做到在一段
临界区代码执行期间，防止捕获某些信号，以保护这段代码。
警告：在信号处理函数中调用诸如printf这样的标准I/O函数是不合适的，其原
因将在11.18中讨论。在System V和Unix 98标准下，如果一个进程把SIGCHLD
设为SIG_IGN，它的子进程就不会变成僵死进程。不幸的是，这种做法仅仅适用
于System V和Unix 98，而POSIX明确表示没有这样的规定。处理僵死进程的可
移植方法是捕获SIGCHLD信号，并调用wait或waitpid。
从子进程终止SIGCHLD信号递交时，父进程阻塞于accept调用，然后SIGCHLD的
处理函数会执行，其wait调用取到子进程的PID和终止状态，最后返回。既然
该信号实在父进程阻塞于慢系统调用（accept）时由父进程捕获的，内核会使
accept返回一个EINTR错误（被中断的系统调用）。
我们用术语慢系统调用（slow system call）描述accept函数，该术语也适用
于那些可能永远阻塞的系统调用。举例来说，如果没有客户连接到服务器上，
那么服务器的accept就没有返回的机会。类似的如果客户没有发送数据给服务
器，那么服务器的read调用也将永不返回。适用于慢系统调用的基本规则是：
当阻塞于某个慢系统调用的一个进程捕获某个信号且相应相应信号处理函数返回
时，该系统调用可能返回一个EINTR错误。有些内核自动重启某些被中断的系统
调用。不过为了便于移植，当我们编写捕获信号的程序时，必须对慢系统调用
返回EINTR有所准备。移植性问题是由早期使用的修饰词“可能”、“有些”和对
POSIX的SA_RESTART标志的支持是可选的这一事实造成的。即使某个实现支持
SA_RESTART标志，也并非所有被中断的系统调用都可以自动重启。例如大多数
源自Berkeley的实现从不自动重启select，其中有些实现从不重启accept和
recvfrom。
为了处理被中断的accept，需要判断错误是否是EINTR，如果是则重新调用accept
函数，而不是直接错误返回。对于accept以及诸如read、write、select和open
之类的函数来说，这是合适的。不过有一个函数我们不能重启：connect。如果该函数
返回EINTR，我们就不能再次调用它，否则立即返回一个错误。当connect被一个捕获
的信号中断而且不自动重启时（TCPv2第466页），我们必须调用select来等待连接
完成。如16.3节所述。
 ** 服务器进程终止 **
当服务器进程终止时会关闭所有打开的文件描述符，这回导致向客户端发送一个FIN，
而客户端TCP响应一个ACK，这就是TCP连接终止工作的前半部分。如果此时客户不理会
读取数据时返回的错误，反而写入更多的数据到服务器上会发生什么呢？这种情况是
可能发生的，例如客户可能在读回任何数据之前执行两次针对服务器的写操作，而第
一次写引发了RST。适用于此的规则是：当一个进程向某个已收到RST的套接字执行写
操作时，内核会向该进程发送一个SIGPIPE信号。该信号的默认行为是终止进程，因此
进程必须捕获以免不情愿的被终止。不论该进程是捕获该信号并从其信号处理函数返回，
还是简单地忽略该信号，写操作都将返回EPIPE错误。
一个在Usenet上经常问及的问题是如何在第一次写操作时而不是在第二次写操作时捕获
该信号。这是不可能的，第一次写操作引发RST，第二次写引发SIGPIPE信号。因为写一个
已接收了FIN的套接字不成问题，但写一个已接收了RST的套接字是一个错误。
处理SIGPIPE的建议方法取决于它发生时应用进程想做什么。如果没有特殊的事情要做，
那么将信号处理方法直接设置为SIG_IGN，并假设后续的输出操作将检查EPIPE错误并终止。
如果信号出现时需采取特殊措施（可能需要在日志文件中记录），那么就必须捕获该信号，
以便在信号处理函数中执行所有期望的动作。但是必须意识到，如果使用了多个套接字，
该信号的递交无法告诉我们是哪个套接字出的错。如果我们确实需要知道是哪个write
出错，那么必须不理会该信号，那么从信号处理函数返回后再处理EPIPE错误。*/

typedef void (*l_sigfunc)(int);

static l_sigfunc llsigact(int sig, l_sigfunc func) {
  /** sigaction - examine and change a signal action **
  #include <signal.h>
  int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact);
  It is used to change the action taken by a process on receipt of a specific signal.
  signum specifies the signal and can be any valid signal except SIGKILL and SIGSTOP.
  If act is non-null, the new action for signum is installed from act. If oldact is
  non-null, the previous action is saved in oldact. The sigaction structure:
  struct sigaction {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*, void*);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
  };
  On some architectures a union is involved, don't assign to both sa_handler and
  sa_sigaction. The sa_restorer is not intended for application use (POSIX does
  not specify a sa_restorer field).
  sa_handler specifies the action to be associated with signum and may be SIG_DFL
  for the default action, SIG_IGN to ignore this signal, or pointer to a signal
  handling function. This function receives the signal number as its only argument.
  sa_mask specifies a mask of signals which should be blocked during execution of
  the signal handler. In addition, the signal which triggered the handler will be
  blocked, unless the SA_NODEFER flag is used.
  If SA_SIGINFO is specified in sa_flags, then sa_sigaction (instead of sa_handler)
  specifies the signal-handling function for signum. Other flags can be:
  SA_NOCLDSTOP - If signum is SIGCHLD, don't receive notification when child process
  stop (i.e., when they receive one of SIGSTOP, SIGTSTP, SIGTTIN, or SIGTTOU) or
  resume (i.e., they receive SIGCONT). This flag is only for SIGCHLD.
  SA_NOCLDWAIT (since Linux 2.6) - If signum is SIGHLD, do not transform children
  into zombies when they terminate. This flag is menaingful only when establishing a
  handler for SIGCHLD, or when setting that signal's disposition to SIG_DFL.
  SA_NODEFER - Don't prevent the signal from being received from within its own
  signal handler. This flag is meaningful only when establishing a single handler.
  SA_NOMASK is an obsolete, nonstandard synonym for this flag.
  SA_ONSTACK - Call the signal handler on an alternate signal stack provided by
  sigaltstack(2). If an alternate stack is not available, the default stack will be
  used. This flag is meaningful only when establishing a signal handler.
  SA_RESETHAND - Restore the signal action to the default upon entry to the signal
  handler. This flag is meaningful only when establishing a signal handler. SA_ONESHOT
  is an obsolete, nonstandard synonym for this flag.
  SA_RESTART - Provide behavior compatible with BSD signal semantics by making certain
  ssystem calls restartable across signals. This flag is meaningful only when establishing
  a signal handler. See signal(7) for a discussion of system call restarting.
  SA_RESTORER - Not intended for application use. This flag is used by C libraries to
  indicate that the sa_restorer field contains the address of a "signal trampoline".
  SA_SIGINFO (since Linux 2.2) - The signal handler takes three arguments, not one. In
  this case, sa_sigaction should be set instead of sa_handler. This flag is meaningful
  only when establishing a signal handler.
  It is returns 0 on success, on error, -1 is returned, and errno is set.
   ** POSIX signal set operations **
  #include <signal.h>
  int sigemptyset(sigset_t* set); // set empty
  int sigfillset(sigset_t* set); // set full, including all signals
  int sigaddset(sigset_t* set, int sig);
  int sigdelset(sigset_t* set, int sig);
  int sigismember(const sigset_t* set, int sig);
  Return 0 on success and -1 on error, and sigismember returns 1 if is a member, returns
  0 if not a member, and -1 on error. On error, the errno is set.
  When creating a filled signal set, the glibc sigfillset() function doesn't include
  the two real-time signals used internally by the NPTL threading implementation. See
  nptl(7) for details.
  */
  struct sigaction act, oldact;
  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  /* SA_RESTART标志可可选的。如果设置，由相应信号中断的系统调用将由内核自动重启。
  如果被捕获的信号不是SIGALRM且SA_RESTART有定义，我们就设置标志。对SIGALRM进行特殊
  处理的原因在于，产生该信号的目的正如14.2节讨论的那样，通常是为I/O操作设置超时，
  这种情况下我们希望受阻塞的系统调用被该信号中端掉从内核返回到用户进程。一些较早的
  系统（如SunOS 4.x）默认设置为自动重启被中断的系统调用（内核自动重启系统调用不会返
  回用户进程），并定义了与SA_RESTART互补的SA_INTERRUPT标志。如果定义了该标志，我们
  就在被捕获的信号是SIGALRM时设置它。
  */
  if (sig == SIGALRM) {
  #ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT; /* SunOS 4.x */
  #endif
  } else {
  #ifdef SA_RESTART
    act.sa_flags |= SA_RESTART; /* SVR4, 4.4BSD */
  #endif
  }
  if (sigaction(sig, &act, &oldact) != 0) {
    l_loge_1("sigaction %s", lserror(errno));
    return SIG_ERR;
  }
  return oldact.sa_handler;
}

void l_sigign(int sig) {
  llsigact(sig, SIG_IGN);
}

void l_socket_startup() {
  l_sigign(SIGPIPE);
}

/** accept - accept a connection on a socket **
#include <sys/types.h>
#include <sys/socket.h>
int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
It is used with connection-based socket types (SOCK_STREAM,
SOCK_SEQPACKET). It extracts the first connection request on the
queue of pending connections for the listening sockfd, and creates
a new connected socket, and returns a new fd referring to it. The
newly created socket is not in the listening state. The original
sockfd is unaffected by this call.
The addr is a pointer to a sockaddr structure. This structure is
filled in with the address of the peer socket, as known to the
communications layer. The exact format of the address returned is
determined by the socket's address family. When addr is NULL, nothing
is filled in; in this case, addrlen is not used, and should also
be NULL. The addrlen argument is a valud-result argument: the caller
must initialize it to contain the size in bytes of the structure
pointed to by addr; on return it will contain the acutal size of
the peer address. The returned address is truncated if the buffer
provided is too small; in this case, addrlen will return a value
greater than was supplied to the call.
If no pending connections are present on the queue, and the socket
is not marked as nonblocking, accept() blocks the caller until a
connection is present. If the socket is marked nonblocking and no
pending connections are present on the queue, accept() fails with
the error EAGIN or EWOULDBLOCK.
on success, it returns a nonnegative interger that is a fd for the accepted
socket. on error, -1 is returned, and errno is set appropriately.
Error handling - linux accept passes already-pending network errors on the
new socket as an error code from accept(). this behavior differs from other
BSD socket implementations. for reliable operation the application should
detect the network errors defined for the protocol after accept() and treat
them like EAGAIN by retrying. in the case of TCP/IP, these are ENETDOWN,
EPROTO, ENOPROTOOPT, EHOSTDOWN, ENONET, EHOSTUNREACH, EOPNOTSUPP, and
ENETUNREACH.
当有一个已完成的连接准备好被accept时，如果使用select会将其作为可读描述符
返回该连接的监听套接字。因此如果我们使用select在某个监听套接字上等待一个
外来连接，那就没必要把该监听套接字设置为非阻塞的，这是因为如果select告诉我
们该套接字上已有连接就绪，那么随后的accept调用不应该阻塞。不幸的是，这里存在
一个可能让我们掉入陷阱的定时问题(Gierth 1996）。为了查看这个问题，我们使TCP
客户端建立连接后发送一个RST到服务器。然后在服务器端当select返回监听套接字
可读后睡一段时间，然后才调用accept来模拟一个繁忙的服务器。此时客户在服务器调
用accept之前就中止了连接时，源自Berkeley的实现不把这个中止的连接返回给服务器
进程，而其他实现应该返回ECONNABORTED错误，却往往代之以返回EPROTO错误。考虑
源自Berkeley的实现，在TCP收到RST后会将已完成的连接移除出队列，假设队列中没有
其他已完成的连接，此时调用accept时会被阻塞，直到其他某个客户建立一个连接为止。
但是在此期间服务器单纯阻塞在accept调用上，无法处理任何其他以就绪的描述符。本
问题的解决方法是：当使用select获悉某个监听套接字上有已完成的连接准备好时，总
是把这个监听套接字设为非阻塞；在后续的accept调用中忽略以下错误继续执行直到队
列为空返回EAGIN或EWOULDBLOCK为止：ECONNABORTED（POSIX实现，客户中止了连接）、
EPROTO(SVR4实现，客户中止了连接）和EINTR（有信号被捕获）。
以上在accept之前收到RST的情况，不同实现可能以不同的方式处理这种中止的连接。
源自Berkeley的实现完全在内核中处理中止的连接，服务器进程根本看不到。然而大多数
SVR4实现返回一个错误给服务其进程作为accept的返回结果，不过错误本身取决于实现。
这些SVR4实现返回一个EPROTO，而POSIX指出返回的必须是ECONNABORTED（software
caused connection abort）。POSIX作出修正的理由在于，流子系统（streams subsystem）
中发生某些致命的协议相关事件时，也会返回EPROTO。要是对于客户引起的一个已建立连接
的非致命中止也返回同样的错误，那么服务器就不知道该再次调用accept还是不该。换成
是ECONNABORTED错误，服务器可以忽略它，再次调用accept即可。源自Berkeley的内核从
不把该错误传递给进程的做法所涉及的步骤在TCPv2中得到阐述，引发该错误的RST在第964
页到达处理，导致tcp_close被调用。*/

void l_socket_accept(l_handle sock, void (*cb)(void*, l_sockconn*), void* ud) {
  int n = 0;
  l_sockconn conn;
  llsockaddr* sa = (llsockaddr*)&(conn.remote);
  socklen_t providedlen = sizeof(ll_sock_addr);

  for (; ;) {
    sa->len = providedlen;
    if ((conn.sock = accept(sock, &(sa->addr.sa), &(sa->len))) != -1) {
      if (sa->len > providedlen) {
        l_loge_s("accept address truncated");
        sa->len = providedlen;
      }
      llsetnonblock(conn.sock);
      cb(ud, &conn);
      continue;
    }

    n = errno;
    if (n == EAGAIN || n == EWOULDBLOCK) {
      /* no more pending completed connections in the kernel */
      return;
    }

    switch (n) {
    case EINTR: /* system call was interrupted by a signal */
    case ECONNABORTED: /* a connection has been aborted */
    case EPROTO: /* protocol error */
      /* current connection is interrupted, aborted or has protocol error,
      so skip this connection and continue to accept next connections
      in the kernel queue until it is empty */
      l_logw_1("accept %s", lserror(n));
      break;
    case EBADF: /* sockfd is not an open fd */
    case EFAULT: /* the addr is not in a writable part of the user address space */
    case EINVAL: /* sockfd is not listening for connections, or addrlen is invalid */
    case EMFILE: /* per-process limit on the number of open fds reached */
    case ENFILE: /* system-wide limit on the total number of open files reached */
    /* no enough free memory. this often means that the memory allocation is limited */
    case ENOMEM: case ENOBUFS: /* by the socket buffer limits, not by the system memroy */
    case ENOTSOCK: /* the sockfd does not refer to a socket */
    case EOPNOTSUPP: /* the referenced socket is not of type SOCK_STREAM */
    case EPERM: /* firewall rules forbid connection */
      l_loge_1("accept %s", lserror(n));
      return; /* unrecoverable error, return */
    default:
      /* in addition, network errors for the new socket and as defined for the protocol
      may be returned. various linux kernels can return other errors such as ENOSR,
      ESOCKTNOSUPPORT, EPROTONOSUPPORT, ETIMEOUT. the value ERESTARTSYS may be seen
      during a trace. */
      l_loge_1("accept %s", lserror(n));
      return;
    }
  }
}

/** TCP连接的各种可能错误 **
服务器主机崩溃 - 为了模拟这种情形必须在不同的主机上运行客户端和服务器。我们先启动
服务器再启动客户，接着在客户上键入一行文本以确认连接工作正常，然后从网络上断开服务
器主机，并在客户上键入另一行文本。这样同时也模拟了当客户发送数据时服务器不可达的情
形（例如建立连接后某些中间路由器不工作）。当服务器崩溃时，已有的网络连接上不会发出
任何东西。这里我们假设的是主机崩溃，而不是执行关机命令。在客户键入文本后，客户TCP
会持续重传数据，试图从服务器上接收到一个ACK。TCPv2的25.11节给出了TCP重传的一个经典
模式：源自Berkeley的实现重传该数据包12次，共等待约9分钟才放弃。当客户TCP最终放弃时
（假设在这段时间内，服务器主机没有重新启动或假设主机仍然不可达），客户进程会返回一个
错误。如果服务器主机已崩溃从而对客户数据没有响应，则返回的错误将是ETIMEOUT。然而如果
某个中间路由器判定服务器主机已不可达，从而响应一个"destination unreachable"的ICMP
消息，那么所返回的错误是EHOSTUNRACH或ENETUNREACH。
尽管我们的客户最终还是会发现对端主机已崩溃或不可达，不过有时候我们需要比不得不等待
9分钟更快的检测出这种情况。所用的方法就是对读操作设置一个超时，我们将在14.2中讨论
这一点。这里讨论的情形只有在我们向服务器发送数据时才能检测到它已经崩溃。如果我们
不主动给它发送数据也想检测服务器主机的崩溃，那么需要另外一个技术，也就是我们将在
7.5节讨论的SO_KEEPALIVE套接字选项，或某些客户/服务器心搏函数。
如果服务器崩溃后又重启，重新连接到网络中。此时假设没有使用SO_KEEPALIVE选项，那么所
发生的事情是：服务器崩溃后客户发送的数据会在重传，当服务器重新启动并连网后，它的
TCP丢失了崩溃前的所有连接信息，因此服务器的TCP会响应一个RST；当客户收到该RST时，客
户正阻塞于读取系统调用，导致该调用返回ECONNRESET错误。
服务器主机关机 - Unix系统关机时，init进程通常先给所有进程发送SIGTERM信号（该信号可
被捕获），等待一段固定的时间（往往在5到20s之间），然后给所有仍在运行的进程发送
SIGKILL信号（该信号不能被捕获）。这么做留给所有运行进程一小段时间来清除和终止。如果
我们忽略SIGTERM信号（如果默认处置SIGTERM也会终止进程），我们的服务器将由SIGKILL信号
终止。进程终止会使所有打开这的描述符都被关闭，然后服务器会发生正常的TCP断连过程。正
如5.12节所讨论的一样，我们必须在客户中使用select或poll函数，以防TCP断连时客户阻塞在
其他的函数中而不能快速知道TCP已经断连了。*/

static int llsocketconnect(l_handle sock, const l_sockaddr* addr) {
  /** connect - initiate a conneciton on a socket **
  #include <sys/types.h>
  #include <sys/socket.h>
  int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
  It connects the socket referred to sockfd to the address specified by addr.
  The addrlen specifies the size of addr. The format of the address in addr
  is determined by the address space of the socket sockfd.
  If the sockfd is of type SOCK_DGRAM, then addr is the address to which
  datagrams are sent by default, and the only address from which datagrams
  are received. If the socket is of type SOCK_STREAM or SOCK_SEQPACKET, this
  call attempts to make a connection to the socket that is bound to the
  address specified by addr.
  Generally, connection-based protocol sockets may successfully connect()
  only once; connectionless protocol sockets may use connect() multiple
  times to change their association. Connectionless sockets may dissolve
  the association by connecting to an address with the sa_family member of
  sockaddr set to AF_UNSPEC (supported on Linux since kernel 2.2).
  ---
  If the connection or binding succeeds, zero is returned. On error, -1
  is returned, and errno is set. If connect() fails, consider the state of
  the socket as unspecified. Protable appliations should close the socket
  and create a new one for reconnecting.
  EACCES, EPERM - The user tried to connect to a broadcast address without
  having the socket broadcast flag enbaled or the conneciton request failed
  because of a local firewall rule.
  EADDRINUSE - local address is already in use.
  EADDRNOTAVAIL - (Internet domain sockets) The socket referred to by sockfd
  had not previously been bound to and address and, upon attempting to bind
  it to an ephemeral port, it was determined that all port numbers in the
  ephemeral port range are currently in use. See the discussion of
  /proc/sys/net/ipv4/ip_local_port_range in ip(7).
  EAFNOSUPPORT - the passed address didn't have the correct address family.
  EAGAIN - No more free local ports or insufficient entries in the routing
  cache. For AF_INET see the description of /proc/sys/net/ipv4/
  ip_local_port_range ip(7) for information on how to increase the number
  of local ports.
  EALREADY - the socket is nonblocking and a previous connection attempt
  has not yet been completed.
  EBADF - sockfd is not a valid open fd.
  ECONNREFUSHED - no one listening on the remote address.
  EFAULT - the socket structure address is outside the user's address space.
  EINPROGRESS - the socket is nonblocking and the connection cannot be
  completed immediately. It is possible to select or poll for completion
  by selecting the socket for writing. After select indicates writability,
  use getsockopt to read the SO_ERROR option at level SO_SOCKET to
  determine whether connect() completed successfully (SO_ERROR is zero) or
  unsuccessfully.
  EINTR - the system call was interrupted by a signal that was caught.
  EISCONN - the socket is already connected.
  ENETUNREACH - network is unreachable.
  ENOTSOCK - the sockfd does not refer to a socket.
  EPROTOTYPE - the socket type does not support the requested communications
  protocol. This error can occur, for example, on an attempt to conect a
  UNIX domain datagram socket to a stream socket.
  ETIMEDOUT - timeout while attempting connection. the server may be too busy
  to accept new connections. Note that for ip sockets the timeout may be very
  long when syncookies are enabled on the server.
  如果是TCP套接字，调用connect函数会激发TCP三次握手过程，而且仅在连接建立成功
  或出错时才返回，其中错误返回可能有以下几种情况：
  如果TCP客户没有收到SYN的回应，则返回ETIMEOUT错误。举例来说，调用connect()时，
  4.4BSD内核发送一个SYN，若无响应则等待6s后再发送一个，若仍然无响应则等待24s
  后再发送一个（TCPv2第828页）。若总共等了75s后仍未收到响应则返回本错误。
  若对客户的SYN响应的是RST，则表明该服务器主机在我们指定的端口上没有进程在等待
  与之连接（如服务器进程也许没有在运行）。这是一种硬错误（hard error），客户一
  接收到RST就马上返回ECONNREFUSED错误。RST是TCP发生错误时发送的一种TCP数据包。
  产生RST的三个条件是：目的地为某端口的SYN到达，然而该端口上没有正在监听的服务
  器；TCP想取消一个已有连接；TCP接收到一个根本不存在的连接上的数据包。
  若客户发出的SYN在中间某个路由器上引发一个destination unreachable（目的地不可
  达）ICMP错误，则认为是一种软错误（soft error）。客户主机内核保存该消息，并按
  第一种情况中所述的时间间隔重新发送SYN。若在某个规定的时间后仍未收到响应，则把
  保存的消息作为EHOSTUNREADCH或ENETUNREACH错误返回给进程。以下两种情况也是可能
  的：一是按照本地系统的转发表，根本没有到达远程系统的路径；二是connect调用根本
  不等待就返回。许多早期系统比如4.2BSD在收到“目的地不可达”ICMP错误时会不正确地
  放弃建立连接的尝试。注意，网路不可达的错误被认为已过时，应用进程应该把
  ENETUNREACH和EHOSTUNREACH作为相同的错误对待。
  按照TCP状态转换图，connect函数导致当前套接字从CLOSED状态转移到SYN_SENT状态，
  若成功再转移到ESTABLISHED状态，如connect失败则该套接字不再可用，必须关闭。
  我们不能对这样的套接字再次调用connect函数
  ---
  当在一个非阻塞的TCP套接字上调用connect时会立即返回一个EINPROGRESS，已经发起的
　　TCP三次握手继续进行，然后可以使用select检测这个连接是否建立成功。使用非阻塞连接:
  (1) 可以把三次握手叠加在其他处理上，完成一个connect要一个RTT时间，而RTT波动范围
　很大，从局域网上的几个毫秒到几百个毫秒到广域网的甚至几秒，这段时间内可能还有其他事情
　需要处理。
　　(2) 可以使用这个技术同时建立多个连接，这个用途随着Web浏览器变得流行起来。
  (3) 既然使用select等待连接建立，就可以给select指定时间限制，使得能缩短连接超时。
　许多实现有75s到数分钟的connect超时时间，应用程序有时想要一个更短的超时。实现方法
　之一就是使用非阻塞connect，另外14.2节也介绍过设置套接字操作超时的方法。
  尽管套接字是非阻塞的，如果连接的服务器在同一个主机上，调用connect通常连接会立即成功。
　源自Berkeley的实现（和POSIX）关于select和非阻塞connect有以下两个规则：
　　(1) 当连接成功建立时，描述符变成可写（TCPv2第531页）；
　 (2) 当连接建立遇到错误时，描述符变得可读又可写（TCPv2第530页）；
　关于select的这两个规则出自6.3节中关于描述符就绪条件的相关规则，一个TCP套接字变为可写
　的条件是，其发送缓冲区中有可用空间（对于连接建立中的套接字而言本子条件总为真，因为尚未
　往其写出任何数据）并且套接字已成功建立（本子条件为真发生在三次握手成功之后）。一个TCP
　套接字发生发生某个错误时，这个待处理错误总是导致套接字变为可读又可写。
  调用select之前可能连接已经建立并有来自对端的数据到达，这种情况下即使套接字上不发生错误，
　套接字也是可读又可写的；阻塞套接字连接在TCP三次握手完成之前被中断了会发生什么？假设被中
　断的套接字没有被内核自动重启，那么它会返回EINTR，此时不能再次调用connect等待未完成的连接，
　这样做将导致返回EADDRINUSE错误，我们只能调用select，就像对于非阻塞connect所做的那样，
　连接建立成功时返回套接字可写条件，连接建立失败时select返回套接字既可读也可写条件。*/
  int n = 0;
  const llsockaddr* sa = (const llsockaddr*)addr;
  if (connect(sock, &(sa->addr.sa), sa->len) == 0) {
    return true;
  }
  n = errno;
  if (n == EISCONN) {
    return true;
  }
  if (n == EINPROGRESS || n == EALREADY || n == EINTR) {
    /* the connection doesn't complete yet */
    errno = EINPROGRESS;
  } else {
    l_loge_1("connect %s", lserror(n));
  }
  return false;
}

void l_socketconn_init(l_sockconn* self, l_strt ip, l_ushort port) {
  self->sock = -1;
  l_sockaddr_init(&(self->remote), ip, port);
}

/* return true if success, return false if inprocess or error (socket closed on error) */
int l_socket_connect(l_sockconn* conn) {
  l_handle sock = conn->sock;
  l_sockaddr* addr = &(conn->remote);
  if (!l_socket_is_open(sock)) {
    llsockaddr* sa = (llsockaddr*)addr;
    int domain = sa->addr.sa.sa_family;
    if (domain != AF_INET && domain != AF_INET6) {
      l_loge_s("connect invalid address");
      return false;
    }
    if (!llsocketcreate(domain, SOCK_STREAM, IPPROTO_TCP, &sock)) {
      return false;
    }
  } else {
    /* socket already opened, it should be called 2nd time after EINPROGRESS */
  }
  if (llsocketconnect(sock, addr)) {
    return true;
  }
  if (errno != EINPROGRESS) {
    l_socket_close(sock);
  }
  return false;
}

l_int ll_read(l_handle fd, void* out, l_int count) {
  /** read - read from a file descriptor **
  #include <unistd.h>
  ssize_t read(int fd, void *buf, size_t count);
  read up to count bytes from fd into out buffer. if count is 0, read() may detect
  the errors described below. in the absence of any errors, or if read() does not
  check for errors, it returns 0 and has no other effects. according to POSIX.1, if
  count is greater than SSIZE_MAX, the result is implementation-defined; on linux,
  read() and similar system calls will transfer at most 0x7ffff000 (2,147,479,552)
  bytes, returning the number of bytes actually transferred. (this is true on both
  32-bit and 64-bit systems).
  On success, the number of bytes read is returned, and the file position is
  advanced by this number. It is not an error if this number is smaller than the
  number of bytes requested; this may happen for example because fewer bytes are
  actually available right now (maybe because we were close to end-of-file, or
  reading from a pipe, or from a terminal), or because read() was interrupted by
  a signal. On error, -1 is returned, and errno is set appropriately. In this case,
  it is left unspecified whether the file position changes.
  EAGAIN - the fd refers to a file other than a socket and has been marked nonblocking,
  and the read would block.
  EAGAIN or EWOULDBLOCK - the fd refers to a socket and has been marked nonblocking,
  and the read would block. POSIX.1-2001 allows either error to be returned for this
  case, and does not require these constants to have the same value, so a portable
  application should check for both possibilities.
  EBADF - fd is not a valid fd or is not open for reading
  EFAULT - buf is outside your accessible address space
  EINTR - the call was interrupted by a signal before any data was read
  EINVAL - fd is attached to an object which is unsuitable for reading; or the file was
  opened with the O_DIRECT flag, and either the address specified in buf, the value
  specified in count, or the file offset is not suitably aligned
  EINVAL - fd was created via a call to timerfd_create(2) and the wrong size buffer was
  given to read(); see timerfd_create(2) for further information
  EIO - I/O error. this will happen for example when the process is in a background
  process group, tries to read from this controlling terminal, and either it is ignoring
  or blocking SIGTTIN or its process group is orphaned. It may also occur when there is
  a low-level I/O error while reading from a disk or tape.
  EISDIR - fd refers to a directory.
  Other errors may occurs, depending on the object connected to fd. */
  ssize_t n = 0;

  if (count < 0 || count > l_max_rdwr_size) {
    l_loge_s("read invalid argument");
    return -2;
  }

  for (; ;) {
    if ((n = read(fd, out, (size_t)count)) >= 0) {
      /* note that one case about read bytes n < count is:
      at least one byte is read and then interrupted by a
      signal, the call is returned success in this case. */
      return (l_int)n;
    }

    n = errno;
    if (n == EINTR) {
      /* interrupted by a signal before read any bytes,
      try to read again. */
      continue;
    }

    if (n == EAGAIN || n == EWOULDBLOCK) {
      /* data is not available currently */
      return -1;
    }

    /* error occurred */
    break;
  }

  l_loge_1("read %s", lserror(n));
  return -2;
}

l_int ll_write(l_handle fd, const void* buf, l_int count) {
  /** write - write to a file descriptor **
  #include <unistd.h>
  ssize_t write(int fd, const void *buf, size_t count);
  Writes up to count bytes from the buffer to the file referred to by the fd.
  The number of bytes written may be less than count if, for example, there
  is insufficient space on the underlying physical medium, or the RLIMIT_FSIZE
  resource limit is encountered (see setrlimit(2)), or the call was interrupted
  by a signal handler after having written less then bytes. (see also pipe(7).)
  POSIX requires that a read(2) that can be proved to occur after a write()
  has returned will return the new data. Note that not all filesystems are
  POSIX conforming.
  According to POSIX.1, if count is greater than SSIZE_MAX, the result is
  implementation-defined; on linux, write() and similar system calls will
  transfer at most 0x7ffff000 (2,147,479,522) bytes, returning the number of
  bytes actually transferred. (this is true on both 32-bit and 64-bit systems).
  A successful return from write() does not make any guarantee that data has
  been committed to disk. In fact, on some buggy implementations, it does not
  even guarantee that space has successfully been reserved for the data. The
  only way is to call fsync(2) after you are done writing all your data.
  If a write() is interrupted by a signal handler before any bytes are written,
  then the call fails with the error EINTR; if it is interrupted after at
  least one byte has been writtn, the call succeeds, and returns the number
  of bytes written.
  ---
  On success, the number of bytes written is returned. It is not an error if
  this number is smaller than the number of bytes requested; this may happen
  for example because the disk device was filled, or interrupted by a signal.
  On error, -1 is returned, and errno is set appropriately.
  If count is zero and fd refers to a regular file, then write() may return a
  failure status if one of the errors below is detected. If no errors are
  detected, or error detection is not performed, 0 will be returned without
  causing any other effect. If count is zero and fd refers to a file other than
  a regular file, the results are not specified.
  EAGAIN - the fd refers to a file other than a socket and has been marked
  nonblocking, and the write would block.
  EAGAIN or EWOULDBLOCK - the fd refers to a socket and has been marked
  nonblocking, and the write would block. POSIX.1-2001 allows either error to
  be returned for this case, and does not require these constants to have the
  same value, so a protable application should check for both possibilities.
  EBADF - fd is not a valid fd or is not open for writing.
  EDESTADDRREQ - fd refers to a datagram socket for which a peer address has
  not been set using connect(2).
  EDQUOT - the usr's quota of disk blocks on the filesystem containing the file
  referred to by fd has been exhausted.
  EFAULT - buf is outside your accessible address space.
  EFBIG - an attempt was made to write a file that exceeds the implementation-
  defined maximum file size or the process's file size limit, or to write
  at position past the maximum allowed offset.
  EINTR - the call was interrupted by a signal before any data was written.
  EINVAL - fd is attached to an object which is unsuitable for writing; or the
  file was opened with the O_DIRECT flag, and either the address specified in
  buf, the value specified in count, or the file offset is not suitably aligned.
  EIO - a low-level I/O error occurred while modifying the inode.
  ENOSPC - the device containing the file referred to by fd has no room for
  the data.
  EPERM - the operation was prevented by a file seal; see fcntl(2).
  EPIPE - fd is connected to a pipe or socket whose reading end is closed.
  When this happens the writing process will also receive a SIGPIPE signal.
  (Thus, the write return value is seen only if the program catches, blocks
  or ignores this signal.)
  Other errors may occur, depending on the object connected to fd. */
  ssize_t n = 0;

  if (count < 0 || count > l_max_rdwr_size) {
    l_loge_s("write invalid argument");
    return -2;
  }

  for (; ;) {
    if ((n = write(fd, buf, (size_t)count)) >= 0) {
      /* note that one case about written bytes n < count is:
      at least one byte is written and then interrupted by a
      signal, the call is returned success in this case. */
      return (l_int)n;
    }

    n = errno;
    if (n == EINTR) {
      /* interrupted by a signal before written any bytes,
      try to read again. */
      continue;
    }

    if (n == EAGAIN || n == EWOULDBLOCK) {
      /* cannot write currently */
      return -1;
    }

    /* error occurred */
    break;
  }

  l_loge_1("write %s", lserror(n));
  return -2;
}

/* *status >=0 success, <0 L_STATUS_ERROR */
l_int l_socket_read(l_handle sock, void* out, l_int count, l_int* status) {
  l_byte* buf = (l_byte*)out;
  l_int n = 0, sum = 0;
  while ((n = ll_read(sock, buf, count)) > 0) {
    sum += n;
    buf += n;
    count -= n;
    if (count == 0) {
      break;
    }
  }
  if (status) {
    *status = (count == 0 ? 0 : (n == -2 ? L_STATUS_ERROR : count));
  }
  return sum;
}

/* *status >=0 success, <0 L_STATUS_ERROR */
l_int l_socket_write(l_handle sock, const void* from, l_int count, l_int* status) {
  l_int n = 0, sum = 0;
  const l_byte* buf = (const l_byte*)from;
  while ((n = ll_write(sock, buf, count)) > 0) {
    sum += n;
    buf += n;
    count -= n;
    if (count == 0) {
      break;
    }
  }
  if (status) {
    *status = (count == 0 ? 0 : (n == -2 ? L_STATUS_ERROR : count));
  }
  return sum;
}

void l_plat_sock_test() {
  l_sockaddr sa;
  l_string ip = l_create_string_from(l_empty_strt());
  /* all kind of socket address size */
  l_logd_1("socklen_t %d-byte", ld(sizeof(socklen_t)));
  l_logd_1("struct in_addr %d-byte", ld(sizeof(struct in_addr)));
  l_logd_1("struct in6_addr %d-byte", ld(sizeof(struct in6_addr)));
  l_logd_1("struct sockaddr %d-byte", ld(sizeof(struct sockaddr)));
  l_logd_1("struct sockaddr_in %d-byte", ld(sizeof(struct sockaddr_in)));
  l_logd_1("struct sockaddr_in6 %d-byte", ld(sizeof(struct sockaddr_in6)));
  l_logd_1("struct sockaddr_storage %d-byte", ld(sizeof(struct sockaddr_storage)));
  l_logd_1("L_SOCKADDR_SIZE %d-byte", ld(L_SOCKADDR_SIZE));
  l_logd_1("l_sockaddr %d-byte", ld(sizeof(l_sockaddr)));
  l_logd_1("INET_ADDRSTRLEN ipv4 string max len %d", ld(INET_ADDRSTRLEN));
  l_logd_1("INET6_ADDRSTRLEN ipv6 string max len %d", ld(INET6_ADDRSTRLEN));
  /* ipv4 string convert */
  l_sockaddr_init(&sa, l_literal_strt("127.0.0.1"), 1024);
  l_sockaddr_ipstring(&sa, &ip);
  l_assert(l_sockaddr_port(&sa) == 1024);
  l_assert(l_string_equal(&ip, l_literal_strt("127.0.0.1")));
  /* ipv6 string convert */
  l_sockaddr_init(&sa, l_literal_strt("::3742:204.152.189.116"), 2048);
  l_sockaddr_ipstring(&sa, &ip);
  l_assert(l_sockaddr_port(&sa) == 2048);
  l_assert(l_string_equal(&ip, l_literal_strt("::3742:cc98:bd74")));
  l_logd_1("l_sockaddr ip string %s", lstring(&ip));
  l_sockaddr_init(&sa, l_literal_strt("::3742:4723:5525"), 4096);
  l_sockaddr_ipstring(&sa, &ip);
  l_assert(l_sockaddr_port(&sa) == 4096);
  l_assert(l_string_equal(&ip, l_literal_strt("::3742:4723:5525")));
  l_string_free(&ip);
  /* protocol number */
  l_logd_1("IPPROTO_IP(0) is %d", ld(IPPROTO_IP));
  l_logd_1("IPPROTO_IPV6(41) is %d", ld(IPPROTO_IPV6));
  l_logd_1("IPPROTO_TCP(6) is %d", ld(IPPROTO_TCP));
  l_logd_1("IPPROTO_UDP(17) is %d", ld(IPPROTO_UDP));
}

