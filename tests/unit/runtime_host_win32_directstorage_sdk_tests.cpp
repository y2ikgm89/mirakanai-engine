// SPDX-License-Identifier: MIT

#include <dstorage.h>
#include <dstorageerr.h>

int main() {
    static_assert(sizeof(DSTORAGE_REQUEST) > 0);
    static_assert(sizeof(DSTORAGE_CONFIGURATION1) > 0);

    auto* const factory_entry = &DStorageGetFactory;
    return factory_entry == nullptr ? 1 : 0;
}
