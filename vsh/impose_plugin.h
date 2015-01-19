// Mysis impose_plugin.h v0.1
typedef struct
{
	int (*DoUnk0)(int,void *);
	int (*DoUnk1)();
	int (*DoUnk2)();
	int (*DoUnk3)(int);
	int (*DoUnk4)();
	int (*DoUnk5)();
	int (*DoUnk6)();
	int (*DoUnk7)();
	int (*DoUnk8)();
	int (*DoUnk9)();
	int (*DoUnk10)();
	int (*DoUnk11)();
	int (*DoUnk12)();
	int (*DoUnk13_start_xmb)(int);
	int (*DoUnk14)();
	int (*DoUnk15)();
	int (*DoUnk16)();
	int (*DoUnk17)(int);
	int (*DoUnk18)();
} impose_plugin_interface;

impose_plugin_interface * impose_interface;
