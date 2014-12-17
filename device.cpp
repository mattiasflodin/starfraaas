#include "device.hpp"

state_set::~state_set()
{
}

device::~device()
{
}

vertex_producer::~vertex_producer()
{
}

texture_producer::~texture_producer()
{
}

index_producer::~index_producer()
{
}

// TODO consider moving to another file; this is a higher layer.
strategy::~strategy()
{
}
void strategy::update(state_set*)
{
}
void strategy::present(device* dev, state_set* states)
{
    dev->present(states);
}

vertex_program::~vertex_program()
{
}

fragment_shader::~fragment_shader()
{
}