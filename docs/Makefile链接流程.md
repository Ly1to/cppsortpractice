# Makefile 构建与链接流程

## 1. 总体流程

默认执行 `make` 时，会使用 `Makefile` 中的第一个目标 `all`。它最终生成：

```text
build/OrdCsltion
```

完整依赖关系如下：

```text
all
 └── build/OrdCsltion
      ├── build/obj/RideAnalysis.o
      │    └── src/RideAnalysis.cpp
      └── build/obj/main.o
           └── src/main.cpp
```

构建过程分为两个阶段：

```text
src/RideAnalysis.cpp  --编译--> build/obj/RideAnalysis.o ┐
                                                           ├--链接--> build/OrdCsltion
src/main.cpp          --编译--> build/obj/main.o         ┘
```

头文件 `include/RideAnalysis.hpp` 不单独生成目标文件。它会在编译每个相关 `.cpp` 文件时被包含，并通过自动生成的 `.d` 文件加入依赖关系。

## 2. Makefile 变量

| 变量 | 当前值 | 作用 |
| --- | --- | --- |
| `CXX` | `g++` | C++ 编译器 |
| `CPPFLAGS` | `-Iinclude` | 增加头文件搜索目录 |
| `CXXFLAGS` | `-std=c++11 -Wall -Wextra -Wpedantic` | 指定 C++ 标准并开启警告 |
| `LDFLAGS` | 空 | 链接器参数 |
| `LDLIBS` | 空 | 需要链接的外部库 |
| `TARGET` | `build/OrdCsltion` | 最终可执行文件 |
| `SOURCE_DIR` | `src` | 源文件目录 |
| `OBJECT_DIR` | `build/obj` | 目标文件目录 |
| `RIDES` | `rides_1k.txt` | 运行程序时传入的数据文件 |

这些变量可以通过命令行临时覆盖，例如：

```sh
make CXX=clang++
make run RIDES=rides_10k.txt
```

## 3. 源文件和目标文件如何对应

Makefile 先找出所有的 C++ 源文件：

```make
SOURCES := $(wildcard $(SOURCE_DIR)/*.cpp)
```

当前得到：

```text
src/RideAnalysis.cpp
src/main.cpp
```

然后使用 `patsubst` 把 `src/*.cpp` 转换为 `build/obj/*.o`：

```make
OBJECTS := $(patsubst $(SOURCE_DIR)/%.cpp,$(OBJECT_DIR)/%.o,$(SOURCES))
```

对应关系是：

```text
src/RideAnalysis.cpp -> build/obj/RideAnalysis.o
src/main.cpp         -> build/obj/main.o
```

随后 `DEPENDENCIES` 再把每个 `.o` 后缀替换为 `.d`：

```make
DEPENDENCIES := $(OBJECTS:.o=.d)
```

也就是：

```text
build/obj/RideAnalysis.d
build/obj/main.d
```

## 4. 编译阶段：生成 `.o` 文件

模式规则负责将任意 `src/*.cpp` 编译为对应的目标文件：

```make
$(OBJECT_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@
```

以 `src/main.cpp` 为例，展开后的实际命令是：

```sh
mkdir -p build/obj/
g++ -Iinclude -std=c++11 -Wall -Wextra -Wpedantic -MMD -MP \
  -c src/main.cpp -o build/obj/main.o
```

关键参数的含义：

- `@mkdir -p $(dir $@)`：确保输出目录 `build/obj` 存在，命令本身不打印。
- `-Iinclude`：让 `#include "RideAnalysis.hpp"` 能在 `include` 目录中找到头文件。
- `-c`：只编译源文件，不进行链接。
- `-o $@`：`$@` 是当前规则的目标，也就是对应的 `.o` 文件。
- `$<`：当前规则的第一个前置条件，也就是对应的 `.cpp` 文件。
- `-MMD -MP`：同时生成头文件依赖并增强依赖文件的容错性。

## 5. 头文件依赖如何参与构建

`-MMD` 会为每个目标文件生成一个 `.d` 文件，例如：

```text
build/obj/main.d
```

该文件记录 `main.cpp` 包含了哪些头文件。Makefile 最后通过下面的语句将这些依赖包含回来：

```make
-include $(DEPENDENCIES)
```

`-include` 前面的减号表示依赖文件暂时不存在时不要报错。第一次构建时还没有 `.d` 文件，也可以正常从编译阶段开始。

完成一次编译后，如果 `include/RideAnalysis.hpp` 发生变化，GNU make 就能看出相关 `.o` 文件已经过期，并重新编译依赖该头文件的源文件。

## 6. 链接阶段：生成可执行文件

最终目标是 `build/OrdCsltion`，它的前置条件是所有 `.o` 文件：

```make
$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@
```

当前展开后的链接命令是：

```sh
mkdir -p build/
g++ build/obj/RideAnalysis.o build/obj/main.o -o build/OrdCsltion
```

链接阶段的命令没有 `-c`，因此编译器驱动会启动链接过程：

1. 收集两个目标文件；
2. 把外部符号引用与对应定义连接起来；
3. 完成静态库或动态库的解析；
4. 生成 `build/OrdCsltion`。

例如，`main.cpp` 中调用了 `loadRides`、`mergeSortByFee` 和 `searchByBike`。这些函数声明来自 `RideAnalysis.hpp`，实际定义位于 `RideAnalysis.cpp`，最终由链接器把 `main.o` 和 `RideAnalysis.o` 中的相关符号连接起来。

当前项目没有额外第三方库，因此 `LDFLAGS` 和 `LDLIBS` 为空。以后如果增加外部库，可以覆盖这两个变量，而不需要修改链接规则的书写方式。

Makefile 中自动变量的作用如下：

| 自动变量 | 当前值示例 | 含义 |
| --- | --- | --- |
| `$@` | `build/OrdCsltion` | 当前规则的目标 |
| `$^` | `build/obj/RideAnalysis.o build/obj/main.o` | 所有未重复的前置条件 |
| `$<` | `src/main.cpp` | 编译规则中的第一个源文件 |

## 7. 增量构建

GNU make 会比较目标和前置条件的修改时间：

- `.cpp` 或相关头文件比 `.o` 新：重新编译对应 `.o`。
- 任意 `.o` 比 `build/OrdCsltion` 新：重新执行链接。
- 所有文件都没有变化：不重新编译，也不重新链接。

因此，只修改 `src/main.cpp` 时，通常只需重新编译 `main.o`，然后重新链接；`RideAnalysis.o` 可以继续复用。

自动生成的 `.d` 文件是增量构建的一部分。执行 `make clean` 会删除整个 `build` 目录，其中也包括目标文件、依赖文件和可执行文件。

## 8. 常用命令

```sh
make
```

等价于 `make all`，构建默认目标。

```sh
make run
```

先确保 `build/OrdCsltion` 已构建，再执行：

```sh
./build/OrdCsltion rides_1k.txt
```

使用其他数据文件：

```sh
make run RIDES=rides_50k_B.txt
```

清理后重新构建：

```sh
make rebuild
```

删除所有构建产物：

```sh
make clean
```

`all`、`run`、`clean` 和 `rebuild` 被声明为 `.PHONY`，表示它们不是由同名文件表示的构建产物，即使目录中意外存在名为 `all` 或 `run` 的文件，也会执行对应规则。
