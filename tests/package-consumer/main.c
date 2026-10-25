#include <intel_pmu.h>

int main(void) {
  struct intel_pmu pmu;
  struct intel_pmu_utilization utilization;

  intel_pmu_initialize(&pmu);
  intel_pmu_read(&pmu, &utilization);
  intel_pmu_close(&pmu);

  for (int engine_class = 0; engine_class < intel_pmu_engine_class_count; ++engine_class) {
    if (utilization.valid[engine_class])
      return 1;
  }
  return 0;
}
