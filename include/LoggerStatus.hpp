/**
 * Status code definitions for the mm_logger library
 */

#ifndef LOGGER_STATUS_HPP
#define LOGGER_STATUS_HPP

namespace mm {

// Error/status codes
#ifndef MM_STATUS_OK
#define MM_STATUS_OK 0
#endif

#ifndef MM_STATUS_ERROR
#define MM_STATUS_ERROR 1
#endif

#ifndef MM_STATUS_ENOMEM
#define MM_STATUS_ENOMEM 9
#endif

#ifndef MM_STATUS_ENOENT
#define MM_STATUS_ENOENT 5
#endif

}  // namespace mm

#endif  // LOGGER_STATUS_HPP
