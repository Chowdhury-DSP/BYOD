#include "SharedJaiContext.h"
#include "byod_jai_lib.h"

JaiContextWrapper::JaiContextWrapper()
{
    internal = __jai_runtime_init (0, nullptr);
}

JaiContextWrapper::~JaiContextWrapper()
{
    __jai_runtime_fini (internal);
}
