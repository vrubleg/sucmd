#ifndef defer

template <typename T>
struct Deferrer
{
	T f;
	Deferrer(T f) : f(f) {};
	Deferrer(const Deferrer&) = delete;
	~Deferrer() { f(); }
};

#define TOKEN_CONCAT_NX(a, b) a ## b
#define TOKEN_CONCAT(a, b) TOKEN_CONCAT_NX(a, b)
#define defer Deferrer TOKEN_CONCAT(__deferred, __COUNTER__) =

#endif
