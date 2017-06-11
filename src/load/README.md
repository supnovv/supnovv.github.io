
C
```
编译环境
* MAKE 文件主要由一系列构建规则组成，每个构建规则分为三部分：工作目标（target），工作目标的必要条件（prerequisite）和需要执行的命令（command）
* 工作目标是一个必须建造的文件或进行的事情，必要条件是工作目标成功创建之前必须事先存在的文件，而命令是必要条件成立时创建工作目标需要执行的SHELL命令
* 链接库可以作为目标的必要条件，例如 count: count.c -lfl，当查找链接库 -l<NAME> 时 make 会依次搜素 libNAME.so 和 libNAME.a 形式的库文件
* 在执行一个工作目标时，会先检查目标的必要条件，如果有文件不存在会搜寻合适的目标创建这个文件，当所有必要文件都存在后就执行命令创建工作目标
* 工作目标不会更新的唯一情况是：目标文件和必要条件中的文件都已经存在，且目标文件时间戳比所有必要文件的时间戳都要新；另外每条命令会在单独的SHELL中运行
* MAKE 文件的第一个工作目标称为默认工作目标，当执行 make 时不指定工作目标，则执行的是默认工作目标
* MAKE 提供了很多命令行选项，其中一个有用的选项是 --just-print 或 -n，该选项使 make 仅显示而不执行工作目标的命令，该功能在编写 MAKE 文件时特别有用
* 一条构建规则的一般形式如下，其中目标可以有0个或多个必要条件，当必要条件为0时对应的命令仅在目标文件不存在时执行，另外多个目标可以共享相同的必要条件和命令
* 需要执行的命令也可以是0个或多个，如果命令为0个表示这个工作目标仅依赖于必要条件，如果必要条件也为空，则这是一个空目标
* target1 target2 target3: prerequisite1 prerequisite2
* <tab> command1
* <tab> command2
* <tab> command3
* 工作目标的必要条件如果很多可以分多次书写，第一次之后指定的必要条件会追加到原来工作目标的必要条件中，例如：
* vpath.o: lexer.c hash.h commands.h filedef.h job.h dep.h
* vpath.o: vpath.c make.h config.h getopt.h gettext.h vpath.h
* <tab> $(CC) $(CFLAGS) -o $@ -c $<
* 模式规则的一般形式如下所示，它们常用于说明某类文件如何生成，例如下面目标文件以及两种可执行文件的生成规则：
* %.o: %.c
* <tab> $(CC) $(CFLAGS) -o $@ -c $<
* %: %.c
* <tab> $(LINK) $^ $(LDFLAGS) $(LDFLAGS) -o $@
* %: %.mod
* <tab> $(CLMOD) -o $@ -e $@ $^
* 模式规则中的百分号 % 可以替代任意多字符，百分号可以放在模式串任何位置但只能出现一次，例如 %,v s%.o wrapper_%
* 当 MAKE 需要使用模式规则生成某类文件时，它会查找相符的模式规则的工作目标，并以匹配百分号的字符串替换到必要条件中以得到具体规则
* 另外还有静态模式规则能应用在特定的工作目标上，例如下面例子中 %.o 只会使用 $(OBJECTS) 中的目标文件进行替换：
* $(OBJECTS): %.o: %.c
* <tab> $(CC) -c $(CFLAGS) $< -o$@
* MAKE 定义了很多隐含规则，这些规则不是模式规则就是老式的后缀规则，这些内置规则可应用于各种类型的文件，可以通过 make -p 查看这些规则
* 当 make 检查一个工作目标时如果找不到可以更新它的具体规则，就会使用隐含规则，隐含规则很容易使用，当编写自己的具体规则时不指定执行命令就行了
* 另外一个没有指定命令脚本的模式规则会将对应的 MAKE 隐含规则删除，例如 %.c: %.l 会删除从 .l 文件生成 .c 文件的隐含规则
* 当链接器连接目标文件或库时，会依次搜索命令行上指定的文件，如果库A包含了一个未定义的符号而且该符号定义在库B中，那么就必须在B之前指定A
* 否则一旦链接器读进A发现未定义的符号再要链接器回头就迟了，因为链接器不会回头再读取前面的程序库，因此命令行上库的指定顺序相当重要
* 一个相关的问题是程序库之间的相互引用或循环引用，此时需要在命令行指定一个程序库两次以解决依赖问题，例如 -lA -lB -lA，下面是一个示例：
* xpong: xpong.o libui.a libdynamics.a libui.a -lX11
* <tab> $(CC) $+ -o $@  # 这里使用了 $+，它不会删除重复的文件以确保连接操作的正确执行
* MAKE 变量的值由赋值符号右边已删除前导空格的所有字符组成，但值尾部空格不会删除；变量有简单扩展（使用 := 赋值）和递归扩展（使用 = 赋值）两种类型
* 简单扩展的含义是变量的值会在定义时立即进行扩展，此方式下赋值时引用的变量必须都已经定义，未定义的变量会扩展成空字符串
* 递归扩展变量在定义时对应的值不立即进行扩展（只简单记住所有字符）而是延迟到变量使用时，这种扩展不会影响简单的字符串赋值，因为这些值不需要扩展
* 另外两个赋值操作符是 += 和 ?=，第一个操作符 += 用于为变量追加新值，而 ?= 仅在变量的值尚未设定（即使设置为空也表示已经设定）时才对变量赋值
* 变量可以用来存储单行字符串，如果要保存多行字符串（例如多行命令）则需要使用宏，通过 define 命令定义的变量称为宏，宏可以包含内置的换行符，例如：
* define create-jar
*   @echo creating $@...
*   $(JAR) -ufm $@ $(MANIFEST)
* endef # 然后宏可以像普通变量那样使用
* 总体来说，赋值符左边部分的变量名会立即扩展，:= 赋值符的右边部分也会立即扩展，而 = 和 ?= 右边部分会等到执行时扩展
* 而 += 赋值符如果左边变量原本是一个简单扩展变量则右边部分会立即扩展，否则延迟到执行时；而对于宏定义，变量名会立即扩展，主体部分会延迟到执行时
* 对于规则，工作目标和必要条件都会立即扩展，而命令则会延迟到执行时；一个准则是尽量先定义变量和宏再使用它们，尤其是在工作目标和必要条件中使用变量时
* 一个重要特性是变量可以在目标的必要条件中赋值，此种赋值仅在工作目标以及相应的任何必要条件需要处理时才执行，且工作目标处理完后变量会恢复其原有值
* gui.o: CPPFLAGS += -DUSE_NEW_MALLOC=1
* gui.o: gui.cpp gui.h
* 另外，使用 vpath 命令可以告诉 MAKE 怎样搜索文件，例如 vpath %.l %.c src 表示在当前 src 目录中搜索 .l 和 .c 文件
* MAKE 还定义了一些默认变量，它们可以获取工作目标以及必要条件中的元素，例如 $@ 表示当前工作目标文件名，$< 表示第一个必要条件的文件名
* $^ 表示所有必要条件的文件名，中间用空格隔开，这份列表删除了重复的文件名，而 $+ 与 $^ 相同只是没有删除重复文件名
* $? 表示时间戳在工作目标时间戳之后的所有必要条件文件名，$* 表示去除文件名后缀的工作目标文件名（但注意是如果文件名没有以.开始的后缀，该变量返回空）
* 这些变量都是单字符变量，在引用时可以不必使用括号，而这些变量的变体需要使用括号，例如 $(@D)、$(<D) 等表示目录部分，$(@F) 等表示文件部分
* 变量可以通过 include 来源于其他文件，也可以在命令行为 make 指定选项重新定义变量，例如：
* make CFLAGS=-g CPPGLAGS="-DBSD -DDEBUG"  # 命令行上定义的变量会覆盖环境变量以及 MAKE 文件中的赋值结果
* 为了避免被命令行的变量覆盖，MAKE 文件中的变量可以使用 override 命令来定义，例如 override LDFLAGS = -EB # 使用 big endian
* 在 make 启动时，所有来自于环境的变量都会自动定义成 make 变量，这些变量具有最低的优先级，所有 MAKE 文件或命令行的赋值都会覆盖环境变量的值
* 当使用条件处理指令时，MAKE 文件根据条件处理结果有些部分会选中而有些部分会被省略，用来控制是否选择的条件具有各种形式：ifdef/ifndef/ifeq/ifneq
* 当使用 ifdef/ifndef 时不需要使用 $() 括住变量名，而使用 ifeq/ifneq 时测试表达式的形式为 "a" "b" 或 (a, b)
* 采用 (a, b) 的形式时必须注意括号内的空格，因为在解析时除了逗号之后的空格会删除外，其他空格都会保留作为变量值的一部分
* 为了创建更稳定的 MAKE 文件，可以使用引号形式的测试表达式，还可以使用 strip 函数手动去除空格，例如：
* ifeq "$(strip $(OPTION))" "-d"
*   CFLAGS += -DDEBUG
* endif
* 条件处理指令可以在 MAKE 文件顶层使用，也可以用在宏定义以及目标命令中使用，例如：
* libgui.a: $(gui_objects)
* <tab> $(AR) $(ARFLAGS) $@ $<
*   ifdef RANLIB
* <tab> $(RANLIB) $@
*   endif
* 注意条件处理指令只有 if-cond endif 和 if-cond else endif 两种形式，没有对应 else-if-cond 的命令，但可以使用嵌套的条件指令实现
* 另外在 MAKE 文件中还可以直接使用的变量有 MAKE_VERSION、CURDIR、MAKEFILE_LIST、.VARIABLES 等
* GCC -E 输出预处理后的源代码，-S 输出编译后的汇编代码，-c 输出汇编后的目标代码，不带选项输出链接后的可执行文件，都可通过 -o 指定输出文件名
WINDOWS环境
* MSYS2下载和安装：https://sourceforge.net/p/msys2/wiki/MSYS2%20installation/
* 安装WINDOWS SDK: https://www.microsoft.com/en-us/download/details.aspx?id=8279
* WINDOWS SDK安装的默认路径为：C:\Program Files\Microsoft SDKs\Windows\v7.1
* WINDOWS SDK帮助文档：C:\Program Files\Microsoft SDKs\Windows\v7.1\ReleaseNotes.Htm
* NMAKE参考：https://msdn.microsoft.com/en-us/library/dd9y37ha.aspx
* 工作目标可以指定一个或多个，它们可以是文件名、目录名、或伪目标（不真实存在），目标之间可以用一个或多个空格或tab隔开，但第一个目标必须顶格开始
* 目标包含的字符不区分大小写，字符的最大长度不能超过256个，如果冒号之前的目标是一个单独的字符，需要在字符与冒号之间加空格，避免解析成盘符
* 相同目标的依赖条件会按顺序合并到一起，例如：
* bounce.exe: jump.obj
* bounce.exe: up.obj
* <tab> echo building bounce.exe...
* 相当于：
* bounce.exe: jump.obj up.obj
* <tab> echo building bounce.exe...
* 多个目标共享相同的依赖条件相当于用相同的依赖条件单独定义，而命令部分只属于当前规则中的目标
* leap.exe bounce.exe: jump.obj
* bounce.exe climb.exe: up.obj
* <tab> building bounce.exe...
* 相当于：
* leap.exe: jump.obj　# 目标 leap 没有命令部分
* bounce.exe: jump.obj up.obj
* <tab> building bounce.exe...
* climb.exe: up.obj
* <tab> building bounce.exe...
* 依赖条件在工作目标之后以冒号（:）分隔，依赖条件可以有0个或多个且不区分大小写，依赖条件可以指定文件名或伪目标
* 一般情况下，NMAKE 会在当前目录下查找依赖文件，但可以使用 {dir;dir2}dependent 形式的语法指定搜索路径
* NMAKE 在执行命令之前会打印命令，除非使用了 /S、.SILENT、!CMDSWITCHES、或@，另外-command或-n command可以忽略非0或大于n的返回错误码
* 命令部分可以包含一条或多条命令，每条命令占据单独一行，在规则和命令之间不能出现空行（命令和命令之间可以），仅包含空格和tab的行是空命令不是空行
* 一个命令行以1个或多个空格或tab开始，续行使可以使用反斜杠加换行符（会被解析成空格），但注意反斜杠之后不能有任何除换行之外的字符
* 单独一条命令可以出现在依赖条件之后，中间使用分号（;）进行分隔，例如 project.obj: project.c project.h ; cl /c project.c
* NMAKE 使用 name=value 形式定义变量，可以包含字母、数字、和下划线，最多1024个字符，名字中可以包含变量，但必须是已经定义的且不能为空
* 变量的名字区分大小写，值部分可以包含0个或多个任意的字符，如果包含0个或仅包含空格和tab，则这个变量的值为空，包括未定义和为空的变量都可以用在值中
* 未定义或值为空的变量会被解析成空串，变量的定义必须单行其顶格出现，不能以空格或tab开头，另外等号两边的空格和tabs会被忽略
* 但命令选项等号两边不能有空格或tab，例如 /D=a 选项中如果出现空白会解析成多个选项，空白在命令行中是选项的分隔符，如果选项值包含空白需使用引号括起
* NMAKE 中同名变量的优先级为（从高到低）：NMAKE 命令行定义、make 文件或包含文件中定义、环境变量、Tools.ini中定义、预定义变量
* 变量通过 $(name) 的形式进行引用，括号中不允许使用空格，如果名字只有一个字符可以不使用括号，引用变量之后，变量的值会替换到变量引用的地方
* NMAKE 定义的特殊变量包括：%@ 表示当前目标文件名，$? 时间戳在目标文件之后的所有依赖文件，这两个变量的含义与 GNU MAKE 中的含义相同
* $* 表示不包含后缀名的当前目标文件名，例如 dir/file、dir/file.txt 的 $* 值为 dir/file，注意 GNU MAKE 也有这个变量但只能使用在模式规则中
* $< 表示时间戳在目标文件之后的依赖文件，只能使用在模式规则中，注意 GNU MAKE 也有这个变量但表示依赖条件中的第一个文件
* $** 表示当前目标中的所有依赖文件，与 GNU MAKE 中的 $^ 相同；另外可以结合前面变量一起使用的 D 和 F 表示目录和文件名，与 GNU MAKE 一样
* 与 GNU MAKE 不一样的是 B 和 R，如果当前目标文件为 C:\folder\file.h，则 $(@B) 表示 file，而 $(@R) 表示 C:\folder\file
* $(MAKE) 相当于执行 namke，$(MAKEDIR) 表示 nmake 执行的当前目录，$(AS) 表示 ml（macro assembler）
* $(BC) 表示 bc（basic compiler），$(CC)、$(CPP)、$(CXX) 都表示 cl，$(RC) 表示 rc（resource compiler）
* NMAKE中的模式规则采用 GNU MAKE 中老式的后缀规则进行定义，例如:
* .c.obj:  # 定义了 .c 文件如何转换成 .obj文件，这些后缀名必须定义在 .SUFFIXES 规则中
* <tab> command
* 可以使用大括号指定文件的搜索路径，只能指定一个目录，如果其中一个指定了路径另一个文件类型也必须指定，可以使用 {.} 或 {} 表示当前目录，例如：
* {misc\}.c{$(OBJDIR)}.obj::    　　　　　　# ::形式的模式规则可以提高编译效率，在生成文件时 NMAKE 可以决定将所有如 .c 文件一次性编译成 .obj
* <tab> $(CC) $(CFLAGS) $<      　　　　　　# 如果使用 : 形式，每次只能编译一个 .c 文件
* {misc\}.cpp($(OBJDIR)}.obj::　　　　　　　　# 预定义变量 $< 只能使用在模式规则中，表示时间戳在目标文件之后的依赖文件
* <tab> $(CC) $(CFLAGS) $(YUPDB) $<　　　# 在模式规则中，如示例中的 .c 或 .cpp 是依赖文件，而 .obj 是目标文件
* NMAKE 定义了很多隐含的模式规则，使得其中的一类文件可以自动转换成另一类文件，可以使用 nmake /p 查看这些隐含模式规则
* NMAKE 定义的预处理命令有：!if/!ifdef/!ifndef !else !endif !include 等，这些命令都必须以 ! 开头，且 ! 之前不能有空白（但后面可以有）
* 预处理命令中可以使用的操作符有：defined(macro)、exist(file)、！、==、!=、>、>=、<、<=、&&、&#124;&#124（逻辑或）
* 整数操作符 +、－、*、/、%、<<、>>、&、^^（异或）、&#124（位或）、~　等，条件表达式中可以包含变量引用、字符串、整数常量等
* NMAKE 和 GNU MAKE 对比：
* !message hello          $(info hell)
* !error message          $(error message)
* !include file           include file
* nmake /n （也可以使用-n）  make -n  # 仅显示命令不执行
* nmake /p （也可以使用-p）  make -p  # 打印预定义变量和规则
* nmake /f （也可以使用-f）  make -f  # MAKE 指定的文件
CL编译器选项
* CL [option...] file... [option | file]... [lib...] [@command-file] [/link link-opt...]
* 编译选项列表：https://msdn.microsoft.com/en-us/library/19z1t1wy.aspx
* 编译器控制的链接选项：https://msdn.microsoft.com/en-us/library/92b5ab4h.aspx
* 代码优化技巧：https://msdn.microsoft.com/en-us/library/eye126ky.aspx
* 如何使用优化编译选项：https://msdn.microsoft.com/en-us/library/ms235601.aspx
* 要查看程序的执行性能，可以使用程序性能跟踪工具 perfmon.exe
* CL 编译完后会自动进行链接，除非使用了 /c 选项，在链接时可以通过 /link 修改链接选项，另外 /E、/EP、/P 只预处理不进行编译
* /D 定义宏、/U 取消宏定义、/I 包含头文件搜索路径、/LD 创建动态链接库、/LDd 创建 debug 版本的动态链接库
* /MD 创建多线程动态链接库（使用 MSVCRT.lib），/MDd 创建 debug 版本的多线程动态链接库（使用 MSVCRTD.lib）
* /MT 创建多线程可执行文件（使用 LIBCMT.lib），/MTd 创建 debug 版本的多线程可执行文件（使用 LIBCMTD.lib）
* /TC、/TP 认为指定的所有文件都是 C 或 C++ 源文件，/Tc 和 /Tp 可以单独指定一个 C 或 C++ 源文件
* /W0，/W1，/W2，/W3，/W4 警告等级，/Wall 开启所有的警告，/WX 把警告当成错误，/showIncludes 显示编译时所有包含的头文件
* /Fopathname 改变一个目标文件的输出路径和文件名，/Fepathname 改变一个可执行文件或动态链接库文件的输出路径和文件名
预定义宏
* http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
* http://beefchunk.com/documentation/lang/c/pre-defined-c/prestd.html
* https://sourceforge.net/p/predef/wiki/OperatingSystems/
持续集成
* jenkins官网：https://jenkins.io
* Ubuntu安装步骤（https://wiki.jenkins-ci.org/display/JENKINS/Installing+Jenkins+on+Ubuntu）
* $ wget -q -O - https://pkg.jenkins.io/debian/jenkins.io.key | sudo apt-key add -
* $ sudo sh -c 'echo deb http://pkg.jenkins.io/debian-stable binary/ > /etc/apt/sources.list.d/jenkins.list'
* $ sudo apt-get update
* $ sudo apt-get install jenkins
* 启动jenkins: sudo /etc/init.d/jenkins start (jenkins默认运行在 localhost:8080 主机端口上)
* 关闭jenkins: sudo /etc/init.d/jenkins stop
* jenkins的log保存在 /var/log/jenkins/jenkins.log 文件中，jenkins的配置可以修改 /etc/default/jenkins 文件
* jenkins的默认安装目录为：/var/lib/jenkins/
* jenkins war 文件的默认位置为：/usr/share/jenkins/jenkins.war
命令脚本
VIM编辑器
* 普通模式（normal mode）
* h/j/k/l       向左/下/上/右移动一个位置
* H             移动到屏幕最上面一行开头
* M             移动到屏幕最中间一行开头
* L             移动到屏幕最下面一行开头
* zt            将当前行移动到屏幕第一行
* zz            将当前行移动到屏幕最中央
* zb            将当前行移动到屏幕最下一行
* N<enter>      向下移动N行
* N+            向下移动N行
* N-            向上移动N行
* gg            移动到第一行开头
* G             移动到最后一行开头
* CTRL+f        移动到下一页
* CTRL+b        移动到上一页
* CTRL+d        向下移动半页
* CTRL+u        向上移动半页
* m<char>       用一个字符标记当前行
* '<char>       跳转到对应字符标记的行开头
* `<char>       跳转到对应字符标记所在行和列
* b             移动到当前单词开头字符，如果已在开头字符，移动到前一个单词的开头字符
* e             移动到当前单词结束字符，如果已在结束字符，移动到后一个单词的结束字符
* w             移动到下一个单词的开头字符
* ge            移动到上一个单词的结尾字符
* %             移动到下一个括号对的结束括号
* b/e/w/ge      基于单词移动位置
* B/E/W/gE      基于字串移动位置
* 0             移动到当前行开头字符
* $             移动到当前行结束字符
* ^             移动到当前行第一个非空白字符
* g_            移动到当前行最后一个非空白字符
* ga            查看当前字符的字符编码
* j/k/0/^/$     基于实际行移动位置
* gj|gk/g0/g^/g$   基于屏幕行移动位置
* f             在当前行查找一个字符并移动到该字符，用逗号(,)查找前一个，用分号(;)查找后一个
* t             在当前行查找一个字符并移动到该字符的前一个字符上
* F/T           在当前行反方向查找一个字符并移动
* *             查找当前单词的下一位置，并移动到该位置单词的开头字符，使用n查找下一个，N查找上一个
* #             查找当前单词的上一个位置
* x             删除当前字符并保持在普通模式
* X             删除前一个字符并保持在普通模式
* r{char}       替换当前字符
* u             撤销上一个修改
* CTRL+r        重做下一个修改
* yy            复制当前行
* p             将内容粘贴到下一行
* P             将内容粘贴到上一行
* dd            删除当前行
* .             在当前位置重复上一次在插入模式中的操作，但在插入模式中移动光标会重置修改状态
* i             进入插入模式，插入的字符会在当前字符之前
* I             进入插入模式，并移动到当前行首，相当于^i
* a             进入插入模式，并移动到当前字符之后
* A             进入插入模式，并移动到当前行尾，相当于$a
* o             进入插入模式，并移动到当前行尾，插入一个换行
* O             进入插入模式，并移动到当前行首，插入一个换行
* C             删除当前字符以及到行尾的所有字符，最终处于插入模式
* D             删除当前字符以及到行尾的所有字符，最终处于普通模式
* 插入模式（insert mode）
* CTRL+n/CTRL+p 显示提示列表，并在列表中进行选择
* CTRL+w        删除前一个单词
* CTRL+u        删除至行首
* CTRL+o        退出插入模式执行一条普通模式命令并返回插入模式
* CTRL+r0       粘贴当前寄存器的内容
* CTRL+r=3*4    插入一个计算结果
* 可视模式（visual mode）
* v             从普通模式进入字符可视模式，可以使用h/j/k/l/b/e等进行选取
* V             行模式
* CTRL+v        列模式
* o             切换到当前选取块的开头字符或结束字符
* 命令模式（command mode）
* :w            把缓冲区中的内容写入文件
* :wa!          把所有缓冲区中的内容写入文件
* :e!           重新读取文件到缓冲区，即回滚所有操作
* :qa!          关闭所有窗口，摒弃所有修改
* :saveas file  文件另存为
* :e file       在当前窗口载入新文件
* :e .          打开文件管理器，并显示当前目录文件
* :N            移动到第N行开头
* :$            移动到最后一行开头
* :!shell       执行shell命令
* :ls           列出VIM当前管理的所有文件缓冲区，其中标有%的文件是当前可见的文件
* :bn           切换到下一个文件
* :bp           切换到上一个文件
* :bf           切换到第一个文件
* :bl           切换到最后一个文件
* :bN           切换到第N个文件
* :b name       切换到包含名称的文件，可用TAB补全
* :bd N1 N2 N3  移除缓冲区中的文件
* :N,M bd       通过范围移除缓冲区中的文件
* :args         显示VIM当前的文件参数列表
* :args file    清空并重置参数列表，没有在文件缓冲区的文件会加到缓冲区
* :args `shell` 用shell命令返回的文件重置参数列表
* :args *.*     用批量文件重置参数列表，不会递归子目录
* :args **/*.*  通配符**会递归子目录
* :n            切换到参数列表的下一个文件
* :p            切换到参数列表的下一个文件
* :tabe file    在新标签页中打开文件
* :tabc         关闭当前标签页和其中的所有窗口
* :tabo         关闭其他所有标签页
* :tabn         切换到下一个标签页
* :tabp         切换到上一个标签页
* :tabnN       切换到第N个标签页
* :tabm0       将当前标签页移动到开头
* :tabm         将当前标签页移动到结尾
* :tabmN       将当前标签页移动到第N位置
* :sp file      水平切分当前窗口并载入文件
* :vsp file     垂直切分当前窗口并载入文件
* :clo          关闭当前窗口
* :on           关闭其他所有窗口
* CTRL+ww       循环切换窗口
* CTRL+w[hjkl]  切换到对应方向的窗口
* :set nu       显示行号
* :set nonu     取消行号
* :Nt.          将第N行的内容拷贝到当前行之下
* :t.           将当前行的内容拷贝到当前行之下
* :NtM          将第N行的内容拷贝到第M行之下
* :N,MtL        将范围内的行或高亮选取的内容拷贝到第L行之下
* :NmM          将第N行的内容剪切到第M行之下
* :N,MmL        将范围内的行或高亮选取的内容剪切到第L行之下
* :normal cmd   执行普通模式下的命令
浮点标准
* https://en.wikipedia.org/wiki/IEEE_754-1985
* https://en.wikipedia.org/wiki/IEEE_floating_point#IEEE_754-2008
* http://steve.hollasch.net/cgindex/coding/ieeefloat.html
* 浮点数包含3个部分：符号位（sign）、指数部分（exponent）、尾数部分（mantissa），尾数由小数部分（fraction）以及隐含的开始比特组成
* 单精度（32-bit）和双精度的浮点数的结构如下，方括号内的数字表示比特位范围
* |                  | Sign   | Exponent  (Bias) | Fraction   | Precision
* | single precision | 1 [31] |  8 [30-23]  127  | 23 [22-00] | approx. ~7.2 decimal digits
* | double precision | 1 [63] | 11 [62-52] 1023  | 52 [51-00] | approx. ~15.9 decimal digits
* 最高位为符号位，0表示正数，1表示负数，翻转这一比特可以翻转数字的符号
* 中间为指数部分（基数2是默认的），它通过减去一个偏差（Bias）来实现正负数的表示，例如单精度浮点指数部分的值为100则实际值为100-127=-27
* 指数部分全0（单精度表示-127，双精度表示-1023）和全1（单精度表示255-127=128，双精度表示2047-1023=1024）是特殊值
* 尾数部分是浮点有效位表示精度，它采用标准形式（将小数点放置在第1个非零比特之后），因此开始比特始终是1无需保存，只需存储小数部分（fraction）
* 正常浮点数：指数部分不为全0也不为全1，以单精度为例它表示的数字是 (-1)^sign × 1.fraction × 2^(exponent-127)
* 非规范化数（denormalized number）：指数部分为0但小数部分不为0，以单精度为例它表示的数字是 (-1)^sign × 0.fraction × 2^(-126)
* 零：指数部分和小数部分都为0，虽然正零（+0）和负零（-0）是相区别的两个数字但它们相等，零是一个特殊的非规范化数
* 无穷大：指数部分全1并且小数部分为0，根据符号位的不同有正无穷（+infinity）和负无穷（-infinity）
* NaN（not a number）：指数部分全1并且小数部分不为0，可用于表示计算结果不是一个数字
* 其中QNaN（Quiet NaN）小数部分最高位为1，表示非法的运算结果；SNaN（Signaling NaN）小数部分最高位为0，用于在运算时引发异常
* 浮点数的特殊操作：任何与NaN进行运算的结果都是NaN，其他的特殊操作如下
* n ÷ ±Infinity = 0， ±Infinity × ±Infinity = ±Infinity， ±Nonzero ÷ 0 = ±Infinity, Infinity + Infinity = Infinity
* ±0 ÷ ±0 = NaN， Infinity - Infinity = NaN， ±Infinity ÷ ±Infinity = NaN， ±Infinity × 0 = NaN
MKD解析细节
* 段落行行尾如果有两个或多个空格，会插入<br>；而GitHub评论会在行尾自动插入<br>
* 连接描述行中可选的title属性值可以放到第二行，该属性值包含在""或''或()内
* 文本按行分割，文本行的类型有：缩进行（indentline）、标题行（atxline）、第二种标题行（stxline）、分隔行（horzline）
* 链接描述行（referline）、代码块起始结束行（fencedline）、表格行（tableline）、引用行（quoteline）
* 列表行（listline）、空白行（blankline）、段落行（paraline）
* 其中标题行、分隔行、链接描述行单独形成自己的内容块，例如解析为标题的标题行包含了标题的所有内容
* 其他内容块则包含多行对应类型的文本行，这些内容块有：缩进块（INDE）、代码块（FENC）、表格（TABL）、引用块（QUOT）、列表（LIST）、段落（PARA）
* 缩进块可包含缩进行和空白行，代码块可包含除代码块结束行外的任意行，表格仅包含表格行，引用块和列表块包含任意行，段落可包含段落行、空白行和缩进行
* 文本行能够开启对应内容块的规则如下：
* indentline是否开启INDE块
* 当前是FENC块: 成为FENC块内容
* 当前是QUOT块: 前一行是空行才开启INDE块
* 当前是LIST块：作为LIST块内容
* 当前是INDE块：追加内容
* 当前是TABL块: 开启缩进块
* 当前是PARA块：前一行是空行才开启INDE块
* fencedline是否开启FENC块（FENC块只能被fencedline或文件结束关闭）
* 当前是FENC块：追加内容
* 当前是QUOT块：前一行是空行才开启FENC块
* 当前是LIST块：前一行是空行才开启FENC块
* 当前是INDE块：开启FENC块
* 当前是TABL块：开启FENC块
* 当前是PARA块：开启FENC块
* quoteline是否开启QUOT块（QUOTE块的内容在去掉开始格式符之后需递归解析）
* 当前是FENC块：成为FENC块内容
* 当前是QUOT块：追加内容
* 当前是LIST块：前一行是空行才开启QUOT块
* 当前是INDE块：开启QUOT块
* 当前是TABL块：开启QUOT块
* 当前是PARA块：开启QUOT块
* listline是否开启LIST块（LIST块的内容在去掉开始格式符之后需递归解析）
* 当前是FENC块：成为FENC块内容
* 当前是QUOT块：前一行是空行才开启LIST块
* 当前是LIST块：追加内容
* 当前是INDE块：开启LIST块
* 当前是TABL块：开启LIST块
* 当前是PARA块：开启LIST块
* tableline/paraline/atxline/stxline/horzline/referline是否开启TABL块/PARA块/标题／垂直分隔／链接表述
* 当前是FENC块：成为FENC块内容
* 当前是QUOT块：前一行是空行才开启PARA块
* 当前是LIST块：前一行是空行才开启PARA块
* 当前是INDE块：开启
* 当前是TABL块：开启，tableline作为TABL块内容追加
* 当前是PARA块：开启，paraline作为PARA块内容追加
* 遇到blankline的处理
* 当前是FENC块：作为其内容
* 当前是QUOT块：作为其内容
* 当前是LIST块：作为其内容
* 当前是INDE块：作为其内容
* 当前是TABL块：关闭TABL块
* 当前是PARA块：作为其内容
```

Lua
```
基本概念
* LUA 的关键字包括 true false nil and or not function local return if else elseif then end while repeat until for in do break goto
* 另外还应该避免使用以下划线开始后跟一个或多个大写字母的名字，例如 _VERSION
* LUA 使用的操作符号包括　+ - * / % ^ # & ~ | << >> // == ~= <= >= < > = ( ) { } [ ] :: ; : , . .. ...
* 特殊字符包括 "\a"（bell）、"\b"（backspace）、"\f"（form feed）、"\n"（newline）、"\r"（carriage return）
* "\t"（horizontal tab）、"\v"（vertical tab）、"\\"（backslash）、"\""（double quote）、"\'"（single quote）
* "\z" 可用于忽略后续包括换行符在内的空白字符，它在书写无换行的长字符串时特别有用；另外反斜杠跟随换行符可用于续行，其本身在字符串中表示一个换行
* "\xXX" 插入一个十六进制字节字符；另外可以使用 "\u{uuuu...}" 来插入一个 UTF8　字符，其中 uuuu... 是该字符的 UNICODE 代码点
* LUA 的短字符串包含在双引号或单引号内，而长字符串的表示形式为 [[string]]、[=[string]=]、[==[string]==]、...
* 长字符串不解析任何转义字符，可以跨多行书写，其中的换行、回车、换行加回车、回车加换行被转换成换行，但如果第一个字符就是换行的话会被忽略
* LUA 的短注释以 -- 开始直到行结尾，长注释的形式为 --[[comment]]、--[=[comment]=]、--[==[comment]==]、...
* Lua 基本类型包括 nil，boolean，number，string，function，userdata，thread，table，没有赋初始值的变量的值为 nil
* 变量类型可以通过 type(var) 判断，它返回对应类型名称的字符串；所有类型值只有 nil 和 false 为假，其他值都为真
* nil、boolean、number、string、light userdata 是值类型，function、full userdata、thread、table 是引用类型
* 自动类型转换：位操作会将浮点转换成整数，幂操作和浮点除法会将整数转换成浮点，其他算术运算如果包含浮点和整数会将整数转换成浮点
* 字符串连接操作符可以连接数字和字符串，如果需要数字的地方传递了字符串，Lua 也会尝试将字符串转换成数字
* 整数自动转换成浮点会使用最接近的浮点表示，当浮点自动转换成整数时，如果浮点整数部分在整数表示范围内则成功，否则失败
* 当字符串自动转换成数字时，首先根据实际的字符串转换成整数或浮点（字符串前后可以有空格），然后根据上下文需要可能继续转换成整数
* 关系操作符包括：等于（==），不等于（~=），小于（<），小于等于（<=），大于（>），大于等于（>=）
* 相等操作先比较两个操作数的类型，如果类型不同则不相等，否则值类型进行值比较，引用类型比较引用（table 和 userdata 可通过元函数改变相等操作的行为)
* 大小关系运算可以比较两个数字或两个字符串，其他类型会尝试调用元函数 lt 或 le；根据 IEEE 754标准，特殊值 NaN 不大于、不小于、不等于任何值（包括它自身）
* load 函数加载 LUA 代码时会在全局环境中编译对应的代码块，这是它与真实函数调用的区别，例如：
* i = 32; local i = 0; f = load("i = i + 1; print(i)"); g = function() i = i + 1; print(i) end f() --[[33]] g() --[[1]]
* LUA 通过 error 抛出异常，通过 pcall 调用函数来捕获函数中的异常，当异常发生时栈会展开异常沿栈向上抛出直到 pcall 捕获
* 当 pcall 捕获异常返回错误时，栈现场已经破坏了，如果想获取异常时的栈信息需要使用 xpcall，它提供额外回调函数可在异常发生时使用debug库保存栈信息
模块
* 全局函数 require(modname) 用于加载模块、执行模块代码、获取模块返回值，并在加载模块之前根据名称查找模块，一般查找流程如下：
* 首先看模块是否已经加载（通过查看 package.loaded），是则直接返回其中的值，否则使用 package.searches 中保存的查询函数进行查询
* 通过修改 packages.searches 可以改变模块的查找方式，它默认有4个查询函数，require 会使用模块名依次调用这些函数查询
* 第1个函数调用 package.preload[modname] 加载模块（如果存在），第2个函数查找 package.path 中的 Lua 模块，第3个函数查找 package.cpath 中的 C 模块
* 第4个函数会在 package.cpath 中查找模块的 root 名称对应的模块，例如 a.b.c （在第3个函数失败后）会查找模块 a，再执行其中的加载函数 luaopen_a_b_c，该功能允许多个 C 模块打包在一个库中
* Lua 模块的加载是直接执行文件中的代码，而 C 模块会先进行动态链接，然后调用其中的加载函数（luaopen_xxx），例如模块 a.b.c-v2.1 的加载函数是 luaopen_a_b_c（不包含后缀）
* 文件或动态库的查找是通过 package.searchpath(name, path[, sep[, rep]]) 完成的，它在指定路径中查找对应名称的文件，名称中的分割符 sep（默认是点号）首先会替换成 rep（默认是斜杠）
* 例如在 package.path 中查找时如果路径是 ./?.lua;/usr/local/?/init.lua，则查找 a.b.c 会依次尝试 ./a/b/c.lua;/usr/local/a/b/c/init.lua
* 而在 package.cpath 中查找时如果路径是 ./?.so;./?.dll;/usr/local/?/init.so，则查找 a.b.c 会依次尝试 ./a/b/c.so;./a/b/c.dll;/usr/local/a/b/c/init.so
执行环境
* Lua 执行环境涉及　_ENV 和　_G 两个变量，_ENV　表示当前执行环境，而 _G 表示全局执行环境，全局环境是唯一的，它保存了 Lua 定义的所有全局符号（例如 print）
* 实际上，名称 _G 只是全局环境（一个 table）中保存的一个变量，这个变量引用全局环境本身，当前环境 _ENV 一般情况下指向全局环境
* 没有使用 local 定义的且不是函数参数的变量是全局变量，全局变量会保存在当前环境 _ENV 中，如果 _ENV 指向全局环境，实际上会保存在全局环境中，如果不指向则不会
* 全局变量可以通过 _ENV.name、_ENV[expr]、_G.name、_G[expr] 显式访问，实际上 Lua 会将自由变量如 x 转换为 _ENV.x
* _ENV 是一个局部变量，Lua 在编译代码块（chunk，Lua 的编译单元）时会首先定义这个变量，例如对代码 local z = 10; x = y + z 的编译结果为：
* local _ENV = <the global environment>; return function(...) local z = 10; _ENV.x = _ENV.y + z end
* 由于全局变量的定义不需要显式声明，在代码中很容易出错（例如将局部变量的名字不小心写错），可以通过元表对全局变量的使用做一些限制
* _ENV 是一个普通的变量，遵循 Lua 的作用域规则，也可以随意修改 _ENV，对 _ENV 的引用总是引用当前作用域中可见的 _ENV
* 但修改 _ENV 时需要注意它指向的全局环境中保存有 Lua 的全局符号，为 _ENV 赋新值会导致这些符号（如 print）不可用，但也可实现对全局符号的访问限制
* 例如：local print = print; _ENV = nil; print(13); print(math.sin(13)) -- error, math is not defined
* 使用 _ENV 或 _G 可访问到被局部变量覆盖的全局变量：a = 13; local a = 12; print(a); print(_ENV.a); print(_G.a)
* 修改 _ENV 改变当前环境，例如赋予 _ENV 一个新的 table，代码块中的全局变量将会保存到这个新的环境中，而不会污染全局环境
* 但为了访问 Lua 的全局符号，可先将全局环境或部分用到的全局符号保存到 table 中，例如：_ENV = {g = _G}; a = 1; g.print(a)
* load() 和 loadfile() 在一般情况下将 _ENV 初始化为 _G，但是它们提供了额外的参数用于给 _ENV 赋值，例如：
* width = 200; height = 300; --[[ file 'config.lua' ]] local env = {}; loadfile("config.lua", "t", env)()
* 此时外部文件中的代码就像在沙盒（env）中运行一样不会影响代码的其他部分，也会隔离代码错误或恶意代码的侵害
* 另一种情况是让同一段代码执行多次，每次在不同的环境中运行，一种方法是使用 debug.setupvalue()：local f = load("b = 10; return a");
* local env = {a = 20}; debug.setupvalue(f, 1, env); --[[ the chunk have only 1 upvalue]] print(f(), env.b); -- 20  10
* 该方法唯一的缺点是使用了 debug 库，该库会打破 Lua 的一些语言规则，例如违反 Lua 变量的可见性规则，这个规则保证局部变量仅在其作用域中可见
* 另一方法是添加额外代码将参数赋给 _ENV，例如：local f = loadEx("_ENV = ...", io.lines(filename, "*L")); f(env1); f(env2)
* Lua 解释器可以执行一段代码 lua -e "code"，也可以执行保存在文件中的代码 lua file.lua arg
协程和线程
* 协程的创建仅需传入协程的主函数 local co = coroutine.create(luafunc)
* 协程有四种不同的状态：suspended，running，normal，dead
* 新创建的协程初始状态为suspended，协程状态可以通过函数coroutine.status(co)获取
* 调用函数coroutine.resume(co)可以恢复suspended协程的执行，使其进入running状态，只有suspended状态的协程才能resume
* 恢复运行的协程要么从主函数返回，要么再次suspended，从主函数返回的协程会进入dead状态，表示协程完全运行完了不能再次resume
* resume函数像pcall函数一样在保护模式执行，不会抛出错误只会返回错误码
* 如果运行中的协程resume另外一个协程运行，它自己的状态将变成normal，normal状态也是不能resume的，因为实际上它正在运行中
* 传给resume的参数会被主函数当做参数（第一次resume时），或被yield接收作为yield函数的返回值
* resume函数的第一个返回是表示是否调用成功的状态值，之后的返回值从主函数或yield函数接收而来
* status, value, value2, ... = coroutine.resume(co)
* 当resume的协程从主函数返回时，主函数的返回值将作为resume的返回值返回
* 当resume的协程suspended而返回时，传入yield的参数将作为resume的返回值返回
* 生产者消费者模型，下面是一个消费者驱动的例子（调用消费者函数驱动生产者生产产品）
* local producer = coroutine.create(function()
*   while true do
*     local x = io.read() -- produce a new value
*     coroutine.yield(x)  -- send it to customer
*   end
* end)
* local consumer = function(prod)
*   while true do
*     local _, x = coroutine.resume(prod) -- receive value from producer
*     io.write(x, "\n")                   -- consume it
*   end
* end
* 不仅如此，在生产者和消费者之间还能实现一个产品过滤层，只将符合条件的产品才提交给消费者
* local prodfilter = function(prod)
*   return coroutine.create(function()
*     while true do
*       local _, x = coroutine.resume(prod) -- receive value from producer
*       if x fulfil the condition then      -- but only the value that meet the condition
*         coroutine.yield(x)                -- is send to customer
*       end
*     end
*   end)
* end
* consumer(producer)               -- get all the products
* consumer(prodfilter(producer))   -- get products only meet the condition
* 协程有4种状态：suspended，running，normal，和 dead；可以通过 coroutine.status(co) 获取协程状态；协程的魔法来自于能在函数中调用 yield
* 例如 local co = coroutine.create(function() for i = 1, 2 do print(i); coroutine.yield() end end)
* coroutine.resume(co) --[[1]] coroutine.resume(co) --[[2]] coroutine.resume(co) -- print nothing
* 注意 resume 运行在保护模式中（像 pcall），协程中的错误不会被抛出，而是以 resume 的返回值返回
* 当协程去 resume 另一个协程时，另一协程会进入 running 状态，而当前协程状态会变成 normal，当 resume 返回后当前协程再次变成 running
* Lua 协程的一个有用的特性是 resume-yield 可以相互传递数据，例如 local co = coroutine.create(function(a,b) --[[see below]] end)
* print(a,b); a, b = coroutine.yield(a, a+b); print(a,b); return 0, 1
* 第一次 resume 例如 local a, b = coroutine.resume(co, 1, 2) 会打印 1 和 2，并且 resume 的返回值是 1 和 3
* 第二次 resume 例如 local a, b = coroutine.resume(co, "a", "b") 会打印 "a" 和 "b"，并且 resume 的返回值是 0 和 1
* 协程的应用场景一是生产者-消费者模式：function producer() while true do send(io.read()) end end
* function consumer() while true do io.write(receive(), "\n") end end
* producer = coroutine.create(producer); function send(x) coroutine.yield(x) end
* function receive() local status, value = coroutine.resume(producer); return value end
* 协程应用场景二是用于迭代遍历：function find(a) for i = 1, 10 do if a[i] > 0 then coroutine.yield(a[i]) end end
* function iter(a) return function() local _, res = resume(create(function() find(a) end)); return res end
* 然后可以在 for 循环中使用上面的遍历函数 iter 进行遍历，例如：for v in iter(a) do print(v) end
构建并发模型
* 任务（task）由一段代码（对应一个 coroutine）和一个句柄标识，一个任务可以给任何任务发送消息，包括其他进程中的任务
* 一个任务应该编写得与线程无关（可以由任何线程执行），一个任务需要处理的工作包括：
* 1. 执行自己的核心业务，当核心业务等待时检查自己的消息队列处理消息（计时器超时也会发送消息到队列中）
* 2. 如果核心业务在等待且所有的消息已处理完，则挂起当前执行线程（为防止其他任务饥饿，可以考虑一些策略在某些情况下提前挂起）
* 有一个特殊的任务是 main task，它负责检查全局消息队列中是否有消息，有则分配线程执行对应任务去处理消息（在该线程上运行的任务会执行上面的步骤1和2）
* 主任务还需要监控计时器超时，如果超时则发送一条消息到对应任务的消息队列；如果下一超时时间还较长（或都超时了）且消息处理完毕则等待操作系统事件一段时间（如socket)
* 如果所有线程都处于忙状态且还有对应的工作需要处理，且该工作对应的任务没有在任何线程内运行，主任务可以帮助先处理该工作，另外主任务还需要负责 log 输出
* 属于一个任务的两项工作不能同时在不同的线程中运行，但是多个任务可以在同一个线程中运行（因为一个任务对应一个协程）
* 操作系统的线程资源是有限的，而客服的请求相对与线程数量来说可以说是无限的
* 因此一个线程必须同时处理多项客户请求任务，任务的分配会根据当时各个工作线程的负载情况进行分配
* 一个任务分配给特定的线程后，一般自始至终都在这个线程中完成，不再线程之间来回切换
* 线程只是基础设施，而任务是各式各样的，不同的任务需要处理的事务可能大不相同
* 为了简化基础设施处理流程，也为了增强基础设施的适用性，可以将任务处理事务的流程标准化
* 基础设施在接收到任务时，只需按照该任务指定的事务流程处理即可
---
* LUA 协程是一种非抢占协作线程，每个协程都拥有自己独立的栈内存，LUA 协程与操作系统线程是两个不同的概念
* 操作系统级线程是抢占式的（优先级高的抢占），多个线程共享当前进程内存，需要同步机制解决内存访问不一致问题
* 在 LUA 的 C API 层面可以认为协程相当于一个栈，它保存着协程挂起的调用信息以及每个调用的参数和局部变量信息，即协程栈保存了其继续执行需要的所有信息
* LUA 的 C API 都需要在一个特定的栈来进行操作，它会使用哪个协程的栈呢？这里的魔法是每个 C API 函数的第一个参数都是 lua_State 指针
* lua_State 表示的不仅仅是 LUA 状态，还表示一个协程，在 LUA 程序开始执行时会创建一个 LUA 状态和主协程
* 对于不关心多协程的程序，其所有代码都在主协程中运行，要创建多个协程需要使用函数 lua_newthread，例如 lua_State* L1 = lua_newthread(L)
* 此时我们拥有了两个协程 L1 和 L，每个协程拥有自己独立的协程栈，但 LUA 状态是共享的，都指向程序开始运行时创建的 LUA 状态
* 新协程 L1 创建后其栈中没有保存任何元素，而协程 L 栈顶保存了一个指向 L1 的引用，这是为了防止 L1 被回收
* 在 C 中使用新协程时，必须注意确保将新协程的引用保存到了已保存的协程的栈中、LUA 注册表中、或 LUA 变量中，否则新线程有被回收的危险
* 注意保存到 C 变量中没有作用，另外当 LUA 对象被置为可回收后对任何 LUA API 的调用都可能引发回收动作，即使通过这个协程进行调用
* 例如 lua_State* L1 = lua_newthread(L); lua_pop(L, 1); /* L1 now is garbage for Lua */
* 上面调用 lua_pop 后保存在 LUA 中该协程的唯一引用也删除了，该协程变成可回收垃圾，注意 LUA 不可能跟踪到 C 语言变量 L1 对该对象的引用
* 之后使用这个新协程都是错误的，例如 lua_pushstring(L1, "hello"); /* 可能导致 L1 被回收，然后程序崩溃 */
* 当拥有新协程后，就可以像主协程那样来使用，例如在它的栈中添加移除元素，通过它调用 LUA 函数等，但这都没必要创建新协程
* 创建新协程的意义是可以多个协程之间进行协作，开启协程的运行需要调用 int lua_resume(lua_State* L, lua_State* from, int narg)
* 首先需要将一个函数入栈，然后是函数的参数（narg是参数个数），最后调用 lua_resume，其中 L 是启动的新协程，from 是当前调用 lua_resume 的协程
* lua_resume 的调用非常类似 lua_pcall，只有3点不同，一是它没有参数指定想要的返回结果个数，它会返回所有结果
* 二是它没有参数来提供错误消息的处理，一个错误不会导致栈展开（即不会沿栈向上抛出异常），因此在错误发生后有机会检查错误发生时的栈现场
* 三是如果调用的函数被挂起（yield），lua_resume 会返回 LUA_YIELD，后面可以再次调用 lua_resume 从该挂起点继续执行该函数
* 当函数挂起返回时，协程栈中保存的返回值是 yield 函数中传入的所有参数，如果要将协程中的参数移到另一个协程中，可以使用函数 lua_xmove
* 再次调用 lua_resume 会继续执行挂起协程，栈中的参数将作为 yield 函数的返回结果，如果不对栈做操作 yield 函数得到的返回结果将是自己的参数
* 可以直接调用 LUA 函数作为协程函数，该 LUA 函数可以在内部挂起，或在其调用的函数中挂起，另外 C 函数也可以作为协程函数执行
* 当 C 函数作为协程函数执行时，C 函数可以调用 LUA 函数，使用 continuations 机制允许在这些 LUA 函数中进行挂起
* 一个 C 函数也可以挂起，但需要提供一个 continuation 函数在 lua_resume 中使用，要使 C 函数挂起时需要调用以下函数：
* int lua_yieldk(lua_State* L, int nresults, int context, lua_CFunction k);
* 而且必须总是在 return 语句中调用该函数，例如 int mycfunc(lua_State* L) { /* ... */ return lua_yieldk(L, nresults, ctx, k); }
* 其中 nresults 是当前栈中指定的作为 yield 函数的参数个数，这些参数会在协程挂起后作为 resume 函数的返回结果
* 而 k 是 continuation 函数，context 会作为参数传给函数 k，当协程挂起后再次启动时会调用函数 k 继续执行其中的代码
* 因此初始协程 C 函数不能做更多的事情，当它被挂起后，之后的代码必须放在 continuation 函数 k 中实现，因为再次启动协程时只会调用函数 k
* 下面是一个使用 C 函数作为协程主体的例子，它读取数据并在数据不可用时挂起：
* int prim_read(lua_State* L) { return readK(L, 0, 0); }
* int readK(lua_State* L, int status, lua_KContext ctx) { (void)status; (void)ctx; /* see below */ }
* if (something_to_read()) { lua_pushstring(L, read_some_data()); return i; } return lua_yieldk(L, 0, 0, &readK);
* 当 C 函数挂起后再次执行时没有事情要做了，可以不指定 k 函数来调用 lua_yieldk 或使用 lua_yield(L, nres)，当下次 resume 时会从函数返回 
* 在相同的 lua_State 中调用 lua_newthread 产生的协程都共享同一个 LUA 状态，只是每个协程会拥有自己独立的栈
* 而调用 luaL_newstate 或 lua_newstate 会创建不同的 LUA 状态，新的 LUA 状态会是完全独立的不共享任何数据
* 这意味着不同的 LUA 状态不能直接进行通信，必须借助 C 代码，也意味着只有那些能用 C 表示的数据才能直接传递（如字符串和数字），其他数据如表必须先序列化
* 在提供多线程的系统中，一个有趣的设计是为每个线程创建一个独立的 LUA 状态，这样每个线程相互独立且可拥有多个协程
```

LUA字符串
```
LUA初始化的字符串哈希表的大小为MINSTRTABSIZE即128
LUA字符串哈希表大小调整规则是元素个数大于等于哈希表大小时将哈希表扩大1倍（一个例外是哈希表大小已经大于等于MAX_INT/2了）
而当元素个数小于哈希表大小的1/4时，将哈希表缩小到原来的1/2
---
struct ccstringtable {
  struct ccsmplnode* slot;
  umedit_int nslot; /* prime number */
  umedit_int nelem; /* number of elements */
};
#define ccstring_newliteral(s) (ccstring_newlstr("" s, (sizeof(s)/sizeof(char))-1)
struct ccstring {
  union {
    struct ccstring* hnext; /* linked list for shrot string hash table */
    sright_int lnglen; /* long string length */
    struct cceight align; /* align for 8-byte boundary */
  } u;
  umedit_int hash;
  nauty_byte type;
  nauty_byte extra; /* long string is hashed or not, this string is reserved word or not for short string */
  nauty_byte shrlen; /* short string length */
　　nauty_char s[1]; /* the string started here */
};
---

LUA字符串哈希值的计算方法使用JSHash函数，并使用字符串长度l异或G(L)->seed作为哈希的初始值
并且不是字符串中的每个字符都用来计算哈希值，而是有一个step间隔，每隔多少个字符才取一个字符来计算哈希值
下面字符间隔的计算相当于（字符串长度/32）＋１，例如长度小于32将使用1个字符计算哈希值
长度在范围[32,64)内将将使用2个字符计算哈希值，长度在[64,96)内将使用3个字符计算哈希值，依次类推
如果长度时能是32位的整数的化，最多会使用134217727（1亿3）个字符来计算哈希值
旧版本没有使用G(L)->seed，存在Hash DoS，见http://lua-users.org/lists/lua-l/2012-01/msg00497.html
新版的G(L)->seed即保存在global_State中，这个种子的构造方法可查看函数makeseed()
---
unsigned int luaS_hash (const char *str, size_t l, unsigned int seed) {
  unsigned int h = seed ^ cast(unsigned int, l); /* 使用长度异或seed作为hash初始值 */
  size_t step = (l >> LUAI_HASHLIMIT) + 1; /* 计算取字符的间隔，LUAI_HASHLIMIT的值为5 */
  for (; l >= step; l -= step)
    h ^= ((h<<5) + (h>>2) + cast_byte(str[l - 1]));
  return h;
}
unsigned int JSHash(char *str) {
    unsigned int hash = 1315423911;
    while (*str) {
        hash ^= ((hash << 5) + (*str++) + (hash >> 2));
    }
    return (hash & 0x7FFFFFFF);
}
unsigned int BKDRHash(char *str) {
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;
    while (*str) {
        hash = hash * seed + (*str++);
    }
    return (hash & 0x7FFFFFFF);
}
---

哈希种子的初始化方法利用了各种内存地址的随机性以及用户可配置的一个随机量来初始化这个种子
---
#if !defined(luai_makeseed)
#include <time.h>
#define luai_makeseed()　cast(unsigned int, time(NULL))
#endif
#define addbuff(b,p,e) { size_t t = cast(size_t, e); memcpy(b + p, &t, sizeof(t)); p += sizeof(t); }
unsigned int makeseed (lua_State *L) {
  char buff[4 * sizeof(size_t)];
  unsigned int h = luai_makeseed();
  int p = 0;　/* 字符串的长度 */
  addbuff(buff, p, L);  /* heap variable */
  addbuff(buff, p, &h);  /* local variable */
  addbuff(buff, p, luaO_nilobject);  /* global variable */
  addbuff(buff, p, &lua_newstate);  /* public function */
  lua_assert(p == sizeof(buff));
  return luaS_hash(buff, p, h);
}
---

短字符串是否相等只需判断类型是否是短字符串并且指针指向的是否是同一个字符串对象
而长字符串的比较，如果指向同一个字符串对象当然也想等，如果不是则需要长度相同且内容一样
---
int luaS_eqlngstr(TString* a, TString* b) {
  size_t len = a->u.lnglen;
  lua_assert(a->tt == LUA_TLNGSTR && b->tt == LUA_TLNGSTR);
  return (a == b) ||  /* same instance or... */
    ((len == b->u.lnglen) &&  /* equal length and ... */
     (memcmp(getstr(a), getstr(b), len) == 0));  /* equal contents */
}
---

创建一个以str为内容的字符串，如果这个字符串在cached中直接返回
否则调用luaS_newlstr真正去生成一个新字符串，并将新生成的字符串cache到对应slot的第一个字符串
字符串缓存以字符串的首地址为键，哈希值的计算如 str%53
---
TString* strcache[STRCACHE_N][STRCACHE_M];  /* cache for strings in API 53*2 */
TString *luaS_new (lua_State *L, const char *str) {
  unsigned int i = point2uint(str) % STRCACHE_N;  /* hash */
  int j;
  TString **p = G(L)->strcache[i];
  for (j = 0; j < STRCACHE_M; j++) {
    if (strcmp(str, getstr(p[j])) == 0)  /* hit? */
      return p[j];  /* that is it */
  }
  /* normal route */
  for (j = STRCACHE_M - 1; j > 0; j--)
    p[j] = p[j - 1];  /* move out last element */
  /* new element is first in the list */
  p[0] = luaS_newlstr(L, str, strlen(str));
  return p[0];
}
---

如果字符串的长度不超过40，则创建一个短字符串，创建短字符串时首先看哈希表中有没有这个字符串
如果有直接返回这个字符串，否则调用createstrobj实际创建一个字符串并加它加到哈希表中
哈希表的大小总是2的倍数，即(size&(size-1))一定等于0，如果确定对应哈希值的字符串该存储在哈希表的那个位置呢？
LUA的计算方法是哈希值与上(size-1)，即 (h & (size-1)),它没有使用除以一个素数取余数的方法
例如size为128，二进制数为1000,0000，则(size-1)的值为0111,1111，与上哈希值h即得到哈希值的最低7位来作为存储位置
在大多数应用场合,长字符串都是文本处理的对象,不会做比较操作，这是将字符串分为短字符串和长字符串的一个原因
注意，LUA字符串一旦创建以后，是不可修改的
---
static TString *internshrstr (lua_State *L, const char *str, size_t l) {
  TString *ts;
  global_State *g = G(L);
　　/* 计算哈希值，并找到对应哈希表的slot，然后查看这个slot中有没有这个字符串，如果有直接返回该字符串 */
  unsigned int h = luaS_hash(str, l, g->seed);
  TString **list = &g->strt.hash[lmod(h, g->strt.size)];
  lua_assert(str != NULL);  /* otherwise 'memcmp'/'memcpy' are undefined */
  for (ts = *list; ts != NULL; ts = ts->u.hnext) {
    if (l == ts->shrlen &&
        (memcmp(str, getstr(ts), l * sizeof(char)) == 0)) {
      /* found! */
      if (isdead(g, ts))  /* dead (but not collected yet)? */
        changewhite(ts);  /* resurrect it */
      return ts;
    }
  }
　　/* 实际存储的元素大于等于表的大小则先扩展表大小（注意标的大小不能找过MAX_INT，因此表大小已达到MAX_INT时不能再扩张）*/
  if (g->strt.nuse >= g->strt.size && g->strt.size <= MAX_INT/2) {
    luaS_resize(L, g->strt.size * 2);
    list = &g->strt.hash[lmod(h, g->strt.size)];  /* recompute with new size */
  }
  /* 表中没有该字符串，新创建一个字符串，并将字符串插入哈希表 */
  ts = createstrobj(L, l, LUA_TSHRSTR, h);
  memcpy(getstr(ts), str, l * sizeof(char));
  ts->shrlen = cast_byte(l);
  ts->u.hnext = *list;
  *list = ts;
  g->strt.nuse++;
  return ts;
}
---

否则超过40就创建一个长字符串，然而长度也不能过分长，如果超过一定限度会报错
创建长字符串时，先调用luaS_createlngstrobj分配空间然后将字符串拷贝到空间中
长字符串不会放进哈希表，是一个普通的可垃圾回收的LUA对象
---
TString *ts;
if (l >= (MAX_SIZE - sizeof(TString))/sizeof(char))
  luaM_toobig(L);
ts = luaS_createlngstrobj(L, l);
memcpy(getstr(ts), str, l * sizeof(char));
return ts;
---

真正用于创建字符串对象的函数是createstrobj，l是字符串的长度，tag是表示字符串的类型
如果是短字符串，tag为LUA_TSHRSTR，哈希值h为已计算出的短字符串的哈希值
如果是长字符串，tag是LUA_TLNGSTR，哈希值存储用于计算哈希值的种子G(L)->seed
---
static TString *createstrobj (lua_State *L, size_t l, int tag, unsigned int h) {
  TString *ts;
  GCObject *o;
  size_t totalsize;  /* total size of TString object */
  totalsize = sizelstring(l);
  o = luaC_newobj(L, tag, totalsize);
  ts = gco2ts(o);
  ts->hash = h;
  ts->extra = 0;
  getstr(ts)[l] = '\0';  /* ending 0 */
  return ts;
}
---
```

网络套接字
```
* socket (socket.lua) => socketdriver (lua-socket.c) => skynet_socket.c => skynet_server.c
* httplisten.lua: fd = socket.listen(ip, port)
* socket.start(fd, function(fd, addr) { send msg to agent[i++]; })
* socket.start(fd) socket.read() socket.write()
* skynet套接字核心循环 main() => skynet_start(&config) => start(n) => thread_socket() => skynet_socket_poll() => socket_server_poll()
* skynet简单HTTP服务器流程:
readfunc(sock) return function (bytes) --[[socketreadfunc]] socket.read(sock, bytes) end end
writefunc(sock) return function (content) --[[socketwritefunc]] socket.write(sock, content) end end
-- read flow
httpd.read_requestx(readfunc(sock), bodybyteslimit)
pcall(readallx, socketreadfunc, bodybyteslimit)
readallx(socketreadfunc, bodybyteslimit)
  local headers = {}
  local body = recvheader(socketreadfunc, headers, "")
  if not body then return 413 --[[request entitiy too large]] end
  local method, url, httpver = get the first line information
  -- parse all headers out from received headers
  local header = parseheader(headers, 2, {})
  -- receive remaining content
  if mode == "chunked" then body, header = recvchunkedbody(socketreadfunc, bodybyteslimit, header, body)
  else body = body .. socketreadfunc(length - #body) end
end
-- socket.lua/lua-socket.c --
socket.read(sock, size)
-- write flow
response(sock, statuscode, bodyorfunc, tableofheaders) -- 
local ok, nerr = httpd.write_response(writefunc(sock), statuscode, bodyorfunc, tableofheaders)
pcall(writeall, sockwritefunc, statuscode, bodyorfunc, tableofheaders)
writeall(sockwritefunc, statuscode, bodyfunc, tableofheaders)
  local statusline = "HTTP/1.1 statuscode statusmsg\r\n"
  socket.write(sock, statusline) -- socketwritefunc(statusline)
  socket.write(sock, headers in the table)
  if bodyorfunc is a string then
    socket.write(sock, "content-length: bodylength\r\n\r\n")
    socket.write(sock, bodyorfunc)
  elseif bodyor func is a function then
    socket.write(sock, "transfer-encoding: chunked\r\n")
    socket.write(sock, "\r\n{ %x | #s }\r\n{ %s | s }") -- s = bodyfunc() until s is nil, then:
    socket.write(sock, "\r\n0\r\n\r\n")
  else
    socket.write(sock, "\r\n")
  end
end
-- socket.lua/lua-socket.c --
socket.write(sock, content)
socketdriver.send => lsend(lua_State* L) {
  skynet_context* ctx = the first upvalue;
  int sock = luaL_checkinteger(L, 1); // get the first argument
  int size = 0; // get the 2nd argument using get_buffer, it will auto get 
  void* buff = get_buffer(L, 2, &size); // the content according to the value type
  int err = skynet_socket_send(ctx, sock, buff, size);
  lua_pushboolean(L, !err);
  return 1; // return the result status
}
-- skynet_socket.c --
skynet_socket_send(ctx, sock, buff, size)
socket_server_send(SOCKET_SERVER, sock, buff, size)
  struct request_package request;
  request.u.send.id = sock
  request.u.send.sz = size
  request.u.send.buffer = (char*)buffer
  send_request(SOCKET_SERVER, &request, 'D', sizeof(request.u.send))
end
send_request(socket_server* ss, request_package* r, char type, int len)
  request->header[6] = (uint8_t)type;
  request->header[7] = (uint8_t)len;
  int n = write(ss->sendctrl_fd, &request->header[6], len+2);
  // continue to write until n <= 0 then return
end
```

超文本传输
```
* 内容编码（Content-Encoding）表示对内容的压缩编码（如gzip），内容编码是可择的（如jpg/png一般不需要），而传输编码（Transfer-Encoding）则用来表示报文内容的格式
* HTTP协议中有一个重要的概念是持久连接或长连接，我们都知道HTTP运行在TCP之上，自然有TCP三次握手和慢启动等特性，为了尽可能提高HTTP性能，使用持久连接尤为重要
* HTTP/1.0中持久连接是后来引入的，通过 Connection: keep-alive 这个头部实现，服务器和客户端都可以使用它告诉对方在发送完数据之后不需要断开TCP连接以备后用
* HTTP/1.1则规定所有连接必须是持久的，除非显式的在头部加上 Connection: close，浏览器重用已经打开的空闲持久连接，可以避免缓慢的三次握手以及TCP慢启动的拥塞适应过程
* 对于非持久连接，浏览器可以通过连接是否关闭来界定请求或响应的边界；但对于持久连接这种方法行不通，一种方法是使用 Content-Length 告诉对方实际的长度
* 但如果不小心将长度设置的比实际长，浏览器又会一直傻傻的等待，另一个指定长度的坏处是如果你事先不知道内容的长度，必须将所有内容加载到内存才知道，可能必须使用很大的内存
* 在Web性能优化中，一个重要的指标叫 TTFB（Time To First Byte），就是客户端发出请求到收到响应的第一个字节所花费的时间，将所有内容都缓存起来再发送也无疑违背这个指标
* 在HTTP报文中，内容主体必须要在头部之后发送，为此我们需要一个新的机制，不依赖头部的长度信息也能知道内容的长度，传输编码（Transfer-Encoding）正是用来解决这个问题的
* 历史上 Transfer-Encoding 有多种取值，还为此定义了一个名为 TE 的头部用来协商采用哪种编码，但最新的HTTP规范里，只定义了一种传输编码：分块编码（chunked）
* 分块编码很简单，在头部加入 Transfer-Encoding: chunked 之后，报文的内容就由一个个的分块组成，连续的小块内容可以不断的发送给客户端，而无需等所有内容加载完才一次发送
* 每个分块包含十六进制的长度值和数据，长度独占一行不包括它后面的CRLF，也不包括分块数据结尾的CRLF，最后一个分块长度必须为0，对应的分块数据仅包含CRLF表示内容结束
```
