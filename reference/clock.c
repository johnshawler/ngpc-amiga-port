/* Host oracle clock interposer. No changes to the pinned upstream core. */
#include <time.h>
static time_t epoch = 946684800;
void reference_set_time(long long value) { epoch = (time_t)value; }
time_t time(time_t *out) { if (out) *out = epoch; return epoch; }
