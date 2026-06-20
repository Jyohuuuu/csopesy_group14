#include <mutex>

// to avoid interleaving of prints from different threads, we use a global mutex that all PrintCommands lock before printing
inline std::mutex g_outputMutex;