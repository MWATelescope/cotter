#ifndef OFFRINGASTMAN_REGISTER_H
#define OFFRINGASTMAN_REGISTER_H

extern "C" {
	/**
	 * This function makes the OffringaStMan known to casacore. The function
	 * is necessary for loading the storage manager from a shared library. It
	 * should have this specific name ("register_" + storage manager's name in
	 * lowercase) to be able to be automatically called when the library is
	 * loaded.
	 */
	void register_offringastman();
}

#endif
