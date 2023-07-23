#include <stdint.h>

typedef uint64_t     u64;
typedef uint32_t     u32;
typedef uint16_t     u16;
typedef uint8_t      u8;
typedef int64_t      s64;
typedef int32_t      s32;
typedef int16_t      s16;
typedef int8_t       s8;
typedef double       float64;
typedef float        float32;

struct Type_Info_Struct;

#ifdef __cplusplus
extern "C" {
    #define null (0)
#else
    typedef s8          bool;
    const bool false = 0;
    const bool true = 1;
#endif

struct jai_Type_Info;
struct jai_string;
struct jai_Array_View_64;
struct jai_Type_Info_Struct;
struct jai_Allocator;
struct jai_Source_Code_Location;
struct jai_Log_Section;
struct jai_Log_Info;
struct jai_Overflow_Page;
struct jai_Temporary_Storage;
struct jai_Dynamic_Context_Entry;
struct jai_Stack_Trace_Procedure_Info;
struct jai_Stack_Trace_Node;
struct jai_Context_Base;
struct jai_Any;
struct jai_Formatter;
struct jai_FormatInt;
struct jai_FormatFloat;
struct jai_FormatStruct;
struct jai_FormatArray;
struct jai_Buffer;
struct jai_String_Builder;
struct jai_Print_Style;
struct jai_Context;
struct jai_Krusher_Lofi_Resample_State;

enum jai_Type_Info_Tag {
    INTEGER = 0,
    FLOAT = 1,
    BOOL = 2,
    STRING = 3,
    POINTER = 4,
    PROCEDURE = 5,
    VOID = 6,
    STRUCT = 7,
    ARRAY = 8,
    OVERLOAD_SET = 9,
    ANY = 10,
    ENUM = 11,
    POLYMORPHIC_VARIABLE = 12,
    TYPE = 13,
    CODE = 14,
    VARIANT = 18,
};

typedef struct jai_Type_Info {
    u32 type;
    s64 runtime_size;
} jai_Type_Info;

typedef struct jai_string {
    s64 count;
    u8 *data;
} jai_string;

typedef struct jai_Array_View_64 {
    s64 count;
    u8 *data;
} jai_Array_View_64;

enum jai_Struct_Status_Flags {
    INCOMPLETE = 1,
    LOCAL = 4,
};

enum jai_Struct_Nontextual_Flags {
    NOT_INSTANTIABLE = 4,
    ALL_MEMBERS_UNINITIALIZED = 64,
    POLYMORPHIC = 256,
};

enum jai_Struct_Textual_Flags {
    FOREIGN = 1,
    UNION = 2,
    NO_PADDING = 4,
    TYPE_INFO_NONE = 8,
    TYPE_INFO_NO_SIZE_COMPLAINT = 16,
    TYPE_INFO_PROCEDURES_ARE_VOID_POINTERS = 32,
};

typedef struct jai_Type_Info_Struct {
    struct jai_Type_Info info;
    jai_string name;
    jai_Array_View_64 specified_parameters;
    jai_Array_View_64 members;
    u32 status_flags;
    u32 nontextual_flags;
    u32 textual_flags;
    struct jai_Type_Info_Struct *polymorph_source_struct;
    void *initializer;
    jai_Array_View_64 constant_storage;
    jai_Array_View_64 notes;
} jai_Type_Info_Struct;

enum jai_Allocator_Mode {
    ALLOCATE = 0,
    RESIZE = 1,
    FREE = 2,
    STARTUP = 3,
    SHUTDOWN = 4,
    THREAD_START = 5,
    THREAD_STOP = 6,
    CREATE_HEAP = 7,
    DESTROY_HEAP = 8,
    IS_THIS_YOURS = 9,
    CAPS = 10,
};

typedef struct jai_Allocator {
    void *proc;
    void *data;
} jai_Allocator;

typedef struct jai_Source_Code_Location {
    jai_string fully_pathed_filename;
    s64 line_number;
    s64 character_number;
} jai_Source_Code_Location;

enum jai_Log_Flags {
    NONE = 0,
    ERROR = 1,
    WARNING = 2,
    CONTENT = 4,
    TO_FILE_ONLY = 8,
    VERBOSE_ONLY = 16,
    VERY_VERBOSE_ONLY = 32,
    TOPIC_ONLY = 64,
};

typedef struct jai_Log_Section {
    jai_string name;
} jai_Log_Section;

typedef struct jai_Log_Info {
    u64 source_identifier;
    struct jai_Source_Code_Location location;
    u32 common_flags;
    u32 user_flags;
    struct jai_Log_Section *section;
} jai_Log_Info;

enum jai_Log_Level {
    NORMAL = 0,
    VERBOSE = 1,
    VERY_VERBOSE = 2,
};

typedef struct jai_Overflow_Page {
    struct jai_Overflow_Page *next;
    struct jai_Allocator allocator;
    s64 size;
} jai_Overflow_Page;

typedef struct jai_Temporary_Storage {
    u8 *data;
    s64 size;
    s64 current_page_bytes_occupied;
    s64 total_bytes_occupied;
    s64 high_water_mark;
    struct jai_Source_Code_Location last_set_mark_location;
    struct jai_Allocator overflow_allocator;
    struct jai_Overflow_Page *overflow_pages;
    u8 *original_data;
    s64 original_size;
} jai_Temporary_Storage;

typedef struct jai_Dynamic_Context_Entry {
    struct jai_Type_Info *type;
    void *data;
} jai_Dynamic_Context_Entry;

typedef struct jai_Stack_Trace_Procedure_Info {
    jai_string name;
    struct jai_Source_Code_Location location;
    void *procedure_address;
} jai_Stack_Trace_Procedure_Info;

typedef struct jai_Stack_Trace_Node {
    struct jai_Stack_Trace_Node *next;
    struct jai_Stack_Trace_Procedure_Info *info;
    u64 hash;
    u32 call_depth;
    u32 line_number;
} jai_Stack_Trace_Node;

typedef struct jai_Context_Base {
    struct jai_Type_Info_Struct *context_info;
    u32 thread_index;
    struct jai_Allocator allocator;
    void *logger;
    void *logger_data;
    u64 log_source_identifier;
    u8 log_level;
    struct jai_Temporary_Storage *temporary_storage;
    struct jai_Dynamic_Context_Entry dynamic_entries[16];
    s32 num_dynamic_entries;
    struct jai_Stack_Trace_Node *stack_trace;
    void *assertion_failed;
    bool handling_assertion_failure;
} jai_Context_Base;

typedef struct jai_Any {
    struct jai_Type_Info *type;
    void *value_pointer;
} jai_Any;

typedef struct jai_Formatter {
    jai_Any value;
} jai_Formatter;

typedef struct jai_FormatInt {
    struct jai_Formatter formatter;
    s64 base;
    s64 minimum_digits;
    u8 padding;
    u16 digits_per_comma;
    jai_string comma_string;
} jai_FormatInt;

enum jai_Zero_Removal {
    YES = 0,
    NO = 1,
    ONE_ZERO_AFTER_DECIMAL = 2,
};

enum jai_Mode {
    DECIMAL = 0,
    SCIENTIFIC = 1,
    SHORTEST = 2,
};

typedef struct jai_FormatFloat {
    struct jai_Formatter formatter;
    s64 width;
    s64 trailing_width;
    s64 zero_removal;
    s64 mode;
} jai_FormatFloat;

typedef struct jai_FormatStruct {
    struct jai_Formatter formatter;
    bool draw_type_name;
    s64 use_long_form_if_more_than_this_many_members;
    jai_string separator_between_name_and_value;
    jai_string short_form_separator_between_fields;
    jai_string long_form_separator_between_fields;
    jai_string begin_string;
    jai_string end_string;
    s32 indentation_width;
    bool use_newlines_if_long_form;
} jai_FormatStruct;

typedef struct jai_FormatArray {
    struct jai_Formatter formatter;
    jai_string separator;
    jai_string begin_string;
    jai_string end_string;
    jai_string printing_stopped_early_string;
    bool draw_separator_after_last_element;
    s64 stop_printing_after_this_many_elements;
} jai_FormatArray;

typedef struct jai_Buffer {
    s64 count;
    s64 allocated;
    s64 ensured_count;
    struct jai_Buffer *next;
} jai_Buffer;

typedef struct jai_String_Builder {
    bool initialized;
    bool failed;
    s64 subsequent_buffer_size;
    struct jai_Allocator allocator;
    struct jai_Buffer *current_buffer;
    u8 initial_bytes[4064];
} jai_String_Builder;

typedef struct jai_Print_Style {
    struct jai_FormatInt default_format_int;
    struct jai_FormatFloat default_format_float;
    struct jai_FormatStruct default_format_struct;
    struct jai_FormatArray default_format_array;
    struct jai_FormatInt default_format_absolute_pointer;
    void *struct_printer;
    void *struct_printer_data;
    s32 indentation_depth;
    bool log_runtime_errors;
} jai_Print_Style;

typedef struct jai_Context {
    struct jai_Context_Base base;
    struct jai_Print_Style print_style;
} jai_Context;

typedef struct jai_Krusher_Lofi_Resample_State {
    double upsample_overshoot;
    double downsample_overshoot;
} jai_Krusher_Lofi_Resample_State;


void __jai_runtime_fini(void *_context);

struct jai_Context * __jai_runtime_init(s32 argc, u8 **argv);

void krusher_process_lofi_downsample(struct jai_Context *ctx, struct jai_Krusher_Lofi_Resample_State *state, float **buffer, s32 num_channels, s32 num_samples, double resample_factor);

void krusher_init_lofi_resample(struct jai_Krusher_Lofi_Resample_State *state);


#ifdef __cplusplus
} // extern "C"
#endif
