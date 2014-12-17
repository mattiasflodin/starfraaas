#include <fstream>
#include <cassert>
#include "cg_program_file.hpp"

vertex_program::language_t cg_program_file::language() const
{
    return CG;
}

cg_program_file::cg_program_file(char const* path)
{
    std::ifstream ifs(path);
    assert(ifs.good());
    ifs.seekg(0, std::ios_base::end);
    std::size_t size = static_cast<std::size_t>(ifs.tellg());
    ifs.seekg(0, std::ios_base::beg);
    program_.resize(size);
    ifs.read(&program_[0], size);
}

std::size_t cg_program_file::char_count() const
{
    return program_.size();
}

void cg_program_file::produce_vertex_program(char* program) const
{
    std::copy(program_.begin(), program_.end(), program);
}

void cg_program_file::produce_fragment_shader(char* program) const
{
    std::copy(program_.begin(), program_.end(), program);
}
