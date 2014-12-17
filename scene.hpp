#ifndef SCENE_HPP
#define SCENE_HPP
#include <vector>
#include <set>
#include "device.hpp"

class renderable;

class batch {
public:
    virtual ~batch() {}
	virtual void add(renderable*) = 0;
	virtual void remove(renderable*) = 0;
    virtual state_set* create_state_set() = 0;
	virtual void remove_state_set(state_set* s) = 0;
	virtual void update(float t) = 0;
    virtual void present() = 0;
};

class renderable {
public:
	void attach(batch* b)
	{
		b->add(this);
		init_state_sets(b);
	}
	void detach(batch *b)
	{
		remove_state_sets(b);
		b->remove(this);
	}
	virtual void update(float t);

protected:
	virtual void init_state_sets(batch* b) = 0;
	virtual void remove_state_sets(batch* b) = 0;
};

class sorted_batch : public batch {
public:
    sorted_batch(device* dev);
	void add(renderable*);
	void remove(renderable*);
    state_set* create_state_set();
	void remove_state_set(state_set* s);
	void update(float t);
    void present();

private:
    typedef std::vector<state_set*> state_set_list;
	typedef std::set<renderable*> renderable_set;
    device* device_;
    state_set_list state_sets_;
	renderable_set renderables_;
};

inline sorted_batch::sorted_batch(device* dev) :
    device_(dev)
{
}
#endif
