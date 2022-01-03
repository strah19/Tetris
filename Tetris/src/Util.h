#ifndef UTIL
#define UTIL

#include  <random>
#include  <iterator>

template<typename Iter, typename RandomGenerator>
Iter SelectRandomly(Iter start, Iter end, RandomGenerator& g) {
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template<typename Iter>
Iter SelectRandomly(Iter start, Iter end) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return SelectRandomly(start, end, gen);
}

#endif //!UTIL
