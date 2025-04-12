#ifndef	VirtualReloads_VERSION_INCLUDED
#define VirtualReloads_VERSION_INCLUDED

#define MAKE_STR_HELPER(a_str) #a_str
#define MAKE_STR(a_str) MAKE_STR_HELPER(a_str)

#define VirtualReloads_VERSION_MAJOR	0
#define VirtualReloads_VERSION_MINOR	0
#define VirtualReloads_VERSION_PATCH	0
#define VirtualReloads_VERSION_BETA	1
#define VirtualReloads_VERSION_VERSTRING	MAKE_STR(VirtualReloads_VERSION_MAJOR) "." MAKE_STR(VirtualReloads_VERSION_MINOR) "." MAKE_STR(VirtualReloads_VERSION_PATCH) "." MAKE_STR(VirtualReloads_VERSION_BETA)

#endif
