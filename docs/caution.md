需要注意的点：

- mergeSortBy*Range 递归 + 显式 buffer：buffer 在 mergeSortByFee 中一次分配、递归复用；最后还要复制回 rides。递归按 [left,right) 半开区间划分，right-left<2 就返回。middle 用 left + ((right-left)>>1)
   防溢出。另外要注意：buffer 是临时数组，交换后不会同步更新 rides，所以每层合并后都必须进行回写。

- 排序稳定性/降序表达：判断条件用 rides[right].cost() > rides[left].cost()，左侧优先，也就是稳定降序；cost() 是 double，比较使用 !=/<，没有 epsilon。

- namespace {} 匿名命名空间：内部辅助函数（mergeSortByFeeRange、sift*、isBetter/isWorse、printTopFeeRides、AreaStatistics、areaRanksBefore）只在本单元可见。

- Top-K 堆（topDuration）：堆中保存的是索引，不是对象；堆顶是所有候选中最差的元素，也就是小顶堆语义，由 isWorseRide 定义：先用时长升序，如果时长相同，再用索引较大的算更差，保证并列时优先取位置更靠前的
   元素。放入新元素时通过 heap.front()=i; siftDown 替换；最后对 k 个元素做插入排序，下标用 const std::size_t current = heap[i] 保存值后再移位，正确处理了“移动时会被覆盖”的问题。

- siftDown 中的 worse：先假设 worse=left，只有右孩子更“差”时才换成右孩子；再和父节点比较，如果并不更差，就提前返回。

- 插入排序的移动语义：Ride current = std::move(rides[i]) 取出，然后在循环里用 std::move(rides[j-1]) 向后移动，最后再写回 rides[j] = std::move(current)；顺序不能颠倒。

- 希尔排序的内层 while (j >= gap && ...)：j 是 std::size_t，但因为 j >= gap 先求值，所以当 j==0 且 gap>0 时是安全的；这是关键点，换写成 j-gap>=0 就会下溢。

- searchByBike 的二分：先按车辆编号归并排序，然后用标准 left<right lower_bound 写法，在 order==0 时记录 firstMatch 并继续缩小右界，同时统计比较次数；firstMatch 用 rides.size() 作为“未找到”的哨兵。

- 文件读取：先读取声明的条数，再执行 resize(declaredCount)，然后逐条读取；读失败就停止；最后 resize(loadedCount) 收紧到实际读入的条数。readFrom 直接返回流状态的布尔值，所以 operator>> 链式转换需要注意
   ；如果成本读取失败，这条记录会被截断，不报错。

- grouped[ride.startArea()] 两处重复查询：用 std::map 分组时 operator[] 会默认构造 AreaStatistics，之后再赋 area；double 累加会有精度漂移，areaRanksBefore 用 != 比较 totalIncome。

- std::cout 格式状态保存/恢复：flags() 和 precision() 先保存再恢复，std::fixed 会残留在 flags 里，所以必须靠恢复来保证其他输出不受影响。

- std::ios::fmtflags/std::streamsize：这里显式写出类型，避免依赖 auto 推导细节。
