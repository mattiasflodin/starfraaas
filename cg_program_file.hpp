#include <vector>
#include "device.hpp"

class cg_program_file : public vertex_program, public fragment_shader {
public:
    cg_program_file(char const* path);
    language_t language() const;
    std::size_t char_count() const;
    void produce_vertex_program(char* program) const;
	void produce_fragment_shader(char* program) const;
private:
    std::vector<char> program_;
};
