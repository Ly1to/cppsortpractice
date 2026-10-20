#include "RideAnalysis.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  const std::string fileName = argc > 1 ? argv[1] : "rides_1k.txt";
  std::vector<Ride> rides;
  const int loadedCount = loadRides(rides, fileName);
  if (loadedCount < 0) {
    return 1;
  }
  if (rides.empty()) {
    return 0;
  }

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

  while (true) {
    std::cout << "\n请选择要执行的函数：\n"
              << "  1. 归并排序：费用降序前 20 笔并计时\n"
              << "  2. 插入排序：费用降序前 20 笔并计时\n"
              << "  3. 希尔排序：费用降序前 20 笔并计时\n"
              << "  4. 按车辆编号查找\n"
              << "  5. 输出骑行时长最长的 " << TOP_DURATION_K << " 笔\n"
              << "  6. 按起点区域统计订单数与总收入前 " << TOP_AREA_K << " 名\n"
              << "  0. 退出\n"
              << "请输入选项：";

    int choice = 0;
    if (!(std::cin >> choice)) {
      if (std::cin.eof()) {
        break;
      }
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::cout << "输入无效，请输入数字选项。\n";
      continue;
    }

    if (choice == 0) {
      break;
    }

    switch (choice) {
    case 1:
      std::cout << "\n归并排序费用前 20 笔：\n";
      runTimedFeeSort("归并排序", mergeSortByFee);
      break;
    case 2:
      std::cout << "\n插入排序费用前 20 笔：\n";
      runTimedFeeSort("插入排序", insertionSortByFee);
      break;
    case 3:
      std::cout << "\n希尔排序费用前 20 笔：\n";
      runTimedFeeSort("希尔排序", shellSortByFee);
      break;
    case 4: {
      std::string vehicleId;
      std::cout << "请输入车辆编号：";
      if (!(std::cin >> vehicleId)) {
        if (std::cin.eof()) {
          return 0;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "车辆编号输入无效。\n";
        break;
      }
      std::cout << "\n车辆编号 " << vehicleId << " 的查找结果：\n";
      std::vector<Ride> workingRides = rides;
      searchByBike(workingRides, vehicleId);
      break;
    }
    case 5:
      std::cout << "\n骑行时长前 " << TOP_DURATION_K << " 笔：\n";
      topDuration(rides);
      break;
    case 6:
      std::cout << "\n起点区域订单数与总收入前 " << TOP_AREA_K
                << " 名（排名 区域 订单数 总收入）：\n";
      topStartAreasByRevenue(rides);
      break;
    default:
      std::cout << "无效选项，请输入 0 到 6。\n";
      break;
    }
  }

  std::cout << "程序已退出。\n";
  return 0;
}
