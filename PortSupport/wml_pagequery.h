#pragma once
#include <mach/mach.h>
#include <mach/vm_region.h>
#include <cstdint>
#include <cstddef>
#include <limits>
inline bool WmlHostRangeReadable(uintptr_t p, size_t length) {
    if(!length) return true;
    if(length > std::numeric_limits<uintptr_t>::max()-p) return false;
    auto end=p+length;
    while(p<end) {
        vm_address_t address=p; vm_size_t size=0;
        vm_region_basic_info_data_64_t info{};
        mach_msg_type_number_t count=VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t object=MACH_PORT_NULL;
        auto result=vm_region_64(mach_task_self(),&address,&size,VM_REGION_BASIC_INFO_64,reinterpret_cast<vm_region_info_t>(&info),&count,&object);
        if(object!=MACH_PORT_NULL) mach_port_deallocate(mach_task_self(),object);
        if(result!=KERN_SUCCESS || address>p || !(info.protection&VM_PROT_READ) || size>std::numeric_limits<uintptr_t>::max()-address || address+size<=p) return false;
        p=address+size;
    }
    return true;
}
