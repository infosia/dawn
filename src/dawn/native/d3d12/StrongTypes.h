#include <cstdint>
#include <concepts>
#include <type_traits>

struct UnitBase {};

template <typename T, int Tag>
struct Unit : UnitBase {
    T value;
    constexpr Unit(T v) : value(v) {}
    constexpr T Value() const { return value; }
};

struct NoneType {};

template <typename T, typename N, typename D=NoneType>
struct Ratio {
    T value;
    constexpr Ratio() = default;
    constexpr Ratio(T v) : value(v) {}
};

// Allow basic arithmetic of the same types
// Unit + Unit
template <typename T, int Tag>
constexpr Unit<T, Tag> operator+(Unit<T, Tag> lhs, Unit<T, Tag> rhs) { return Unit<T, Tag>{lhs.value + rhs.value}; }
// Unit - Unit
template <typename T, int Tag>
constexpr Unit<T, Tag> operator-(Unit<T, Tag> lhs, Unit<T, Tag> rhs) { return Unit<T, Tag>{lhs.value - rhs.value}; }


// Ratio<N,D> + Ratio<N,D> = Ratio<N,D>
template <typename T, typename N, typename D>
constexpr Ratio<T, N, D> operator+(Ratio<T, N, D> lhs, Ratio<T, N, D> rhs) {
    return Ratio<T, N, D>{lhs.value + rhs.value};
}

// Ratio<N,D> - Ratio<N,D> = Ratio<N,D>
template <typename T, typename N, typename D>
constexpr Ratio<T, N, D> operator-(Ratio<T, N, D> lhs, Ratio<T, N, D> rhs) {
    return Ratio<T, N, D>{lhs.value - rhs.value};
}

// Ratio<N,D> * Ratio<D> = Ratio<N>
template <typename T, typename N, typename D>
constexpr Ratio<T, N> operator*(Ratio<T, N, D> lhs, Ratio<T, D> rhs) {
    return Ratio<T, N>{lhs.value * rhs.value};
}

// Ratio<N,D> * D = N --> Conflict with Ratio * Ratio? Restrict D type?
template <typename T, typename N, typename D>
constexpr N operator*(Ratio<T, N, D> lhs, D rhs) {
    return N{lhs.value * rhs.value};
}

// Ratio<N> / Ratio<N,D> = Ratio<D>
template <typename T, typename N, typename D>
constexpr Ratio<T, D> operator/(Ratio<T, N> lhs, Ratio<T, N, D> rhs) {
    return Ratio<T, D>{lhs.value * rhs.value};
}

// // N / Ratio<N,D> = Ratio<D>
// template <typename T, typename N, typename D>
// constexpr Ratio<T, D> operator/(N lhs, Ratio<T, N, D> rhs) {
//     return Ratio<T, D>{lhs.value / rhs.value};
// }

// N / Ratio<N,D> = D
template <typename T, typename N, typename D>
constexpr D operator/(N lhs, Ratio<T, N, D> rhs) {
    return D{lhs.value / rhs.value};
}


// Tests
#include <functional>
namespace Tests {
template <typename Lhs, typename Op, typename Rhs, typename Result>
struct LhsOpRhsIsResult {
    // static_assert(std::is_same_v<decltype(Op{}(std::declval<Lhs>(), std::declval<Rhs>())), Result>);
    static constexpr bool value = std::is_same_v<decltype(Op{}(std::declval<Lhs>(), std::declval<Rhs>())), Result>;
};
template <typename Lhs, typename Op, typename Rhs, typename Result>
constexpr bool LhsOpRhsIsResult_v = LhsOpRhsIsResult<Lhs, Op, Rhs, Result>::value;

template<typename T, typename U>
concept CanAdd = requires(T t, U u) { t + u; };

template<typename T, typename U>
concept CanSubtract = requires(T t, U u) { t - u; };

template<typename T, typename U>
concept CanMultiply = requires(T t, U u) { t * u; };

template<typename T, typename U>
concept CanDivide = requires(T t, U u) { t / u; };

static_assert(CanAdd<Unit<uint32_t, 0>, Unit<uint32_t, 0>>);
static_assert(!CanAdd<Unit<uint32_t, 0>, Unit<uint32_t, 1>>); // Different tag
static_assert(!CanAdd<Unit<uint32_t, 0>, Unit<int32_t, 0>>); // Different storage type
static_assert(LhsOpRhsIsResult_v<Unit<uint32_t, 0>, std::plus<>, Unit<uint32_t, 0>, Unit<uint32_t, 0>>);

static_assert(CanSubtract<Unit<uint32_t, 0>, Unit<uint32_t, 0>>);
static_assert(!CanSubtract<Unit<uint32_t, 0>, Unit<uint32_t, 1>>); // Different tag
static_assert(!CanSubtract<Unit<uint32_t, 0>, Unit<int32_t, 0>>); // Different storage type
static_assert(LhsOpRhsIsResult_v<Unit<uint32_t, 0>, std::minus<>, Unit<uint32_t, 0>, Unit<uint32_t, 0>>);

static_assert(CanAdd<Ratio<uint32_t, int, float>, Ratio<uint32_t, int, float>>);
static_assert(!CanAdd<Ratio<uint32_t, int, float>, Ratio<uint32_t, float, int>>); // Swap N and D
static_assert(CanAdd<Ratio<uint32_t, int>, Ratio<uint32_t, int>>);
static_assert(!CanAdd<Ratio<uint32_t, int>, Ratio<uint32_t, float>>); // Different Ns
static_assert(!CanAdd<Ratio<uint32_t, int>, Ratio<int32_t, int>>); // Different storage type
static_assert(LhsOpRhsIsResult_v<Unit<uint32_t, 0>, std::plus<>, Unit<uint32_t, 0>, Unit<uint32_t, 0>>);
}
