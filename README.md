
# C语言编写的简单计算器

>使用SLR Parser实现语法分析，本项目除了实现了SLR Parser外还实现了几种简单的数据结构：数组栈、循环队列、哈希表和哈希集合 以及 树。应该可以勉强作为数据结构的课程设计，编译原理我还没正式学，SLR Parser是我在维基百科和网上找的一个关于LR Parser的课件里面学到的一点皮毛，如有错误欢迎指出。
	
## 项目结构说明
```
📦src
 ┣ 📂main               项目主体代码文件夹
 ┃ ┣ 📜evaluator.c      计算器
 ┃ ┣ 📜grammar.c        语法定义和语法分析表生成
 ┃ ┣ 📜lexer.c          分词器
 ┃ ┣ 📜main.c           程序入口
 ┃ ┣ 📜map.c            哈希表 
 ┃ ┣ 📜parser.c         语法分析器、语法分析树(ParseTree)转抽象语法树AST 以及 AST转字符串表达式
 ┃ ┣ 📜queue.c          循环队列
 ┃ ┣ 📜set.c            基于哈希表实现的哈希集合
 ┃ ┣ 📜stack.c          数组栈
 ┗ 📂test
 ┃ ┣ 📜evaluator_test.c 计算器测试
 ┃ ┣ 📜grammar_test.c   语法定义和语法分析表生成测试
 ┃ ┣ 📜hash_map_test.c  哈希表测试
 ┃ ┣ 📜hash_set_test.c  哈希集合测试
 ┃ ┣ 📜lexer_test.c     分词器测试
 ┃ ┣ 📜parser_test.c    语法分析测试
 ┃ ┣ 📜queue_test.c     循环队列测试
 ┃ ┣ 📜stack_test.c     数组栈测试
 ┃ ┣ 📜test.c           测试入口
```
## 使用说明

#### make 命令 （需要安装gcc编译环境）
1. `make main`  编译并运行主体程序
2. `make test`  编译并运行测试程序
3. `make clean` 清理build目录

#### Visual Studio
直接打开`CCalculator.sln`即可

## 计算器输入说明
1. 支持常数、自定义标识符（未知数或者已赋值的标识符）和函数，比如 1.e+03、x 和 diff(x, x, 1)
2. 支持常规表达式和后缀表达式（逆波兰表达式）混合输入，比如 1 2 + + 3 结果是 6
3. 支持赋值表达式，比如 f = x

## 支持的函数
```
sin(x)              正弦函数
cos(x)              余弦函数
tan(x)              正切函数
asin(x)、arcsin(x)  反正弦函数
acos(x)、arccos(x)  反余弦函数
atan(x)、arctan(x)  反正切函数
ln(x)               自然常数对数函数
abs(x)              绝对值函数
diff(f, x, c)       求导函数
```

## 具体例子
```
> 1604.4 6 6 6 20 21 /^-^/
< 666

> 1 2 3 * + * (x - 5)
< 7 * (x - 5)

> diff(ln(cos(x)), x, 1)
< -1 / cos(x) * sin(x)

> f = g = x^2 + x + 1

> diff(f, x, 1) 
< 2 * x + 1

> x = -3

> f
< 7

> x + 1 + x + 3
< -2

> x = ? (x = ? 表示令x变回未知数)

> x + 1 + x + 3
< x + x + 4

> exit (退出程序)
```

## 存在的问题
只做了常数合并化简，未知数的没有做，所以如果输入 `x + 1 + x + 3 ` 结果会是 `x + x + 4`