#include "RideAnalysis.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <utility>

bool Ride::readFrom(std::istream &input) {
  return static_cast<bool>(input >> vehicleId_ >> startTime_ >> duration_ >>
                           cost_ >> startArea_);
}

const std::string &Ride::vehicleId() const noexcept { return vehicleId_; }

int Ride::startTime() const noexcept { return startTime_; }

int Ride::duration() const noexcept { return duration_; }

double Ride::cost() const noexcept { return cost_; }

const std::string &Ride::startArea() const noexcept { return startArea_; }

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

  const std::size_t middle = left + ((right - left) >> 1);
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

void topDuration(const std::vector<Ride> &rides, std::size_t k) {
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

void topStartAreasByRevenue(const std::vector<Ride> &rides, std::size_t k) {
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
