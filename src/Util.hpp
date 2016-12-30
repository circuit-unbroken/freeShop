#ifndef FREESHOP_UTIL_HPP
#define FREESHOP_UTIL_HPP

#include <sys/stat.h>
#include <string>

namespace FreeShop
{

bool pathExists(const char* path, bool escape = true);
void makeDirectory(const char *dir, mode_t mode = 0777);
int removeDirectory(const char *path, bool onlyIfEmpty = false);
std::string getCountryCode(int region);

} // namespace FreeShop

#endif // FREESHOP_UTIL_HPP
