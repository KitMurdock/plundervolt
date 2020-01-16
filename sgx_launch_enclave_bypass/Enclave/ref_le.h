#include "sgx_report.h"

#if 0
    /* From https://github.com/intel/linux-sgx/blob/master/psw/ae/inc/internal/ref_le.h */
    
    /* All fields are big endian */
    
    #pragma pack(push, 1)
    
    typedef struct _ref_le_white_list_entry_t
    {
        uint8_t                     provision_key;
        uint8_t                     match_mr_enclave;
        uint8_t                     reserved[6]; // align the measurment on 64 bits
        sgx_measurement_t           mr_signer;
        sgx_measurement_t           mr_enclave;
    } ref_le_white_list_entry_t;
    
    #pragma pack(pop)
#else
    typedef struct _ref_le_white_list_entry_t
    {
        sgx_measurement_t           mr_signer;
        uint8_t                     provision_key;
    } ref_le_white_list_entry_t;
#endif
