# Advanced Compiler Techniques Project Report

PKUEECS 研究生课程 Advanced Compiler Techniques 大作业：编译器的IR优化。

2001213077 陈俊达

# 0. 目录

- [Advanced Compiler Techniques Project Report](#advanced-compiler-techniques-project-report)
- [0. 目录](#0-目录)
- [1. 完成的工作](#1-完成的工作)
- [2. 编译和运行](#2-编译和运行)
- [3. 实现要点](#3-实现要点)
  - [3.1 翻译三地址码到C](#31-翻译三地址码到c)
    - [3.1.1 处理函数调用](#311-处理函数调用)
    - [3.1.2 处理全局变量、局部变量和参数](#312-处理全局变量局部变量和参数)
    - [3.1.3 处理跳转地址](#313-处理跳转地址)
    - [3.1.4 处理struct](#314-处理struct)
  - [3.2 优化策略](#32-优化策略)
  - [3.3 Reaching Definition和Simple Constant Propagation](#33-reaching-definition和simple-constant-propagation)
    - [3.3.1 预先优化](#331-预先优化)
    - [3.3.2 识别每个语句def和use](#332-识别每个语句def和use)
    - [3.3.3 处理函数调用](#333-处理函数调用)
  - [3.4 Live Variable和Dead Code Elimination](#34-live-variable和dead-code-elimination)
  - [3.5 转换到SSA](#35-转换到ssa)
    - [3.5.1 计算Dominator tree](#351-计算dominator-tree)
  - [3.6 基于SSA进行Constant Propagation](#36-基于ssa进行constant-propagation)
  - [3.7 从SSA转换回三地址码](#37-从ssa转换回三地址码)

# 1. 完成的工作

- lab1：
    - [x] 翻译三地址码到C代码（已通过所有测试代码）
- lab2：
    - [x] 构建CFG（已通过所有测试代码）
    - [ ] Strongly Connected Components分析（不知道是什么……）
    - [x] Reaching Definition Analysis
    - [x] Simple Constant Propagation
    - [x] Live Variable Analysis
    - [x] Dead Code Elimination（report中没有包含strongly connected component）
    - [ ] Extra credit: perform constant propagation with the Wegman & Zadeck algorithm. 
- lab3:
    - [x] 转换到SSA
    - [x] SSA based constant propagation
    - [ ] SSA based loop invariant code motion
    - [x] SSA转换回三地址码
    - [ ] Extra Credits: Perform global common subexpression elimination.

# 2. 编译和运行

代码采用C++编写，已在`Arch Linux`中使用`g++ 10.2.0`编译通过，应该没有使用C++ 14及以上的特性，所以支持C++11的C++编译器都可以编译。

代码编译方式均为在对应目录（`lab1/lab1`, `lab2/lab2`, `lab3/lab3`）下运行`./compile.sh`。

下面为运行测试锁需要的命令和脚本。运行任何验证之前都需要首先在`lab1/src`下运行make.sh编译好自带的C到三地址码编译器，并存放在`lab1/src`目录下。

```bash
# 验证lab1：

cd lab1/examples
# 对比各个文件的MD5，是一致的
# 注意，lab1要求还要输出各个函数的定义，
#   所以lab1的可执行文件在stdout输出正常的结果之外，
#   还会在stderr输出所有函数的定义
#   lab1/lab1/run.sh将会把stderr重定向到当前目录$(date +"%s").h文件中
./check.sh

# 验证lab2：
cd lab2/examples
# 验证CFG正确性
./check.sh
# 验证经过scp和dse优化后的输出C代码的正确性（对比输出）
./check-correctness.sh

# 验证lab3：

cd lab3/examples
# 验证经过lab2的DSE优化和基于SSA的常数传播优化后的输出C代码的正确性（对比输出）
./check-correctness.sh
```

# 3. 实现要点

## 3.1 翻译三地址码到C

为了实现简单，翻译方式采用了最简单的接近直译的大量使用label和goto的方式来实现，将虚拟寄存器考虑为一个名为`r{所在行号}`的变量。举例如下。所有情况请看`lab1/lab1/translator.cpp:324`开始的代码。

| 汇编                                  | C代码                     |
| ------------------------------------- | ------------------------- |
| `instr 18: move a#-8 b#-16`           | `b = a;`                  |
| `instr 19: add 8 b#-16`               | `long long r19 = 8 + b;`  |
| `instr 20: cmpeq 8 b#-16`             | `long long r20 = 8 == b;` |
| `instr 21: blbc (19) [24]`            | `if ((!(r19))) goto l24;` |
| `instr 22: add results_base#32696 GP` | `LL r22 = (LL)&results;`  |
| `instr 23: load (22)`                 | `LL r23 = *((LL *)r22);`  |
| `instr 24: store 8 (22)`              | `*((LL *)r22) = 8;`       |

### 3.1.1 处理函数调用

代码首先扫描一遍所有代码，得出所有函数的参数个数（`ret`的操作数就是参数空间的长度，由于参数不能传数组，所以ret的操作数 / 8就是参数个数），并记录下来。

如果函数需要传参，代码在调用`call`前会有`param`指令。代码在翻译时维护一个`stack`，在遇到`param`指令时，将操作数压栈。在遇到`call`指令时，首先查找对应的函数需要多少个参数，然后从`stack`中pop出对应数量的参数。

### 3.1.2 处理全局变量、局部变量和参数

代码在分析操作数时，将会同时分析操作数是否为全局变量（offset > 8192）、局部变量（offset < 0）或者参数（offset > 0），并将其记录在对应的offset中。


当系统在输出时，首先输出各个全局变量，然后输出各个函数。在输出函数时，首先输出函数签名，再输出局部变量，然后是翻译出来的C代码。

当系统在输出全局变量和局部变量时，将会从最低地址开始分析，并计算出每个全局变量的长度，如果长度为8，说明是`long long`类型，否则是数组。

举例：如果代码中出现过以下全局变量：

- `a#32760`
- `b#32744`
- `d#32704`
- `c#32736`

从出现过的最低偏移量32704开始往32768开始分析，可以发现各个变量所占据的空间及对应的类型为：

| 变量 | 空间 | 类型         |
| ---- | ---- | ------------ |
| a    | 8    | long long    |
| b    | 16   | long long[2] |
| c    | 8    | long long    |
| d    | 32   | long long[4] |

局部变量从最小出现的offset分析，得出每个变量的长度以及类型。

由于参数传入的都是long long类型的，所以不需要对参数进行特殊分析，只需要记录各个参数的偏移量得出对应的位置，然后输出到对应的参数列表中的位置就可以了。

### 3.1.3 处理跳转地址

代码在分析三地址码的过程中，还会记录所有出现的`[%d]`位置，并在对应的行之前插入`l%d:;`标签，用于支持goto语句的跳转。

### 3.1.4 处理struct

代码将struct和array进行同等处理。所有struct将会被当成array，对struct成员的访问等同于对数组对应偏移量的访问。


## 3.2 优化策略

- 对于lab2和lab3中的优化，代码都采用了多次优化的策略，即对转换后的三地址码循环地采用指定的优化方式进行优化，直到每个优化方式都没能优化（SCP：没有propagate任何常量；DSE：没有删除任意一行的代码等）才停止。如果backend为rep，那么每种优化只有一个输出结果，其值为每次优化的结果的和。
- 代码如果要删除一个语句，都是将语句设置为`nop`，而非直接删除语句。因为直接删除语句可能影响CFG的信息、跳转目标等信息，每次删除语句都需要更新CFG和跳转目标，较为麻烦
- 对于基于数据流框架的结果的优化（lab2中的SCP和DSE），在获得每个BB的IN和OUT结果后，系统将使用IN和OUT集对每个BB进行优化时。在对BB进行优化时，代码还将会遍历一遍BB中的语句（SCP是从头到尾，DSE是从尾到头，和使用的数据流分析的方向一致），在对语句进行优化的同时，还会同时更新对应的信息，以获得最佳的优化效果。

举例：DSE优化，分析得到以下BB的OUT集为[b]

```
instr 2: move 1 b#-8
instr 3: move 2 b#-8
```

在进行优化时，代码首先访问语句3，发现b在OUT集中，说明B是live的，所以语句3是不能删除的。

同时，代码发现语句3是一个对b的def，所以代码将会把b从OUT集中删除，在3语句之前，b不是live的，。

然后，系统访问语句2，发现b不在OUT集中，所以在语句2时B不是live（即使在这个BB中是live的），所以instr 2将会被删除（设置为`nop`）。最终结果为：

```
instr 2: nop
instr 3: move 2 b#-8
```

## 3.3 Reaching Definition和Simple Constant Propagation

lab2中的SCP实现了对`long long`类型的**全局变量**、**局部变量**和**虚拟寄存器**的Reaching Definition分析和常数传播，并使用了`lab2/examples`下的所有样例程序的测试，证明了优化后的程序的输出没有发生变化。

参考`lab2/lab2`目录下`example-scp`为名的几个文件查看优化效果。

- `.c`为原程序
- `.3addr`为未经过优化的原程序对应的三地址码
- `.scp.c`为仅使用scp优化过的程序C代码

### 3.3.1 预先优化

自带的C到三地址码翻译器把`long long`类型的变量的访问和赋值使用了两种翻译形式：

- 局部变量：
  - 访问：`add a#-8 9`（对应`a + 9;`）
  - 赋值：`move 8 a#-8`（对应`a = 8;`）
- 全局变量：
  - 访问：
    - `instr 1: add results_base#32696 GP`
    - `instr 2: load (1)`
    - 对应`long long r2 = results;`
  - 赋值：
    - `instr 1: add results_base#32696 GP`
    - `instr 2: store 8 (1)`
    - 对应`results = 8;`

这使得对分析各种变量的赋值和使用造成了麻烦。

所以在进行优化之前，代码首先对以上出现的对全局变量的访问和赋值翻译为与局部变量相同的形式。

代码引入了一个新的操作码`assign`，有1个操作数，行为是把操作数的值附给对应的虚拟寄存器。

优化后，上面的对results变量的访问的三地址码变为：

- `instr 1: nop`
- `instr 2: assign results_base#32696`

对results的赋值的三地址码变为：

- `instr 1: nop`
- `instr 2: move 8 results_base#32696`

此部分的实现在`lab2/lab2/peephole.cpp`中。

### 3.3.2 识别每个语句def和use

在经过预先优化后，分析每个语句产生对哪个变量的def按照以下规则进行：

- `move`：对第二个参数的def
- "enter", "nop", "param", "wrl", "write", "entrypc", "store",
    "br", "blbc", "blbs", "enter", "ret", "call"以外的语句：对对应的行的虚拟寄存器的def
- 其他指令：不产生def

而每个语句的use就是所有语句的操作数（除了move的第二个参数）。

### 3.3.3 处理函数调用

在CFG生成过程中，`call [line]`的后一条语句也被考虑为一个BB的leader，并在call语句所在的BB和下一个BB之间有一条CFG边。

函数调用有可能修改全局变量，所以`call`语句之后的BB的IN不应该继承上一个前继BB的对全局变量的def。

若不考虑这个问题，`hanoifibfac.c`例子的`Hanoi`函数将会优化出错：

```c
void Hanoi(long height)
{
  // BB1
  count = 0;
  MoveTower(1, 2, 3, height);

  // BB2
  WriteLine();
  WriteLong(count);
  WriteLine();
}
```

MoveTower函数中将会对全局变量count进行修改，如果count = 0的def传到了BB2中，那么`WriteLong(count)`将会被优化为`WriteLong(0)`，这是不正确。

为了解决这个问题，代码中将Reaching Definition的数据流方程从原来的`F(x) = GEN(B) U (x - KILL(B))`修改为：

`F(x) = (GEN(B) - GLOBALS(B)) U (x - KILL(B))`

其中`GLOBALS(B)`定义为：

- 如果B的最后一条语句不是call，`GLOBALS(B) = 空集`
- 如果是call，`GLOBALS(B) = B中所有对全局变量的def`

这样，如果一个BB中有对全局变量的赋值，而且它的最后一条语句是一个函数调用，那么这个全局变量的赋值将不会在BB的OUT集，后继BB也不会或者这个赋值，避免了这个问题。

## 3.4 Live Variable和Dead Code Elimination

代码实现了对所有`long long`类型的**局部变量**和**虚拟寄存器**的Live Variables分析和Dead Code Elimination优化，不对全局变量进行分析。

由于不清楚Strongly Connected Components分析是什么，所以就没有做这个分析，这个DCE的输出结果格式如下：

```
Function: 2
Number of statements eliminated: 1
```

在分析之前，代码先分析一遍函数中的所有语句，获取需要被分析的局部变量以及被分配的虚拟寄存器，然后再计算每个BB的use和kill集。

因为分析和优化过程不涉及全局变量，所以代码在进行数据流分析过程之前的初始化步骤为OUT[EXIT] = IN[EXIT] = 空集。

参考`lab2/lab2`目录下`example-dse`为名的几个文件查看优化效果

- `.c`为原程序
- `.3addr`为未经过优化的原程序对应的三地址码
- `.dse.c`为仅使用scp优化过的程序C代码

## 3.5 转换到SSA

SSA和原三地址码之间的转换基本上和上课讲和网上的资料类似，具体实现时参考了 https://www.ed.tus.ac.jp/j-mune/keio/m/ssa2.pdf 这份资料。

一些实现要点：

- 代码中只转换了**long long类型的局部变量**，未考虑数组、全局变量、虚拟寄存器
- 由于SSA中不包含每个变量的offset，所以要在转换SSA之前，记录每个long long类型的局部变量的偏移量（`Function::symbol_table`）
- 在插入`phi`语句时，`phi`语句的语句编号为所有指令数+1，所以SSA形式下的语句编号可能不是递增的。在插入`phi语句`后，还需要修改跳转目标、CFG中每个BB块的语句编号集合以及CFG的leaders编号

### 3.5.1 计算Dominator tree

代码采用了一个很直接、实现很简单、但是运行效率不高的计算Dominator tree的方法，步骤如下：

1. 使用数据流框架计算每个块的dominators
   - 转换方程为：DOM[n] = { n } U (对于n的所有前继s，DOM(n)的交集)
   - 初始条件为：对于每个BB n，DOM[n] = 所有节点的集合
2. 对于每个BB n，直接使用immediate Dominator的定义判断哪个n的dominator是n的immediate dominator
   - 即：对于每个BB n，对于n的每个dominator d，如果d是n的idom，idom[n] = d，break
   - 判断d是n的idom，d != n，并且不存在p != d != n，d dom p dom n
3. 对每个BB初始化对应的dom tree节点
4. 对于每个BB n以及对应的idom(n) i，把n节点加入i节点的children列表中

## 3.6 基于SSA进行Constant Propagation

参考`lab3/lab3`下的`example-ssa`为名字的以下文件查看SSA效果：

- `.c`为原文件
- `.3addr`为原文件直接转换到的三地址码
- `.ssa`为原文件的三地址码的SSA形式
- `.ssacp.ssa`为进行基于SSA的常数传播优化后的SSA形式
- `.ssacp.c`为进行基于SSA的常数传播优化后转换回的C代码

在SSA下进行Constant Propagation很简单，分为以下两步：

1. 遍历所有语句，记录下为被赋值为常量的SSA变量
   1. 如果`move 8 a$1`，则`a$1`是常量，其值为8 
   2. 如果遇到一个`phi`语句，其所有操作数都是同样的常量a，那么它对应的SSA变量（下一个语句（一定是`move`语句）的第二个操作数）也是常量，其值就是a
      1. 同时，把这个`phi`语句以及下一个语句（一定是`move`）都设置成`nop`
2. 再次扫描所有语句，把所有是常量的SSA变量替换为常量

和之前的优化一样，这个优化方式将会被循环进行，直到没有新的SSA变量被修改为常量。

## 3.7 从SSA转换回三地址码

这个转换分为两步：

1. 把所有形如`a$1`的SSA变量转换回形如`a#-8`的原来的带偏移量的形式
   - 由于转换到SSA的过程中只考虑了局部变量，且`Function::symbol_table`中事先记录了每个变量的偏移量，所以这个步骤是比较简单的
2. 将`phi`操作数转换为对应前继块的`move`操作。
   - 代码中对这个操作的实现为：
        1. 遇到`phi`操作后，获取下一条语句（一定是`move`），获得phi操作相关的变量，并获取这个变量的偏移量
        2. 把`phi`操作和后面的`move`语句都设置为`nop`（不直接删除的原因和前面一致，避免修改跳转目标等）
        3. 把`phi`操作的第j个操作数以`move 第j个操作数 变量#变量的偏移量`的形式插入到第j个前继的最后。
    - 对于把语句插入到BB最后的操作，由于新语句一定不是BB块的开头，所以不需要修改跳转目标和CFG和leader编号，只需要将新语句对应的语句编号插入对应的BB块中就可以了

