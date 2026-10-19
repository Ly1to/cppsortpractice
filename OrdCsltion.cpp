#include <chrono>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#define TOP_DURATION_K 10
#define TOP_AREA_K 5

class Ride {
public:
  bool readFrom(std::istream &input) {
    return static_cast<bool>(input >> vehicleId_ >> startTime_ >> duration_ >>
                             cost_ >> startArea_);
  }

  const std::string &vehicleId() const noexcept { return vehicleId_; }

  int startTime() const noexcept { return startTime_; }

  int duration() const noexcept { return duration_; }

  double cost() const noexcept { return cost_; }

  const std::string &startArea() const noexcept { return startArea_; }

private:
  std::string vehicleId_;
  int startTime_ = 0;
  int duration_ = 0;
  double cost_ = 0.0;
  std::string startArea_;
};

int loadRides(std::vector<Ride> &rides, const std::string &fileName) {
  std::ifstream input(fileName);
  if (!input) {
    std::cerr << "无法打开骑行记录文件：" << fileName << '\n';
    return -1;
  }

  std::size_t declaredCount = 0;
  if (!(input >> declaredCount)) {
    rides.clear();
    std::cout << "共读入0条\n";
    return 0;
  }

  rides.clear();
  rides.resize(declaredCount);

  std::size_t loadedCount = 0;
  while (loadedCount < declaredCount && rides[loadedCount].readFrom(input)) {
    ++loadedCount;
  }
  rides.resize(loadedCount);

  if (loadedCount != declaredCount) {
    std::cerr << "记录读取不完整：文件声明 " << declaredCount
              << " 条，实际读入 " << loadedCount << " 条\n";
  }

  std::cout << "共读入" << loadedCount << "条\n";
  return static_cast<int>(loadedCount);
}

namespace {

void mergeSortByFeeRange(std::vector<Ride> &rides, std::vector<Ride> &buffer,
                         std::size_t left, std::size_t right) {
  if (right - left < 2) {
    return;
  }

  const std::size_t middle = left + (right - left) / 2;
  mergeSortByFeeRange(rides, buffer, left, middle);
  mergeSortByFeeRange(rides, buffer, middle, right);

  std::size_t leftIndex = left;
  std::size_t rightIndex = middle;
  std::size_t outputIndex = left;

  while (leftIndex < middle && rightIndex < right) {
    if (rides[rightIndex].cost() > rides[leftIndex].cost()) {
      buffer[outputIndex++] = rides[rightIndex++];
    } else {
      buffer[outputIndex++] = rides[leftIndex++];
    }
  }

  while (leftIndex < middle) {
    buffer[outputIndex++] = rides[leftIndex++];
  }
  while (rightIndex < right) {
    buffer[outputIndex++] = rides[rightIndex++];
  }

  for (std::size_t i = left; i < right; ++i) {
    rides[i] = buffer[i];
  }
}

void mergeSortByBikeIdRange(std::vector<Ride> &rides, std::vector<Ride> &buffer,
                            std::size_t left, std::size_t right) {
  if (right - left < 2) {
    return;
  }

  const std::size_t middle = left + (right - left) / 2;
  mergeSortByBikeIdRange(rides, buffer, left, middle);
  mergeSortByBikeIdRange(rides, buffer, middle, right);

  std::size_t leftIndex = left;
  std::size_t rightIndex = middle;
  std::size_t outputIndex = left;

  while (leftIndex < middle && rightIndex < right) {
    if (rides[rightIndex].vehicleId() < rides[leftIndex].vehicleId()) {
      buffer[outputIndex++] = rides[rightIndex++];
    } else {
      buffer[outputIndex++] = rides[leftIndex++];
    }
  }

  while (leftIndex < middle) {
    buffer[outputIndex++] = rides[leftIndex++];
  }
  while (rightIndex < right) {
    buffer[outputIndex++] = rides[rightIndex++];
  }

  for (std::size_t i = left; i < right; ++i) {
    rides[i] = buffer[i];
  }
}

bool isWorseRide(const std::vector<Ride> &rides, std::size_t lhs,
                 std::size_t rhs) {
  if (rides[lhs].duration() != rides[rhs].duration()) {
    return rides[lhs].duration() < rides[rhs].duration();
  }
  return lhs > rhs;
}

bool isBetterRide(const std::vector<Ride> &rides, std::size_t lhs,
                  std::size_t rhs) {
  if (rides[lhs].duration() != rides[rhs].duration()) {
    return rides[lhs].duration() > rides[rhs].duration();
  }
  return lhs < rhs;
}

void siftUp(std::vector<std::size_t> &heap, const std::vector<Ride> &rides,
            std::size_t index) {
  while (index > 0) {
    const std::size_t parent = (index - 1) / 2;
    if (!isWorseRide(rides, heap[index], heap[parent])) {
      break;
    }
    std::swap(heap[index], heap[parent]);
    index = parent;
  }
}

void siftDown(std::vector<std::size_t> &heap, const std::vector<Ride> &rides,
              std::size_t index) {
  while (true) {
    const std::size_t left = index * 2 + 1;
    if (left >= heap.size()) {
      return;
    }

    std::size_t worse = left;
    const std::size_t right = left + 1;
    if (right < heap.size() && isWorseRide(rides, heap[right], heap[left])) {
      worse = right;
    }
    if (!isWorseRide(rides, heap[worse], heap[index])) {
      return;
    }

    std::swap(heap[index], heap[worse]);
    index = worse;
  }
}

void printTopFeeRides(const std::vector<Ride> &rides) {
  const std::size_t displayCount =
      rides.size() < 20 ? rides.size() : std::size_t{20};
  const std::ios::fmtflags originalFlags = std::cout.flags();
  const std::streamsize originalPrecision = std::cout.precision();
  std::cout << std::fixed << std::setprecision(2);

  for (std::size_t i = 0; i < displayCount; ++i) {
    std::cout << rides[i].vehicleId() << ' ' << rides[i].duration() << ' '
              << rides[i].cost() << '\n';
  }

  std::cout.flags(originalFlags);
  std::cout.precision(originalPrecision);
}

struct AreaStatistics {
  std::string area;
  std::size_t orderCount = 0;
  double totalIncome = 0.0;
};

bool areaRanksBefore(const AreaStatistics &lhs, const AreaStatistics &rhs) {
  if (lhs.totalIncome != rhs.totalIncome) {
    return lhs.totalIncome > rhs.totalIncome;
  }
  if (lhs.orderCount != rhs.orderCount) {
    return lhs.orderCount > rhs.orderCount;
  }
  return lhs.area < rhs.area;
}

} // namespace

void mergeSortByFee(std::vector<Ride> &rides) {
  std::vector<Ride> buffer(rides.size());
  mergeSortByFeeRange(rides, buffer, 0, rides.size());
  printTopFeeRides(rides);
}

void insertionSortByFee(std::vector<Ride> &rides) {
  for (std::size_t i = 1; i < rides.size(); ++i) {
    Ride current = std::move(rides[i]);
    std::size_t j = i;

    while (j > 0 && rides[j - 1].cost() < current.cost()) {
      rides[j] = std::move(rides[j - 1]);
      --j;
    }
    rides[j] = std::move(current);
  }
  printTopFeeRides(rides);
}

void shellSortByFee(std::vector<Ride> &rides) {
  for (std::size_t gap = rides.size() / 2; gap > 0; gap /= 2) {
    for (std::size_t i = gap; i < rides.size(); ++i) {
      Ride current = std::move(rides[i]);
      std::size_t j = i;

      while (j >= gap && rides[j - gap].cost() < current.cost()) {
        rides[j] = std::move(rides[j - gap]);
        j -= gap;
      }
      rides[j] = std::move(current);
    }
  }
  printTopFeeRides(rides);
}

void searchByBike(std::vector<Ride> &rides, const std::string &vehicleId) {
  std::vector<Ride> buffer(rides.size());
  mergeSortByBikeIdRange(rides, buffer, 0, rides.size());

  std::size_t left = 0;
  std::size_t right = rides.size();
  std::size_t firstMatch = rides.size();
  std::size_t comparisonCount = 0;

  while (left < right) {
    const std::size_t middle = left + (right - left) / 2;
    ++comparisonCount;

    const int order = rides[middle].vehicleId().compare(vehicleId);
    if (order < 0) {
      left = middle + 1;
    } else {
      if (order == 0) {
        firstMatch = middle;
      }
      right = middle;
    }
  }

  if (firstMatch == rides.size()) {
    std::cout << "未找到车辆编号：" << vehicleId << '\n';
    std::cout << "比较次数：" << comparisonCount << '\n';
    return;
  }

  const Ride &ride = rides[firstMatch];
  const std::ios::fmtflags originalFlags = std::cout.flags();
  const std::streamsize originalPrecision = std::cout.precision();
  std::cout << std::fixed << std::setprecision(2);
  std::cout << ride.startTime() << ' ' << ride.duration() << ' ' << ride.cost()
            << '\n';
  std::cout.flags(originalFlags);
  std::cout.precision(originalPrecision);
  std::cout << "比较次数：" << comparisonCount << '\n';
}

void topDuration(const std::vector<Ride> &rides,
                 std::size_t k = TOP_DURATION_K) {
  if (k == 0 || rides.empty()) {
    return;
  }

  std::vector<std::size_t> heap;
  heap.reserve(k);

  for (std::size_t i = 0; i < rides.size(); ++i) {
    if (heap.size() < k) {
      heap.push_back(i);
      siftUp(heap, rides, heap.size() - 1);
    } else if (isBetterRide(rides, i, heap.front())) {
      heap.front() = i;
      siftDown(heap, rides, 0);
    }
  }

  for (std::size_t i = 1; i < heap.size(); ++i) {
    const std::size_t current = heap[i];
    std::size_t j = i;
    while (j > 0 && isBetterRide(rides, current, heap[j - 1])) {
      heap[j] = heap[j - 1];
      --j;
    }
    heap[j] = current;
  }

  for (const std::size_t index : heap) {
    std::cout << rides[index].vehicleId() << ' ' << rides[index].startTime()
              << ' ' << rides[index].duration() << '\n';
  }
}

void topStartAreasByRevenue(const std::vector<Ride> &rides,
                            std::size_t k = TOP_AREA_K) {
  if (k == 0 || rides.empty()) {
    return;
  }

  std::map<std::string, AreaStatistics> grouped;
  for (const Ride &ride : rides) {
    AreaStatistics &statistics = grouped[ride.startArea()];
    statistics.area = ride.startArea();
    ++statistics.orderCount;
    statistics.totalIncome += ride.cost();
  }

  std::vector<AreaStatistics> statistics;
  statistics.reserve(grouped.size());
  for (const auto &entry : grouped) {
    statistics.push_back(entry.second);
  }

  for (std::size_t i = 1; i < statistics.size(); ++i) {
    const AreaStatistics current = statistics[i];
    std::size_t j = i;
    while (j > 0 && areaRanksBefore(current, statistics[j - 1])) {
      statistics[j] = statistics[j - 1];
      --j;
    }
    statistics[j] = current;
  }

  const std::size_t displayCount =
      statistics.size() < k ? statistics.size() : k;
  const std::ios::fmtflags originalFlags = std::cout.flags();
  const std::streamsize originalPrecision = std::cout.precision();
  std::cout << std::fixed << std::setprecision(2);

  for (std::size_t i = 0; i < displayCount; ++i) {
    const AreaStatistics &statisticsEntry = statistics[i];
    std::cout << i + 1 << ' ' << statisticsEntry.area << ' '
              << statisticsEntry.orderCount << ' '
              << statisticsEntry.totalIncome << '\n';
  }

  std::cout.flags(originalFlags);
  std::cout.precision(originalPrecision);
}

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
