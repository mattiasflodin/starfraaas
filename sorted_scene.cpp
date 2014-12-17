#include "scene.hpp"
#include "utility.hpp"

void renderable::update(float)
{
}

void sorted_batch::add(renderable* b)
{
	renderables_.insert(b);
}
void sorted_batch::remove(renderable* b)
{
	renderables_.erase(b);
}

state_set* sorted_batch::create_state_set()
{
    state_set* s = device_->create_state_set();
    state_sets_.push_back(s);
    return s;
}
void sorted_batch::remove_state_set(state_set* s)
{
	//std::find(
}

void sorted_batch::update(float t)
{
	FOR_EACH(renderable_set, it, renderables_)
	{
		(*it)->update(t);
	}
}

void sorted_batch::present()
{
    using std::swap;

    state_set_list::iterator it = state_sets_.begin();
    state_set_list::const_iterator end = state_sets_.end();
    if(it == end)
        return;

    state_set_list::iterator previous = it;
    device_->present(*it);
    ++it;

    while(it != end) {
        device_->present(*it);
        // Partial bubble sort
        //if((*it)->less(*previous))
        //    swap(*it, *previous);
        previous = it;
        ++it;
    }
}
