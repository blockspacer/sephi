﻿#include "sephi/ipc/shm/shared_memory.h"

#include "sephi/ipc/shm/types.h"


using std::make_unique;
using boost_ipc::create_only;
using boost_ipc::open_only;


sephi::ipc::SharedMemory::SharedMemory(
    ShmCreateOnly, std::string const& name, size_t size)
    : name_{name}
{
    memory_ = make_unique<ManagedSharedMemory>(create_only, name_.c_str(), size);
}

sephi::ipc::SharedMemory::SharedMemory(ShmOpenOnly, std::string const& name)
    : name_{name}
{
    memory_ = make_unique<ManagedSharedMemory>(open_only, name_.c_str());
}

sephi::ipc::SharedMemory::~SharedMemory() noexcept
{
    destroy();
}

size_t sephi::ipc::SharedMemory::free_size() const
{
    return memory_->get_free_memory();
}

std::string sephi::ipc::SharedMemory::name() const
{
    return name_;
}


void sephi::ipc::SharedMemory::destroy()
{
#if !_WIN32
    ManagedSharedMemory::remove(name_.c_str());
#endif
}