#if !defined(__ErrorDef_hdr__)
#define __ErrorDef_hdr__

enum class Error_t
{
    kNoError,

    kFileOpenError,
    kFileAccessError,

    kFunctionInvalidArgsError,

    kNotInitializedError,
    kFunctionIllegalCallError,
    kInvalidString,

    kMemError,

    kUnknownError,

    kNotImplementedError,

    kNumErrors
};
#endif // #if !defined(__ErrorDef_hdr__)



