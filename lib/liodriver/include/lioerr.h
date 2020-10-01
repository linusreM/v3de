#pragma once

typedef enum{
    LIO_FAIL = -1,
    LIO_OK = 0,
    LIO_EINVAL = 100,
    LIO_EIO = 101,
    LIO_ENOMEM = 102
} lio_error_t;