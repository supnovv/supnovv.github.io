<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=Edge">
    <title>HTML Test Page</title>
  </head>
  <body>
    <p></p>
    <ul>
      <li><a href="test.html">Testing Page</a></li>
      <li><a href="https://www.joelonsoftware.com/">Joel on Software</a></li>
      <li><a href="http://www.norvig.com/">Peter Norvig</a></li>
      <li><a href="https://blog.codinghorror.com/">Coding Horror</a></li>
      <li><a href="http://andrewdupont.net/">Andrew Dupont's Blog</a>: [<a href="http://andrewdupont.net/2006/05/18/javascript-associative-arrays-considered-harmful/">JS Arrays</a>]
      </li>
      <li><a href="http://dean.edwards.name/">Dean Edwards's Site</a></li>
      <li><a href="https://imququ.com/">Jerry Qu's Site</a></li>
      <li><a href="http://www.smartlei.com/">Smart Lei's Site</a></li>
      <li><a href="http://marvel.wikia.com/wiki/Groot_(Earth-616)">I am Groot</a></li>
      <li><a href="https://html5boilerplate.com/">Boilerplate</a></li>
      <li><a href="https://www.byvoid.com/">BYVoid's Site</a></li>
    </ul>
  </body>
</html>


<!DOCTYPE html>
<html lang="zh-CN">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=Edge">
    <title>Nice Online Tools</title>
    <link rel="stylesheet" href="src/thatcore.css">
    <script src="src/thatjcore.js"></script>
    <script src="src/thatjtest.js"></script>
  </head>
  <body>
    <div id="root-container">
    <header id="page-header">
    <h1>NICE ONLINE TOOLS</h1>
    </header>
    <main id="page-main">
      <form id="grid-calc" action="#">
        <p>简单网格成本计算</p>
        <fieldset id="grid-calc-input">
          <legend>输入价位和买入金额</legend>
　　　　　　　　　　<label>第一组   价格</label><input></input>　<label>计划金额</label><input></input><br>
        </fieldset>
        <fieldset id="grid-calc-result">
          <legend>网格成本一览</legend>
        </fieldset>
      </form>
    </main>
    <nav id="page-nav"></nav>
    <footer id="page-footer"></footer>
    </div>
    <aside id="page-console"></aside>
  </body>
</html>

<!DOCTYPE html>
<html lang="zh-CN">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=Edge">
    <title>Testing Page</title>
    <link rel="stylesheet" href="src/thatcore.css">
    <script src="src/thatjcore.js"></script>
    <script src="src/thatjtest.js"></script>
  </head>
  <body>
    <div id="root-container">
    <header id="page-header">
    <h1>IAMGROOT.ORG</h1>
    <p class="legend">有关格鲁特以及其他一切 <small>ABOUT GROOT AND ANYTHING ELSE</small></p>
    <p class="time">[叁仟零壹拾陆]年[拾贰]月[贰拾玖]日 [拾壹]点[贰拾肆]分</p>
    </header>
    <main id="page-main">
      <section>
      <h2>传奇<span class="ttsep"> | </span><small>LEGEND STORIES</small></h2>
      </section>
      <section>
      <h2>流言<span class="ttsep"> | </span><small>WHISPER IN THE WIND</small></h2>
      <p>传说格鲁特在二十世纪八十年代来过地球，在那里有一世的经历。</p>
      <p>传说格鲁特的一根树须长大后来到中土世界，曾帮助人类攻陷艾辛格，保卫了幽暗森林。</p>
      <p>艾辛格在千年前就屹立在那里，智者萨鲁曼。</p>
      <h2>魔法<span class="ttsep"> | </span><small>MAGIC, SPELL</small></h2>
      <h2>修炼<span class="ttsep"> | </span><small>LEARNING AND IMPROVE</small></h2>
      <p>格鲁特JS手册</p>
      <p>格鲁特LUA手册</p>
      <p>格鲁特C手册</p>
      <p>C/CPP/JAVA/PHP/JS/LUA/SCHEME</p>
      <p>格鲁特曾为 JS 浏览器兼容性发疯，体会到地球谋生的艰难。</p>
      <p>后来，他无意中用古老的树之魔法扫开障碍，建立了他的第一个 JS 核心库。</p>
      <p>格鲁特 JS 纪要</p>
      <p>一、</p>
      </section>
    </main>
    <nav id="page-nav">Nav</nav>
    <footer id="page-footer">Footer</footer>
    </div>
    <aside id="page-console"></aside>
  </body>
</html>

@HTML.default{
  LANG.zh
  CHARSET.utf8
}

(@HTML.smplgrid|LANG.en)

1.内嵌指令以(@head开始以)结束并可以使用多个括号例如以((@head开始以))结束
2.其中head的内容可以是space+variable/break+variable/variable
3.space+variable表示先加一个空格然后扩展变量的内容
4.break+variable表示先添加一个换行再扩展变量的内容
5.只有variable的情况表示在内嵌指令位置直接扩展变量的内容
6.变量variable的格式可以是TYPE/TYPES/TYPESname/TYPESname/text
7.如果当前上下文只有一处使用该类型可以用类型名当做变量名写成TYPE如果有多处使用必须为每一处声明一个变量名称写成TYPEname
8.类型后面如果加一个S表示该变量是一个该类型的数组注意类型名称只使用大写字母
9.如果variable以小写字母开始表示这是一个纯文本变量会按照元素所允许的字符扩展其中的字符串
0.@@HTML定义类型HTML该类型是一个模板定义开头可以有以--开始的注释行
1.@@LANG:定义类型LANG能拥有的值每个值都以点.开头例如.zh表示LANG有一个名称为zh值为"zh"的值
2.另外.cn="zh-CN"表示LANG有一个名称为cn值为"zh-CN"的值值定义开头也可以有以--开始的注释行
3.模板类型和值类型的定义以空行结束

@tmpl.css
---
<link rel="stylesheet" href="(@URL)">

@tmpl.js
---
<script src="(@URL)"></script>

(@break+function(a, b)[[
  return a..b
]])

