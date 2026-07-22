#ifndef FD
#define FD

class Fd
{
private:
	int& fd;
	bool owns;

public:
	explicit Fd(int& fd);
	~Fd();

	int get() const;
	void release();

private:
	Fd(const Fd& other);
	Fd& operator=(const Fd& other);
};

#endif