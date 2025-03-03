#include <gtest/gtest.h>

#include "KokkosComm.hpp"

template <typename T> class SendRecv : public testing::Test {
public:
  using Scalar = T;
};

using ScalarTypes =
    ::testing::Types<int, int64_t, float, double, Kokkos::complex<float>,
                     Kokkos::complex<double>>;
TYPED_TEST_SUITE(SendRecv, ScalarTypes);

TYPED_TEST(SendRecv, 1D_contig) {
  Kokkos::View<typename TestFixture::Scalar *> a("a", 1000);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if (size < 2) {
    GTEST_SKIP() << "Requires >= 2 ranks (" << size << " provieded)";
  }

  if (0 == rank) {
    int dst = 1;
    Kokkos::parallel_for(
        a.extent(0), KOKKOS_LAMBDA(const int i) { a(i) = i; });
    KokkosComm::send(Kokkos::DefaultExecutionSpace(), a, dst, 0,
                     MPI_COMM_WORLD);
  } else if (1 == rank) {
    int src = 0;
    KokkosComm::recv(Kokkos::DefaultExecutionSpace(), a, src, 0,
                     MPI_COMM_WORLD);
    int errs;
    Kokkos::parallel_reduce(
        a.extent(0),
        KOKKOS_LAMBDA(const int &i, int &lsum) { lsum += a(i) != i; }, errs);
    ASSERT_EQ(errs, 0);
  }
}

TYPED_TEST(SendRecv, 1D_noncontig) {
  // this is C-style layout, i.e. b(0,0) is next to b(0,1)
  Kokkos::View<typename TestFixture::Scalar **, Kokkos::LayoutRight> b("a", 10,
                                                                       10);
  auto a = Kokkos::subview(b, 2, Kokkos::ALL); // take column 2 (non-contiguous)

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (0 == rank) {
    int dst = 1;
    Kokkos::parallel_for(
        a.extent(0), KOKKOS_LAMBDA(const int i) { a(i) = i; });
    KokkosComm::send(Kokkos::DefaultExecutionSpace(), a, dst, 0,
                     MPI_COMM_WORLD);
  } else if (1 == rank) {
    int src = 0;
    KokkosComm::recv(Kokkos::DefaultExecutionSpace(), a, src, 0,
                     MPI_COMM_WORLD);
    int errs;
    Kokkos::parallel_reduce(
        a.extent(0),
        KOKKOS_LAMBDA(const int &i, int &lsum) { lsum += a(i) != i; }, errs);
    ASSERT_EQ(errs, 0);
  }
}