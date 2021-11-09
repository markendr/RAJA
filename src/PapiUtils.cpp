#include <omp.h>

namespace RAJA {

unsigned long omp_get_thread_num_helper(void)
{
  return (unsigned long)omp_get_thread_num();
}

}
