/*===- InstrProfiling.h- Support library for PGO instrumentation ----------===*\
|*
|* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
|* See https://llvm.org/LICENSE.txt for license information.
|* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
|*
\*===----------------------------------------------------------------------===*/

#ifndef PROFILE_INSTRPROFILING_H_
#define PROFILE_INSTRPROFILING_H_

#include "InstrProfilingPort.h"
#include <stdio.h>

// Make sure __LLVM_INSTR_PROFILE_GENERATE is always defined before
// including instr_prof_interface.h so the interface functions are
// declared correctly for the runtime.
// __LLVM_INSTR_PROFILE_GENERATE is always `#undef`ed after the header,
// because compiler-rt does not support profiling the profiling runtime itself.
#ifndef __LLVM_INSTR_PROFILE_GENERATE
#define __LLVM_INSTR_PROFILE_GENERATE
#endif
#include "profile/instr_prof_interface.h"
#undef __LLVM_INSTR_PROFILE_GENERATE

#define INSTR_PROF_VISIBILITY COMPILER_RT_VISIBILITY
#include "profile/InstrProfData.inc"

enum ValueKind {
#define VALUE_PROF_KIND(Enumerator, Value, Descr) Enumerator = Value,
#include "profile/InstrProfData.inc"
};

typedef void *IntPtrT;
typedef struct COMPILER_RT_ALIGNAS(INSTR_PROF_DATA_ALIGNMENT)
    __llvm_profile_data {
#define INSTR_PROF_DATA(Type, LLVMType, Name, Initializer) Type Name;
#include "profile/InstrProfData.inc"
} __llvm_profile_data;

typedef struct __llvm_profile_header {
#define INSTR_PROF_RAW_HEADER(Type, Name, Initializer) Type Name;
#include "profile/InstrProfData.inc"
} __llvm_profile_header;

typedef struct ValueProfNode * PtrToNodeT;
typedef struct ValueProfNode {
#define INSTR_PROF_VALUE_NODE(Type, LLVMType, Name, Initializer) Type Name;
#include "profile/InstrProfData.inc"
} ValueProfNode;

typedef struct COMPILER_RT_ALIGNAS(INSTR_PROF_DATA_ALIGNMENT) VTableProfData {
#define INSTR_PROF_VTABLE_DATA(Type, LLVMType, Name, Initializer) Type Name;
#include "profile/InstrProfData.inc"
} VTableProfData;

typedef struct COMPILER_RT_ALIGNAS(INSTR_PROF_DATA_ALIGNMENT)
    __llvm_gcov_init_func_struct {
#define COVINIT_FUNC(Type, LLVMType, Name, Initializer) Type Name;
#include "profile/InstrProfData.inc"
} __llvm_gcov_init_func_struct;

/*!
 * \brief Return 1 if profile counters are continuously synced to the raw
 * profile via an mmap(). This is in contrast to the default mode, in which
 * the raw profile is written out at program exit time.
 */
int __llvm_profile_is_continuous_mode_enabled(void);

/*!
 * \brief Enable continuous mode.
 *
 * See \ref __llvm_profile_is_continuous_mode_enabled. The behavior is undefined
 * if continuous mode is already enabled, or if it cannot be enable due to
 * conflicting options.
 */
void __llvm_profile_enable_continuous_mode(void);

/*!
 * \brief Disable continuous mode.
 *
 */
void __llvm_profile_disable_continuous_mode(void);

/*!
 * \brief Set the page size.
 *
 * This is a pre-requisite for enabling continuous mode. The buffer size
 * calculation code inside of libprofile cannot simply call getpagesize(), as
 * it is not allowed to depend on libc.
 */
void __llvm_profile_set_page_size(unsigned PageSize);

/*!
 * \brief Get number of bytes necessary to pad the argument to eight
 * byte boundary.
 */
uint8_t __llvm_profile_get_num_padding_bytes(uint64_t SizeInBytes);

/*!
 * \brief Get required size for profile buffer.
 */
uint64_t __llvm_profile_get_size_for_buffer(void);

/*!
 * \brief Write instrumentation data to the given buffer.
 *
 * \pre \c Buffer is the start of a buffer at least as big as \a
 * __llvm_profile_get_size_for_buffer().
 */
int __llvm_profile_write_buffer(char *Buffer);

const __llvm_profile_data *__llvm_profile_begin_data(void);
const __llvm_profile_data *__llvm_profile_end_data(void);
const char *__llvm_profile_begin_names(void);
const char *__llvm_profile_end_names(void);
const char *__llvm_profile_begin_vtabnames(void);
const char *__llvm_profile_end_vtabnames(void);
char *__llvm_profile_begin_counters(void);
char *__llvm_profile_end_counters(void);
char *__llvm_profile_begin_bitmap(void);
char *__llvm_profile_end_bitmap(void);
ValueProfNode *__llvm_profile_begin_vnodes(void);
ValueProfNode *__llvm_profile_end_vnodes(void);
const VTableProfData *__llvm_profile_begin_vtables(void);
const VTableProfData *__llvm_profile_end_vtables(void);

/*!
 * \brief Merge profile data from buffer.
 *
 * Read profile data from buffer \p Profile and merge with in-process profile
 * counters and bitmaps. The client is expected to have checked or already
 * know the profile data in the buffer matches the in-process counter
 * structure before calling it. Returns 0 (success) if the profile data is
 * valid. Upon reading invalid/corrupted profile data, returns 1 (failure).
 */
int __llvm_profile_merge_from_buffer(const char *Profile, uint64_t Size);

/*! \brief Check if profile in buffer matches the current binary.
 *
 *  Returns 0 (success) if the profile data in buffer \p Profile with size
 *  \p Size was generated by the same binary and therefore matches
 *  structurally the in-process counters and bitmaps. If the profile data in
 *  buffer is not compatible, the interface returns 1 (failure).
 */
int __llvm_profile_check_compatibility(const char *Profile,
                                       uint64_t Size);

/*!
 * \brief Counts the number of times a target value is seen.
 *
 * Records the target value for the CounterIndex if not seen before. Otherwise,
 * increments the counter associated w/ the target value.
 * void __llvm_profile_instrument_target(uint64_t TargetValue, void *Data,
 *                                       uint32_t CounterIndex);
 */
void INSTR_PROF_VALUE_PROF_FUNC(
#define VALUE_PROF_FUNC_PARAM(ArgType, ArgName, ArgLLVMType) ArgType ArgName
#include "profile/InstrProfData.inc"
    );

void __llvm_profile_instrument_target_value(uint64_t TargetValue, void *Data,
                                            uint32_t CounterIndex,
                                            uint64_t CounterValue);

/*!
 * \brief Write instrumentation data to the current file.
 *
 * Writes to the file with the last name given to \a *
 * __llvm_profile_set_filename(),
 * or if it hasn't been called, the \c LLVM_PROFILE_FILE environment variable,
 * or if that's not set, the last name set to INSTR_PROF_PROFILE_NAME_VAR,
 * or if that's not set,  \c "default.profraw".
 */
int __llvm_profile_write_file(void);

/*!
 * \brief Set the FILE object for writing instrumentation data. Return 0 if set
 * successfully or return 1 if failed.
 *
 * Sets the FILE object to be used for subsequent calls to
 * \a __llvm_profile_write_file(). The profile file name set by environment
 * variable, command-line option, or calls to \a  __llvm_profile_set_filename
 * will be ignored.
 *
 * \c File will not be closed after a call to \a __llvm_profile_write_file() but
 * it may be flushed. Passing NULL restores default behavior.
 *
 * If \c EnableMerge is nonzero, the runtime will always merge profiling data
 * with the contents of the profiling file. If EnableMerge is zero, the runtime
 * may still merge the data if it would have merged for another reason (for
 * example, because of a %m specifier in the file name).
 *
 * Note: There may be multiple copies of the profile runtime (one for each
 * instrumented image/DSO). This API only modifies the file object within the
 * copy of the runtime available to the calling image.
 *
 * Warning: This is a no-op if EnableMerge is 0 in continuous mode (\ref
 * __llvm_profile_is_continuous_mode_enabled), because disable merging requires
 * copying the old profile file to new profile file and this function is usually
 * used when the proess doesn't have permission to open file.
 */
int __llvm_profile_set_file_object(FILE *File, int EnableMerge);

/*! \brief Register to write instrumentation data to file at exit. */
int __llvm_profile_register_write_file_atexit(void);

/*! \brief Initialize file handling. */
void __llvm_profile_initialize_file(void);

/*! \brief Initialize the profile runtime. */
void __llvm_profile_initialize(void);

/*! \brief Initialize the gcov profile runtime. */
void __llvm_profile_gcov_initialize(void);

/*!
 * \brief Return path prefix (excluding the base filename) of the profile data.
 * This is useful for users using \c -fprofile-generate=./path_prefix who do
 * not care about the default raw profile name. It is also useful to collect
 * more than more profile data files dumped in the same directory (Online
 * merge mode is turned on for instrumented programs with shared libs).
 * Side-effect: this API call will invoke malloc with dynamic memory allocation.
 */
const char *__llvm_profile_get_path_prefix(void);

/*!
 * \brief Return filename (including path) of the profile data. Note that if the
 * user calls __llvm_profile_set_filename later after invoking this interface,
 * the actual file name may differ from what is returned here.
 * Side-effect: this API call will invoke malloc with dynamic memory allocation
 * (the returned pointer must be passed to `free` to avoid a leak).
 *
 * Note: There may be multiple copies of the profile runtime (one for each
 * instrumented image/DSO). This API only retrieves the filename from the copy
 * of the runtime available to the calling image.
 */
const char *__llvm_profile_get_filename(void);

/*! \brief Get the magic token for the file format. */
uint64_t __llvm_profile_get_magic(void);

/*! \brief Get the version of the file format. */
uint64_t __llvm_profile_get_version(void);

/*! \brief Get the number of entries in the profile data section. */
uint64_t __llvm_profile_get_num_data(const __llvm_profile_data *Begin,
                                     const __llvm_profile_data *End);

/*! \brief Get the size of the profile data section in bytes. */
uint64_t __llvm_profile_get_data_size(const __llvm_profile_data *Begin,
                                      const __llvm_profile_data *End);

/*! \brief Get the size in bytes of a single counter entry. */
size_t __llvm_profile_counter_entry_size(void);

/*! \brief Get the number of entries in the profile counters section. */
uint64_t __llvm_profile_get_num_counters(const char *Begin, const char *End);

/*! \brief Get the size of the profile counters section in bytes. */
uint64_t __llvm_profile_get_counters_size(const char *Begin, const char *End);

/*! \brief Get the number of bytes in the profile bitmap section. */
uint64_t __llvm_profile_get_num_bitmap_bytes(const char *Begin,
                                             const char *End);

/*! \brief Get the size of the profile name section in bytes. */
uint64_t __llvm_profile_get_name_size(const char *Begin, const char *End);

/*! \brief Get the number of virtual table profile data entries */
uint64_t __llvm_profile_get_num_vtable(const VTableProfData *Begin,
                                       const VTableProfData *End);

/*! \brief Get the size of virtual table profile data in bytes. */
uint64_t __llvm_profile_get_vtable_section_size(const VTableProfData *Begin,
                                                const VTableProfData *End);

/* ! \brief Given the sizes of the data and counter information, computes the
 * number of padding bytes before and after the counter section, as well as the
 * number of padding bytes after other sections in the raw profile.
 * Returns -1 upon errors and 0 upon success. Output parameters should be used
 * iff return value is 0.
 *
 * Note: When mmap() mode is disabled, no padding bytes before/after counters
 * are needed. However, in mmap() mode, the counter section in the raw profile
 * must be page-aligned: this API computes the number of padding bytes
 * needed to achieve that.
 */
int __llvm_profile_get_padding_sizes_for_counters(
    uint64_t DataSize, uint64_t CountersSize, uint64_t NumBitmapBytes,
    uint64_t NamesSize, uint64_t VTableSize, uint64_t VNameSize,
    uint64_t *PaddingBytesBeforeCounters, uint64_t *PaddingBytesAfterCounters,
    uint64_t *PaddingBytesAfterBitmap, uint64_t *PaddingBytesAfterNames,
    uint64_t *PaddingBytesAfterVTable, uint64_t *PaddingBytesAfterVNames);

/*!
 * \brief Set the flag that profile data has been dumped to the file.
 * This is useful for users to disable dumping profile data to the file for
 * certain processes in case the processes don't have permission to write to
 * the disks, and trying to do so would result in side effects such as crashes.
 */
void __llvm_profile_set_dumped(void);

/*!
 * \brief Write custom target-specific profiling data to a separate file.
 * Used by offload PGO.
 */
int __llvm_write_custom_profile(const char *Target,
                                const __llvm_profile_data *DataBegin,
                                const __llvm_profile_data *DataEnd,
                                const char *CountersBegin,
                                const char *CountersEnd, const char *NamesBegin,
                                const char *NamesEnd,
                                const uint64_t *VersionOverride);

/*!
 * This variable is defined in InstrProfilingRuntime.cpp as a hidden
 * symbol. Its main purpose is to enable profile runtime user to
 * bypass runtime initialization code -- if the client code explicitly
 * define this variable, then InstProfileRuntime.o won't be linked in.
 * Note that this variable's visibility needs to be hidden so that the
 * definition of this variable in an instrumented shared library won't
 * affect runtime initialization decision of the main program.
 *  __llvm_profile_profile_runtime. */
COMPILER_RT_VISIBILITY extern int INSTR_PROF_PROFILE_RUNTIME_VAR;

/*!
 * This variable is defined in InstrProfilingVersionVar.c as a hidden symbol
 * (except on Apple platforms where this symbol is checked by TAPI).  Its main
 * purpose is to encode the raw profile version value and other format related
 * information such as whether the profile is from IR based instrumentation. The
 * variable is defined as weak so that compiler can emit an overriding
 * definition depending on user option.
 */
COMPILER_RT_VISIBILITY extern uint64_t
    INSTR_PROF_RAW_VERSION_VAR; /* __llvm_profile_raw_version */

/*!
 * This variable is a weak symbol defined in InstrProfiling.c. It allows
 * compiler instrumentation to provide overriding definition with value
 * from compiler command line. This variable has default visibility.
 */
extern char INSTR_PROF_PROFILE_NAME_VAR[1]; /* __llvm_profile_filename. */

const __llvm_gcov_init_func_struct *__llvm_profile_begin_covinit();
const __llvm_gcov_init_func_struct *__llvm_profile_end_covinit();
#endif /* PROFILE_INSTRPROFILING_H_ */
