# `main.cpp` 中的 `runTimedFeeSort` 解析

## 1. 它是什么

`runTimedFeeSort` 是定义在 `main` 函数内部的一个局部 lambda。它把“复制原始数据、调用费用排序函数、统计耗时、输出耗时”四件事封装在一起，供菜单中的归并排序、插入排序和希尔排序复用。

```cpp
const auto runTimedFeeSort = [&rides](
                                 const std::string &sortName,
                                 void (*sortFunction)(std::vector<Ride> &)) {
  std::vector<Ride> workingRides = rides;
  const auto startTime = std::chrono::steady_clock::now();
  sortFunction(workingRides);
  const auto endTime = std::chrono::steady_clock::now();
  const double elapsedMilliseconds =
      std::chrono::duration<double, std::milli>(endTime - startTime).count();

  const std::ios::fmtflags originalFlags = std::cout.flags();
  const std::streamsize originalPrecision = std::cout.precision();
  std::cout << sortName << "耗时：" << std::fixed << std::setprecision(3)
            << elapsedMilliseconds << " ms\n";
  std::cout.flags(originalFlags);
  std::cout.precision(originalPrecision);
};
```

这里的 `auto` 并不表示“普通函数变量”，而是让编译器推导出一个唯一的 lambda 闭包类型。这个闭包对象只存在于 `main` 函数内，不需要在头文件中声明。

## 2. 参数和捕获列表

### `[&rides]`

`rides` 是在 `main` 中通过 `loadRides` 读入的原始骑行记录。方括号中的 `[&rides]` 表示按引用捕获这个 `vector`。

这样处理后，每次执行 lambda 都能访问当前完整的原始数据，同时不会复制整个 `vector` 作为闭包自身的数据成员。lambda 内先把它复制到 `workingRides`，因此后续排序只修改副本，原始 `rides` 保持不变。

引用捕获要求 `rides` 的生命周期覆盖 lambda 的每次调用。这里两者都在 `main` 中，lambda 不会在 `main` 返回后继续使用，所以生命周期是安全的。

### `const std::string &sortName`

`sortName` 是显示在耗时信息中的算法名称，例如 `"归并排序"`。使用 `const` 引用可以避免复制字符串；调用时传入的字符串字面量也能绑定到这个形参。

### `void (*sortFunction)(std::vector<Ride> &)`

`sortFunction` 是函数指针，指向一个接收 `std::vector<Ride> &` 且返回 `void` 的函数。它使同一个 lambda 能接收不同的排序实现：

```cpp
runTimedFeeSort("归并排序", mergeSortByFee);
runTimedFeeSort("插入排序", insertionSortByFee);
runTimedFeeSort("希尔排序", shellSortByFee);
```

三个目标函数的签名都与该指针类型完全匹配。`sortFunction` 指向的是函数代码，不代表当前排序结果，也不保存某次运行的额外状态。

## 3. 函数体逐步说明

### 第一步：复制原始数据

```cpp
std::vector<Ride> workingRides = rides;
```

每次调用都会从原始 `rides` 得到一份新副本。这样连续进行多次排序时，每次实验都从相同的数据顺序开始，不会把上一次排序的结果带入下一次。

该复制发生在计时开始之前，因此复制 `vector` 的耗时不计入最终显示的排序耗时。它本身的额外空间复杂度是 `O(n)`。

### 第二步：记录开始时间

```cpp
const auto startTime = std::chrono::steady_clock::now();
```

`std::chrono::steady_clock` 是单调时钟，适合测量一段代码执行了多久；它不用于表示具体日期和时间，也不受系统时间被人为调整的影响。

### 第三步：调用传入的排序函数

```cpp
sortFunction(workingRides);
```

这里通过函数指针执行实际排序。当前三个排序函数做了两件事：

1. 按单笔费用从高到低排列整个 `workingRides`。
2. 调用 `printTopFeeRides`，输出费用最高的前 20 条记录。

因此，下面显示的“耗时”不是纯排序算法的时间，而是“完整排序 + 输出前 20 条记录”的总时间。

### 第四步：记录结束时间并换算单位

```cpp
const auto endTime = std::chrono::steady_clock::now();
const double elapsedMilliseconds =
    std::chrono::duration<double, std::milli>(endTime - startTime).count();
```

两个时间点相减得到一个 duration，再转换为以毫秒为单位的浮点数。`double` 保留了小数部分，后续输出将其格式化为三位小数。

计时区间可以概括为：

```text
复制 rides         排除
排序工作           包含
输出前 20 条记录   包含
输出耗时信息       排除
```

### 第五步：临时修改再恢复输出格式

```cpp
const std::ios::fmtflags originalFlags = std::cout.flags();
const std::streamsize originalPrecision = std::cout.precision();
```

代码先保存 `std::cout` 原来的格式标志和小数精度。随后使用：

```cpp
std::fixed << std::setprecision(3)
```

让耗时固定显示三位小数，例如 `12.345 ms`。`std::fixed` 和 `setprecision` 的设置会持续影响之后对同一个输出流的操作，所以打印结束后要恢复：

```cpp
std::cout.flags(originalFlags);
std::cout.precision(originalPrecision);
```

这样，菜单后续的整数和大整数字段不会意外带上固定小数格式。

## 4. 调用关系和数据流

以菜单选项 `1` 为例：

```text
输入选项 1
  -> 输出“归并排序费用前 20 笔”标题
  -> runTimedFeeSort("归并排序", mergeSortByFee)
     -> workingRides = rides
     -> 开始计时
     -> mergeSortByFee(workingRides)
        -> 归并排序全部记录
        -> printTopFeeRides(workingRides) 输出前 20 条
     -> 结束计时
     -> 输出“归并排序耗时：... ms”
```

选项 `2` 和 `3` 的数据流相同，只是函数指针分别指向 `insertionSortByFee` 和 `shellSortByFee`。

## 5. 阅读时容易误解的地方

- `runTimedFeeSort` 自己不负责排序算法，它是排序函数的计时包装器。
- “前 20 笔”是三个被调用排序函数内部打印的结果，lambda 本身不截取前 20 条。
- 每次调用都会整表排序，即使最终只显示 20 条。
- 测量值包含前 20 条记录的格式化与输出，因此会受到终端输出、缓冲和运行时环境的影响。
- 复制原始记录发生在计时之外，所以测量值不包含这次 `O(n)` 复制。
- `const auto` 限制的是这个局部 lambda 闭包对象在初始化后不再被重新赋值或修改；排序时被修改的是 lambda 内新建的 `workingRides`。
