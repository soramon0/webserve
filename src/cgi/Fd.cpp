#include "Fd.hpp"
#include "cgi_utils.hpp"

Fd::Fd(int& fd) : fd(fd), owns(true) {}

Fd::~Fd() { if (owns) close_wrapper(fd);}

int Fd::get() const { return (fd); }

void Fd::release() { owns = false; }