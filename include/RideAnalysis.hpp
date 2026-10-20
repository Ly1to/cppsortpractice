#ifndef RIDE_ANALYSIS_HPP
#define RIDE_ANALYSIS_HPP

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

constexpr std::size_t TOP_DURATION_K = 10;
constexpr std::size_t TOP_AREA_K = 5;

class Ride {
public:
  bool readFrom(std::istream &input);

  const std::string &vehicleId() const noexcept;
  int startTime() const noexcept;
  int duration() const noexcept;
  double cost() const noexcept;
  const std::string &startArea() const noexcept;

private:
  std::string vehicleId_;
  int startTime_ = 0;
  int duration_ = 0;
  double cost_ = 0.0;
  std::string startArea_;
};

int loadRides(std::vector<Ride> &rides, const std::string &fileName);

void mergeSortByFee(std::vector<Ride> &rides);
void insertionSortByFee(std::vector<Ride> &rides);
void shellSortByFee(std::vector<Ride> &rides);
void searchByBike(std::vector<Ride> &rides, const std::string &vehicleId);

void topDuration(const std::vector<Ride> &rides,
                 std::size_t k = TOP_DURATION_K);
void topStartAreasByRevenue(const std::vector<Ride> &rides,
                            std::size_t k = TOP_AREA_K);

#endif
