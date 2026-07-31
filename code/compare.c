#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <gmp.h>
#include <mpfr.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "../header/compare_api.h"
#include "../header/entire_combination.h"
#include "../header/TIling_function_Simplified.h"
#include "../header/Tiling_function.h"

enum {
    CMP_EXIT_OK = 0,
    CMP_EXIT_USAGE = 1,
    CMP_EXIT_PARSE = 2,
    CMP_EXIT_RUNTIME = 3
};

#define INPUT_BUFFER_SIZE 512
#define MAX_INPUT_ATTEMPTS 3
#define MAX_SAMPLING_ARRAYS 128
#define MAX_INPUT_TOKENS 5
#define SPIRAL_VALUE_BUFFER_SIZE 256
#define SPIRAL_COMMAND_BUFFER_SIZE 4096
#define SPIRAL_LINE_BUFFER_SIZE 2048
#define SPIRAL_READ_TIMEOUT_SECONDS 10

typedef int (*sampling_parse_fn)(int argc, char **argv, sample_array_list *sal);

typedef struct {
    const char *name;
    size_t arg_count;
    sampling_parse_fn parse;
} sampling_spec;

typedef enum {
    USER_OUTPUT_I64,
    USER_OUTPUT_U64,
    USER_OUTPUT_FLOAT,
    USER_OUTPUT_DOUBLE,
    USER_OUTPUT_MPZ,
    USER_OUTPUT_MPFR
} user_output_type;

typedef struct {
    char path[INPUT_BUFFER_SIZE];
    char function_name[INPUT_BUFFER_SIZE];
    user_output_type output_type;
} function_info;

typedef struct {
#ifdef _WIN32
    HMODULE handle;
#else
    void *handle;
#endif
    compare_target_fn function;
} loaded_function;

//struct to help store the info of spiral process and communication
typedef struct {
    char executable[INPUT_BUFFER_SIZE];
    char txt_path[INPUT_BUFFER_SIZE];
    char *setup_text;
    char *call_template;
#ifndef _WIN32
    pid_t pid;
    int stdin_fd;
    int stdout_fd;
#endif
} spiral_target;

typedef struct {
    user_output_type type;
    union {
        int64_t i64;
        uint64_t u64;
        float f32;
        double f64;
    } native;
    mpz_t mpz;
    mpfr_t mpfr;
} function_result;

typedef enum {
    READ_LINE_OK,
    READ_LINE_EOF,
    READ_LINE_TOO_LONG
} read_line_status;

static int parse_simplified_uint(int argc, char **argv, sample_array_list *sal);
static int parse_simplified_int(int argc, char **argv, sample_array_list *sal);
static int parse_simplified_float(int argc, char **argv, sample_array_list *sal);
static int parse_umpz(int argc, char **argv, sample_array_list *sal);
static int parse_mpz(int argc, char **argv, sample_array_list *sal);
static int parse_mpfr(int argc, char **argv, sample_array_list *sal);

//array that contains the specification of each sampling array
static const sampling_spec sampling_specs[] = {
    { "uint", 3, parse_simplified_uint },
    { "int", 3, parse_simplified_int },
    { "float", 3, parse_simplified_float },
    { "umpz", 3, parse_umpz },
    { "mpz", 3, parse_mpz },
    { "mpfr", 5, parse_mpfr },
};

//puts forward the prompt and readline and decide the validity of a line
static read_line_status read_line(const char *prompt, char *buffer, size_t capacity)
{
    size_t length;
    int ch;

    fputs(prompt, stdout);
    fflush(stdout);

    if (fgets(buffer, (int)capacity, stdin) == NULL) {
        return READ_LINE_EOF;
    }

    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
        if (length > 0 && buffer[length - 1] == '\r') {
            buffer[length - 1] = '\0';
        }
        return READ_LINE_OK;
    }

    if (feof(stdin)) {
        return READ_LINE_OK;
    }

    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
    return READ_LINE_TOO_LONG;
}

//split a string apart according to space
static size_t split_tokens(char *line, char **tokens, size_t capacity)
{
    size_t count = 0;
    char *token = strtok(line, " \t");

    while (token != NULL) {
        if (count == capacity) {
            return capacity + 1;
        }
        tokens[count++] = token;
        token = strtok(NULL, " \t");
    }

    return count;

}

//determine if the output type is correct
static bool parse_output_type(const char *text, user_output_type *out)
{
    if (strcmp(text, "int") == 0 || strcmp(text, "int64") == 0) {
        *out = USER_OUTPUT_I64;
        return true;
    }
    if (strcmp(text, "uint") == 0 || strcmp(text, "uint64") == 0) {
        *out = USER_OUTPUT_U64;
        return true;
    }
    if (strcmp(text, "float") == 0) {
        *out = USER_OUTPUT_FLOAT;
        return true;
    }
    if (strcmp(text, "double") == 0) {
        *out = USER_OUTPUT_DOUBLE;
        return true;
    }
    if (strcmp(text, "mpz") == 0) {
        *out = USER_OUTPUT_MPZ;
        return true;
    }
    if (strcmp(text, "mpfr") == 0) {
        *out = USER_OUTPUT_MPFR;
        return true;
    }

    return false;
}

//determine if the path is valid
static bool file_exists(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    fclose(file);
    return true;
}

//read and decide if the function information typed in is valid
static int read_function_info(const char *prompt, function_info *out)
{
    char line[INPUT_BUFFER_SIZE];
    char *tokens[3];

    for (int attempt = 1; attempt <= MAX_INPUT_ATTEMPTS; attempt++) {
        read_line_status status = read_line(prompt, line, sizeof(line));
        if (status == READ_LINE_TOO_LONG) {
            fprintf(stderr, "Error: input line is too long.\n");
            return -1;
        }
        if (status == READ_LINE_EOF) {
            fprintf(stderr, "Error: failed to read input.\n");
            return -1;
        }

        size_t token_count = split_tokens(line, tokens, 3);
        if (token_count < 3) {
            fprintf(stderr,
                    "Error: too few inputs; enter a DLL path, function name, and output type.\n");
        } else if (token_count > 3) {
            fprintf(stderr,
                    "Error: too many inputs; enter exactly a DLL path, function name, and output type.\n");
        } else if (!file_exists(tokens[0])) {
            fprintf(stderr, "Error: DLL file '%s' does not exist or cannot be opened.\n",
                    tokens[0]);
        } else if (tokens[1][0] == '\0') {
            fprintf(stderr, "Error: function name must not be empty.\n");
        } else if (!parse_output_type(tokens[2], &out->output_type)) {
            fprintf(stderr, "Error: invalid output type '%s'.\n", tokens[2]);
            fprintf(stderr, "Valid output types: int64, uint64, float, double, mpz, mpfr.\n");
        } else {
            snprintf(out->path, sizeof(out->path), "%s", tokens[0]);
            snprintf(out->function_name, sizeof(out->function_name), "%s", tokens[1]);
            return 0;
        }

        fprintf(stderr, "Attempt %d of %d failed.\n", attempt, MAX_INPUT_ATTEMPTS);
    }

    return -1;
}

//copy the given range of the text and print out as a result
static char *copy_text_range(const char *start, size_t length)
{
    char *copy = malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, start, length);
    copy[length] = '\0';
    return copy;
}

//store the entire content of the file into a char list for later process
static char *read_entire_file(const char *path)
{
    FILE *file;
    long size;
    char *content;
    size_t bytes_read;

    file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }
    rewind(file);

    content = malloc((size_t)size + 1);
    if (content == NULL) {
        fclose(file);
        return NULL;
    }

    bytes_read = fread(content, 1, (size_t)size, file);
    fclose(file);
    content[bytes_read] = '\0';
    return content;
}

//load the text into setup text and template text(the last line of the file)
static int load_spiral_txt(const char *path, spiral_target *target)
{
    char *content = read_entire_file(path);
    size_t length;
    size_t template_start;
    size_t setup_length;

    if (content == NULL) {
        fprintf(stderr, "Error: failed to read SPIRAL txt file '%s'.\n", path);
        return -1;
    }

    length = strlen(content);
    while (length > 0 && (content[length - 1] == '\n' || content[length - 1] == '\r')) {
        content[--length] = '\0';
    }

    if (length == 0) {
        fprintf(stderr, "Error: SPIRAL txt file must not be empty.\n");
        free(content);
        return -1;
    }

    template_start = length;
    while (template_start > 0 &&
           content[template_start - 1] != '\n' &&
           content[template_start - 1] != '\r') {
        template_start--;
    }

    setup_length = template_start;
    while (setup_length > 0 &&
           (content[setup_length - 1] == '\n' || content[setup_length - 1] == '\r')) {
        setup_length--;
    }

    target->setup_text = copy_text_range(content, setup_length);
    target->call_template = copy_text_range(content + template_start, length - template_start);
    free(content);

    if (target->setup_text == NULL || target->call_template == NULL) {
        fprintf(stderr, "Error: failed to allocate SPIRAL txt content.\n");
        free(target->setup_text);
        free(target->call_template);
        target->setup_text = NULL;
        target->call_template = NULL;
        return -1;
    }
    if (target->call_template[0] == '\0') {
        fprintf(stderr, "Error: SPIRAL call template must not be empty.\n");
        return -1;
    }

    return 0;
}

//function that reads the inputs of spiral info and parse the result to spiral_target if legal
static int read_spiral_info(spiral_target *target)
{
    char line[INPUT_BUFFER_SIZE];
    char *tokens[2];

    memset(target, 0, sizeof(*target));
#ifndef _WIN32
    target->pid = -1;
    target->stdin_fd = -1;
    target->stdout_fd = -1;
#endif

    for (int attempt = 1; attempt <= MAX_INPUT_ATTEMPTS; attempt++) {
        read_line_status status = read_line(
            "Please enter SPIRAL executable and txt path: ",
            line,
            sizeof(line));
        if (status == READ_LINE_TOO_LONG) {
            fprintf(stderr, "Error: input line is too long.\n");
            return -1;
        }
        if (status == READ_LINE_EOF) {
            fprintf(stderr, "Error: failed to read input.\n");
            return -1;
        }

        size_t token_count = split_tokens(line, tokens, 2);
        if (token_count < 2) {
            fprintf(stderr,
                    "Error: too few inputs; enter a SPIRAL executable and txt path.\n");
        } else if (token_count > 2) {
            fprintf(stderr,
                    "Error: too many inputs; enter exactly a SPIRAL executable and txt path.\n");
        } else if (!file_exists(tokens[1])) {
            fprintf(stderr, "Error: SPIRAL txt file '%s' does not exist or cannot be opened.\n",
                    tokens[1]);
        } else {
            snprintf(target->executable, sizeof(target->executable), "%s", tokens[0]);
            snprintf(target->txt_path, sizeof(target->txt_path), "%s", tokens[1]);
            return load_spiral_txt(tokens[1], target);
        }

        fprintf(stderr, "Attempt %d of %d failed.\n", attempt, MAX_INPUT_ATTEMPTS);
    }

    return -1;
}

//clear the memory occupied by the structure target
static void clear_spiral_target(spiral_target *target)
{
    if (target == NULL) {
        return;
    }
    free(target->setup_text);
    free(target->call_template);
    target->setup_text = NULL;
    target->call_template = NULL;
}

//parse a string to uint32_t, return true if successful
static bool parse_u32(const char *text, uint32_t *out)
{
    char *end = NULL;
    unsigned long value;

    if (text == NULL || *text == '\0') {
        return false;
    }

    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value > UINT32_MAX) {
        return false;
    }

    *out = (uint32_t)value;
    return true;
}

//parse a string to size_t, return true if successful
static bool parse_size(const char *text, size_t *out)
{
    char *end = NULL;
    unsigned long long value;

    if (text == NULL || *text == '\0') {
        return false;
    }

    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value > SIZE_MAX) {
        return false;
    }

    *out = (size_t)value;
    return true;
}

//find the sampling_spec that corresponds to the name
static const sampling_spec *find_sampling_spec(const char *name)
{
    size_t num_specs = sizeof(sampling_specs) / sizeof(sampling_specs[0]);

    for (size_t i = 0; i < num_specs; i++) {
        if (strcmp(name, sampling_specs[i].name) == 0) {
            return &sampling_specs[i];
        }
    }

    return NULL;
}

//read and decide if the sigma value is valid
static int read_sigma(mpfr_t sigma)
{
    char line[INPUT_BUFFER_SIZE];
    char *tokens[1];

    for (int attempt = 1; attempt <= MAX_INPUT_ATTEMPTS; attempt++) {
        read_line_status status =
            read_line("Please enter sigma value: ", line, sizeof(line));
        if (status == READ_LINE_TOO_LONG) {
            fprintf(stderr, "Error: input line is too long.\n");
            return -1;
        }
        if (status == READ_LINE_EOF) {
            fprintf(stderr, "Error: failed to read input.\n");
            return -1;
        }

        size_t token_count = split_tokens(line, tokens, 1);
        if (token_count < 1) {
            fprintf(stderr, "Error: too few inputs; enter one sigma value.\n");
        } else if (token_count > 1) {
            fprintf(stderr, "Error: too many inputs; enter only one sigma value.\n");
        } else if (mpfr_set_str(sigma, tokens[0], 10, MPFR_RNDN) != 0 ||
                   !mpfr_number_p(sigma)) {
            fprintf(stderr, "Error: sigma must be a finite numeric value.\n");
        } else if (mpfr_sgn(sigma) < 0) {
            fprintf(stderr, "Error: sigma must not be negative.\n");
        } else {
            return 0;
        }

        fprintf(stderr, "Attempt %d of %d failed.\n", attempt, MAX_INPUT_ATTEMPTS);
    }

    return -1;
}

//read and decide if the amount of sample is valid
static int read_sampling_count(size_t *out)
{
    char line[INPUT_BUFFER_SIZE];
    char *tokens[1];

    for (int attempt = 1; attempt <= MAX_INPUT_ATTEMPTS; attempt++) {
        read_line_status status =
            read_line("Please enter the number of sampling arrays: ", line, sizeof(line));
        if (status == READ_LINE_TOO_LONG) {
            fprintf(stderr, "Error: input line is too long.\n");
            return -1;
        }
        if (status == READ_LINE_EOF) {
            fprintf(stderr, "Error: failed to read input.\n");
            return -1;
        }

        size_t token_count = split_tokens(line, tokens, 1);
        if (token_count < 1) {
            fprintf(stderr, "Error: too few inputs; enter one sampling count.\n");
        } else if (token_count > 1) {
            fprintf(stderr, "Error: too many inputs; enter only one sampling count.\n");
        } else if (!parse_size(tokens[0], out)) {
            fprintf(stderr, "Error: sampling count must be a non-negative integer.\n");
        } else if (*out == 0 || *out > MAX_SAMPLING_ARRAYS) {
            fprintf(stderr, "Error: sampling count must be between 1 and %d.\n",
                    MAX_SAMPLING_ARRAYS);
        } else {
            return 0;
        }

        fprintf(stderr, "Attempt %d of %d failed.\n", attempt, MAX_INPUT_ATTEMPTS);
    }

    return -1;
}

//read sample and put the sample into sampling array if valid input
static int read_sampling(size_t index, size_t total, sample_array_list *sal)
{
    char line[INPUT_BUFFER_SIZE];
    char prompt[96];
    char *tokens[MAX_INPUT_TOKENS];

    snprintf(prompt, sizeof(prompt),
             "Please enter sampling %zu of %zu: ", index + 1, total);

    for (int attempt = 1; attempt <= MAX_INPUT_ATTEMPTS; attempt++) {
        read_line_status status = read_line(prompt, line, sizeof(line));
        if (status == READ_LINE_TOO_LONG) {
            fprintf(stderr, "Error: input line is too long.\n");
            return -1;
        }
        if (status == READ_LINE_EOF) {
            fprintf(stderr, "Error: failed to read input.\n");
            return -1;
        }

        size_t token_count = split_tokens(line, tokens, MAX_INPUT_TOKENS);
        if (token_count == 0) {
            fprintf(stderr, "Error: too few inputs; enter a sampling definition.\n");
        } else if (token_count > MAX_INPUT_TOKENS) {
            fprintf(stderr, "Error: too many sampling inputs.\n");
        } else {
            const sampling_spec *spec = find_sampling_spec(tokens[0]);
            if (spec == NULL) {
                fprintf(stderr, "Error: unknown sampling type '%s'.\n", tokens[0]);
            } else if (token_count < spec->arg_count) {
                fprintf(stderr,
                        "Error: too few inputs for '%s'; expected %zu but received %zu.\n",
                        spec->name, spec->arg_count, token_count);
            } else if (token_count > spec->arg_count) {
                fprintf(stderr,
                        "Error: too many inputs for '%s'; expected %zu but received %zu.\n",
                        spec->name, spec->arg_count, token_count);
            } else if (spec->parse((int)token_count, tokens, sal) >= 0) {
                return 0;
            }
        }

        fprintf(stderr, "Attempt %d of %d failed.\n", attempt, MAX_INPUT_ATTEMPTS);
    }

    return -1;
}

//parse the unsigned input string into sample_array_list
static int parse_simplified_uint(int argc, char **argv, sample_array_list *sal)
{
    uint32_t precision;
    size_t num_samples;
    void *array;

    if (argc < 3) {
        fprintf(stderr, "Error: uint requires precision and num_samples.\n");
        return -1;
    }

    if (!parse_u32(argv[1], &precision)) {
        fprintf(stderr, "Error: invalid precision '%s' for uint.\n", argv[1]);
        return -1;
    }

    if (precision == 0 || precision > 64) {
        fprintf(stderr, "Error: uint precision must be between 1 and 64.\n");
        return -1;
    }

    if (!parse_size(argv[2], &num_samples)) {
        fprintf(stderr, "Error: invalid num_samples '%s' for uint.\n", argv[2]);
        return -1;
    }

    if (num_samples == 0) {
        fprintf(stderr, "Error: uint num_samples must be greater than 0.\n");
        return -1;
    }

    array = tiling_int_simplified(num_samples, precision, false);
    if (array == NULL) {
        return -1;
    }

    add_to_sample_array(sal, sizeof(uint64_t), num_samples, array, CMP_U64);
    return 3;
}

//function that parse a simplified signed int from input
static int parse_simplified_int(int argc, char **argv, sample_array_list *sal)
{
    uint32_t precision;
    size_t num_samples;
    void *array;

    if (argc < 3) {
        fprintf(stderr, "Error: int requires precision and num_samples.\n");
        return -1;
    }

    if (!parse_u32(argv[1], &precision)) {
        fprintf(stderr, "Error: invalid precision '%s' for int.\n", argv[1]);
        return -1;
    }

    if (precision == 0 || precision > 64) {
        fprintf(stderr, "Error: int precision must be between 1 and 64.\n");
        return -1;
    }

    if (!parse_size(argv[2], &num_samples)) {
        fprintf(stderr, "Error: invalid num_samples '%s' for int.\n", argv[2]);
        return -1;
    }

    if (num_samples == 0) {
        fprintf(stderr, "Error: int num_samples must be greater than 0.\n");
        return -1;
    }

    array = tiling_int_simplified(num_samples, precision, true);
    if (array == NULL) {
        return -1;
    }

    add_to_sample_array(sal, sizeof(int64_t), num_samples, array, CMP_I64);
    return 3;
}

//function that parse a simplified float from input
static int parse_simplified_float(int argc, char **argv, sample_array_list *sal)
{
    uint32_t precision;
    size_t num_samples;
    size_t elem_size;
    void *array;

    if (argc < 3) {
        fprintf(stderr, "Error: float requires precision and num_samples.\n");
        return -1;
    }

    if (!parse_u32(argv[1], &precision)) {
        fprintf(stderr, "Error: invalid precision '%s' for float.\n", argv[1]);
        return -1;
    }

    if (precision != 32 && precision != 64) {
        fprintf(stderr, "Error: float precision must be 32 or 64.\n");
        return -1;
    }

    if (!parse_size(argv[2], &num_samples)) {
        fprintf(stderr, "Error: invalid num_samples '%s' for float.\n", argv[2]);
        return -1;
    }

    if (num_samples == 0) {
        fprintf(stderr, "Error: float num_samples must be greater than 0.\n");
        return -1;
    }

    array = tiling_float_simplified(num_samples, precision);
    if (array == NULL) {
        return -1;
    }

    elem_size = precision == 32 ? sizeof(float) : sizeof(double);
    add_to_sample_array(sal, elem_size, num_samples, array, CMP_DOUBLE);
    return 3;
}

//a common function that prase mpz array out of input
static int parse_mpz_common(int argc, char **argv, sample_array_list *sal, bool sign, const char *name)
{
    uint32_t precision;
    size_t num_samples;
    mpz_t *array = NULL;

    if (argc < 3) {
        fprintf(stderr, "Error: %s requires precision and num_samples.\n", name);
        return -1;
    }

    if (!parse_u32(argv[1], &precision)) {
        fprintf(stderr, "Error: invalid precision '%s' for %s.\n", argv[1], name);
        return -1;
    }

    if (precision == 0 || precision > 256) {
        fprintf(stderr, "Error: %s precision must be between 1 and 256.\n", name);
        return -1;
    }

    if (!parse_size(argv[2], &num_samples)) {
        fprintf(stderr, "Error: invalid num_samples '%s' for %s.\n", argv[2], name);
        return -1;
    }

    if (num_samples == 0) {
        fprintf(stderr, "Error: %s num_samples must be greater than 0.\n", name);
        return -1;
    }

    tiling_int(&array, num_samples, precision, sign);
    if (array == NULL) {
        return -1;
    }

    add_to_sample_array(sal, sizeof(mpz_t), num_samples, array, CMP_MPZ);
    return 3;
}

//function that takes in input of unsigned large int type
static int parse_umpz(int argc, char **argv, sample_array_list *sal)
{
    return parse_mpz_common(argc, argv, sal, false, "umpz");
}

//function that takes in input of signed large int type
static int parse_mpz(int argc, char **argv, sample_array_list *sal)
{
    return parse_mpz_common(argc, argv, sal, true, "mpz");
}

//parse the rounding type
static bool parse_rounding(const char *text, rnd_type *out)
{
    if (strcmp(text, "RNDN") == 0) {
        *out = RNDN;
        return true;
    }
    if (strcmp(text, "RNDZ") == 0) {
        *out = RNDZ;
        return true;
    }
    if (strcmp(text, "RNDU") == 0) {
        *out = RNDU;
        return true;
    }
    if (strcmp(text, "RNDD") == 0) {
        *out = RNDD;
        return true;
    }
    if (strcmp(text, "RNDA") == 0) {
        *out = RNDA;
        return true;
    }

    return false;
}

//parse the mpfr function using the parameter given
static int parse_mpfr(int argc, char **argv, sample_array_list *sal)
{
    size_t precision_value;
    size_t mantissa_value;
    size_t num_samples;
    mpfr_prec_t precision;
    mpfr_prec_t mantissa;
    rnd_type round;
    mpfr_t *array = NULL;

    if (argc < 5) {
        fprintf(stderr, "Error: mpfr requires precision, mantissa, num_samples, and rounding.\n");
        return -1;
    }

    if (!parse_size(argv[1], &precision_value)) {
        fprintf(stderr, "Error: invalid precision '%s' for mpfr.\n", argv[1]);
        return -1;
    }

    if (precision_value == 0 || precision_value > 256) {
        fprintf(stderr, "Error: mpfr precision must be between 1 and 256.\n");
        return -1;
    }

    if (!parse_size(argv[2], &mantissa_value)) {
        fprintf(stderr, "Error: invalid mantissa '%s' for mpfr.\n", argv[2]);
        return -1;
    }

    if (precision_value <= 2 || mantissa_value == 0 || mantissa_value >= precision_value - 1) {
        fprintf(stderr, "Error: mpfr mantissa must be greater than 0 and less than precision - 1.\n");
        return -1;
    }

    if (!parse_size(argv[3], &num_samples)) {
        fprintf(stderr, "Error: invalid num_samples '%s' for mpfr.\n", argv[3]);
        return -1;
    }

    if (num_samples == 0) {
        fprintf(stderr, "Error: mpfr num_samples must be greater than 0.\n");
        return -1;
    }

    if (!parse_rounding(argv[4], &round)) {
        fprintf(stderr, "Error: invalid rounding mode '%s' for mpfr.\n", argv[4]);
        fprintf(stderr, "Valid rounding modes: RNDN, RNDZ, RNDU, RNDD, RNDA.\n");
        return -1;
    }

    precision = (mpfr_prec_t)precision_value;
    mantissa = (mpfr_prec_t)mantissa_value;
    tiling_float(&array, num_samples, precision, mantissa, round);
    if (array == NULL) {
        return -1;
    }

    add_to_sample_array(sal, sizeof(mpfr_t), num_samples, array, CMP_MPFR);
    return 5;
}

//load function from the DLL
static bool load_function(const function_info *info, loaded_function *out)
{
#ifdef _WIN32
    FARPROC symbol;

    out->handle = LoadLibraryA(info->path);
    if (out->handle == NULL) {
        fprintf(stderr, "Error: failed to load DLL '%s' (Windows error %lu).\n",
                info->path, (unsigned long)GetLastError());
        return false;
    }

    symbol = GetProcAddress(out->handle, info->function_name);
    if (symbol == NULL) {
        fprintf(stderr, "Error: function '%s' was not found in DLL '%s'.\n",
                info->function_name, info->path);
        FreeLibrary(out->handle);
        out->handle = NULL;
        return false;
    }

    memcpy(&out->function, &symbol, sizeof(out->function));
#else
    void *symbol;
    const char *error;

    out->handle = dlopen(info->path, RTLD_NOW);
    if (out->handle == NULL) {
        fprintf(stderr, "Error: failed to load library '%s': %s\n",
                info->path, dlerror());
        return false;
    }

    dlerror();
    symbol = dlsym(out->handle, info->function_name);
    error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Error: function '%s' was not found in library '%s': %s\n",
                info->function_name, info->path, error);
        dlclose(out->handle);
        out->handle = NULL;
        return false;
    }

    memcpy(&out->function, &symbol, sizeof(out->function));
#endif

    return true;
}

//unload function to release memory
static void unload_function(loaded_function *loaded)
{
    if (loaded->handle == NULL) {
        return;
    }

#ifdef _WIN32
    FreeLibrary(loaded->handle);
#else
    dlclose(loaded->handle);
#endif
    loaded->handle = NULL;
    loaded->function = NULL;
}

#ifndef _WIN32

//helper to append the entire string to a given string
static int append_text(char *destination, size_t capacity, size_t *used, const char *text)
{
    size_t length = strlen(text);
    if (*used > capacity || length >= capacity - *used) {
        return -1;
    }

    memcpy(destination + *used, text, length);
    *used += length;
    destination[*used] = '\0';
    return 0;
}

//helper function to append a single char after a given string
static int append_char(char *destination, size_t capacity, size_t *used, char ch)
{
    if (*used + 1 >= capacity) {
        return -1;
    }

    destination[(*used)++] = ch;
    destination[*used] = '\0';
    return 0;
}

//decide if the content is decimal number
static bool is_decimal_digit(char ch)
{
    return ch >= '0' && ch <= '9';
}

//determine of the the char belongs to one of the following format string specification
static bool is_printf_conversion_char(char ch)
{
    return strchr("diuoxXfFeEgGaAcsp", ch) != NULL;
}

//process all kinds of special format specifier
static const char *printf_specifier_end(const char *percent)
{
    const char *cursor = percent + 1;

    if (*cursor == '\0') {
        return NULL;
    }
    if (*cursor == '%') {
        return cursor + 1;
    }

    while (strchr("-+ #0", *cursor) != NULL) {
        cursor++;
    }

    if (*cursor == '*') {
        cursor++;
    } else {
        while (is_decimal_digit(*cursor)) {
            cursor++;
        }
    }

    if (*cursor == '.') {
        cursor++;
        if (*cursor == '*') {
            cursor++;
        } else {
            while (is_decimal_digit(*cursor)) {
                cursor++;
            }
        }
    }

    if (cursor[0] == 'h' && cursor[1] == 'h') {
        cursor += 2;
    } else if (cursor[0] == 'l' && cursor[1] == 'l') {
        cursor += 2;
    } else if (strchr("hljztL", *cursor) != NULL) {
        cursor++;
    }

    if (!is_printf_conversion_char(*cursor)) {
        return NULL;
    }

    return cursor + 1;
}

//helper that turn all the format specifier into actual samples
static int build_spiral_command(
    const spiral_target *target,
    const combination_array *combinations,
    combination inputs,
    size_t num_inputs,
    char *command,
    size_t command_capacity)
{
    char values[MAX_SAMPLING_ARRAYS][SPIRAL_VALUE_BUFFER_SIZE];
    size_t used = 0;
    size_t value_index = 0;
    const char *cursor;

    if (target == NULL || target->call_template == NULL ||
        command == NULL || command_capacity == 0 ||
        num_inputs > MAX_SAMPLING_ARRAYS) {
        return -1;
    }

    //turn all the outputs from void * to string format
    for (size_t i = 0; i < num_inputs; i++) {
        if (format_combination_value(
                combinations,
                i,
                inputs[i],
                values[i],
                sizeof(values[i])) != 0) {
            return -1;
        }
    }

    //Go through every char; replace each printf-style conversion with the next sample.
    command[0] = '\0';
    cursor = target->call_template;
    while (*cursor != '\0') {
        if (*cursor == '%') {
            if (cursor[1] == '%') {
                if (append_char(command, command_capacity, &used, '%') != 0) {
                    return -1;
                }
                cursor += 2;
                continue;
            }

            const char *specifier_end = printf_specifier_end(cursor);
            if (specifier_end == NULL) {
                fprintf(stderr,
                        "Error: SPIRAL template has an unsupported or incomplete printf placeholder.\n");
                return -1;
            }
            if (value_index >= num_inputs) {
                fprintf(stderr,
                        "Error: SPIRAL template has more printf placeholders than inputs.\n");
                return -1;
            }
            if (append_text(command, command_capacity, &used, values[value_index]) != 0) {
                return -1;
            }
            value_index++;
            cursor = specifier_end;
            continue;
        }

        if (append_char(command, command_capacity, &used, *cursor) != 0) {
            return -1;
        }
        cursor++;
    }

    if (value_index < num_inputs) {
        fprintf(stderr,
                "Error: SPIRAL template has fewer printf placeholders than inputs.\n");
        return -1;
    }

    if (used == 0 || command[used - 1] != '\n') {
        if (append_char(command, command_capacity, &used, '\n') != 0) {
            return -1;
        }
    }

    return 0;
}

//get the result and turn the result into mpfr for comparison
static bool parse_mpfr_from_line(mpfr_t out, const char *line)
{
    char copy[SPIRAL_LINE_BUFFER_SIZE];
    char *token;

    snprintf(copy, sizeof(copy), "%s", line);
    token = strtok(copy, " \t\r\n,;:");
    while (token != NULL) {
        if (mpfr_set_str(out, token, 10, MPFR_RNDN) == 0 &&
            mpfr_number_p(out)) {
            return true;
        }
        token = strtok(NULL, " \t\r\n,;:");
    }

    return false;
}
#endif

#ifdef _WIN32//---------------------------spiral unavaliable for windows--------------------------//
static bool start_spiral(spiral_target *target)
{
    (void)target;
    fprintf(stderr, "Error: SPIRAL mode currently requires POSIX fork/pipe; "
                    "this Windows build only supports DLL-vs-DLL mode.\n");
    return false;
}

static void stop_spiral(spiral_target *target)
{
    (void)target;
}

static bool run_spiral_sample(
    spiral_target *target,
    const combination_array *combinations,
    combination inputs,
    size_t num_inputs,
    mpfr_t output)
{
    (void)target;
    (void)combinations;
    (void)inputs;
    (void)num_inputs;
    (void)output;
    return false;
}
#else //---------------------linux special feature for spiral comparison--------------------------//
//write the content of "text" as input to spiral
static bool write_all_to_fd(int fd, const char *text)
{
    size_t length = strlen(text);
    size_t written_total = 0;

    while (written_total < length) {
        ssize_t written = write(fd, text + written_total, length - written_total);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        written_total += (size_t)written;
    }

    return true;
}

//start a child process and replace it with spiral, and then pipe it and use dup to connect the result channel
static bool start_spiral(spiral_target *target)
{
    int to_spiral[2];
    int from_spiral[2];

    if (pipe(to_spiral) != 0) {
        perror("pipe");
        return false;
    }
    if (pipe(from_spiral) != 0) {
        perror("pipe");
        close(to_spiral[0]);
        close(to_spiral[1]);
        return false;
    }

    target->pid = fork();
    if (target->pid < 0) {
        perror("fork");
        close(to_spiral[0]);
        close(to_spiral[1]);
        close(from_spiral[0]);
        close(from_spiral[1]);
        return false;
    }

    if (target->pid == 0) {
        dup2(to_spiral[0], STDIN_FILENO);
        dup2(from_spiral[1], STDOUT_FILENO);
        dup2(from_spiral[1], STDERR_FILENO);

        close(to_spiral[0]);
        close(to_spiral[1]);
        close(from_spiral[0]);
        close(from_spiral[1]);

        execlp(target->executable, target->executable, (char *)NULL);
        perror("execlp");
        _exit(127);
    }

    close(to_spiral[0]);
    close(from_spiral[1]);
    target->stdin_fd = to_spiral[1];
    target->stdout_fd = from_spiral[0];

    if (target->setup_text != NULL && target->setup_text[0] != '\0') {
        if (!write_all_to_fd(target->stdin_fd, target->setup_text) ||
            !write_all_to_fd(target->stdin_fd, "\n")) {
            fprintf(stderr, "Error: failed to write setup text to SPIRAL.\n");
            return false;
        }
    }

    return true;
}

//close all the files and channels that are in use
static void stop_spiral(spiral_target *target)
{
    int status;

    if (target == NULL) {
        return;
    }
    if (target->stdin_fd >= 0) {
        close(target->stdin_fd);
        target->stdin_fd = -1;
    }
    if (target->stdout_fd >= 0) {
        close(target->stdout_fd);
        target->stdout_fd = -1;
    }
    if (target->pid > 0) {
        waitpid(target->pid, &status, 0);
        target->pid = -1;
    }
}

//try to read the output and return exit code as noncoero if the execution is not successful
static int read_char_with_timeout(int fd, char *out)
{
    fd_set read_fds;
    struct timeval timeout;
    ssize_t bytes_read;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        timeout.tv_sec = SPIRAL_READ_TIMEOUT_SECONDS;
        timeout.tv_usec = 0;

        int ready = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (ready == 0) {
            return 0;
        }

        bytes_read = read(fd, out, 1);
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (bytes_read == 0) {
            return -1;
        }
        return 1;
    }
}

//get the result from one input to spiral
static bool read_spiral_numeric_result(spiral_target *target, mpfr_t output)
{
    char line[SPIRAL_LINE_BUFFER_SIZE];
    size_t length = 0;

    while (true) {
        char ch;
        int status = read_char_with_timeout(target->stdout_fd, &ch);
        if (status < 0) {
            fprintf(stderr, "Error: failed to read SPIRAL output.\n");
            return false;
        }
        if (status == 0) {
            fprintf(stderr, "Error: timed out while waiting for SPIRAL output.\n");
            return false;
        }

        if (ch == '\n') {
            line[length] = '\0';
            if (parse_mpfr_from_line(output, line)) {
                return true;
            }
            length = 0;
        } else if (length + 1 < sizeof(line)) {
            line[length++] = ch;
        } else {
            length = 0;
        }
    }
}

//write a single set of samples to spiral
static bool run_spiral_sample(
    spiral_target *target,
    const combination_array *combinations,
    combination inputs,
    size_t num_inputs,
    mpfr_t output)
{
    char command[SPIRAL_COMMAND_BUFFER_SIZE];

    if (build_spiral_command(
            target,
            combinations,
            inputs,
            num_inputs,
            command,
            sizeof(command)) != 0) {
        fprintf(stderr, "Error: failed to build SPIRAL command.\n");
        return false;
    }

    if (!write_all_to_fd(target->stdin_fd, command)) {
        fprintf(stderr, "Error: failed to write command to SPIRAL.\n");
        return false;
    }

    return read_spiral_numeric_result(target, output);
}
#endif

//initialize function_result
static void function_result_init(function_result *result, user_output_type type)
{
    result->type = type;
    memset(&result->native, 0, sizeof(result->native));

    if (type == USER_OUTPUT_MPZ) {
        mpz_init(result->mpz);
    } else if (type == USER_OUTPUT_MPFR) {
        mpfr_init2(result->mpfr, 256);
    }
}

//reset the variable of type function_result
static void function_result_reset(function_result *result)
{
    memset(&result->native, 0, sizeof(result->native));

    if (result->type == USER_OUTPUT_MPZ) {
        mpz_set_ui(result->mpz, 0);
    } else if (result->type == USER_OUTPUT_MPFR) {
        mpfr_set_zero(result->mpfr, 0);
    }
}

//clear the function result to release memory
static void function_result_clear(function_result *result)
{
    if (result->type == USER_OUTPUT_MPZ) {
        mpz_clear(result->mpz);
    } else if (result->type == USER_OUTPUT_MPFR) {
        mpfr_clear(result->mpfr);
    }
}

//return the result
static void *function_result_output(function_result *result)
{
    switch (result->type) {
        case USER_OUTPUT_I64:
            return &result->native.i64;
        case USER_OUTPUT_U64:
            return &result->native.u64;
        case USER_OUTPUT_FLOAT:
            return &result->native.f32;
        case USER_OUTPUT_DOUBLE:
            return &result->native.f64;
        case USER_OUTPUT_MPZ:
            return result->mpz;
        case USER_OUTPUT_MPFR:
            return result->mpfr;
    }

    return NULL;
}

//turn all the result to mpfr for comparison purpose
static void function_result_to_mpfr(mpfr_t destination, const function_result *result)
{
    switch (result->type) {
        case USER_OUTPUT_I64:
            mpfr_set_sj(destination, (intmax_t)result->native.i64, MPFR_RNDN);
            break;
        case USER_OUTPUT_U64:
            mpfr_set_uj(destination, (uintmax_t)result->native.u64, MPFR_RNDN);
            break;
        case USER_OUTPUT_FLOAT:
            mpfr_set_flt(destination, result->native.f32, MPFR_RNDN);
            break;
        case USER_OUTPUT_DOUBLE:
            mpfr_set_d(destination, result->native.f64, MPFR_RNDN);
            break;
        case USER_OUTPUT_MPZ:
            mpfr_set_z(destination, result->mpz, MPFR_RNDN);
            break;
        case USER_OUTPUT_MPFR:
            mpfr_set(destination, result->mpfr, MPFR_RNDN);
            break;
    }
}

//determine if the samples that is within the sigma value
static bool results_within_sigma(
    const function_result *first,
    const function_result *second,
    mpfr_srcptr sigma,
    bool *absolute_within,
    bool *relative_within)
{
    mpfr_t first_value;
    mpfr_t second_value;
    mpfr_t difference;
    mpfr_t first_abs;
    mpfr_t second_abs;
    mpfr_t scale;
    mpfr_t threshold;

    mpfr_init2(first_value, 512);
    mpfr_init2(second_value, 512);
    mpfr_init2(difference, 512);
    mpfr_init2(first_abs, 512);
    mpfr_init2(second_abs, 512);
    mpfr_init2(scale, 512);
    mpfr_init2(threshold, 512);

    *absolute_within = false;
    *relative_within = false;

    function_result_to_mpfr(first_value, first);
    function_result_to_mpfr(second_value, second);

    if (mpfr_nan_p(first_value) || mpfr_nan_p(second_value)) {
        *absolute_within = false;
        *relative_within = false;
    } else if (mpfr_equal_p(first_value, second_value)) {
        *absolute_within = true;
        *relative_within = true;
    } else if (!mpfr_number_p(first_value) || !mpfr_number_p(second_value)) {
        *absolute_within = false;
        *relative_within = false;
    } else {
        mpfr_sub(difference, first_value, second_value, MPFR_RNDN);
        mpfr_abs(difference, difference, MPFR_RNDN);
        *absolute_within = !mpfr_nan_p(difference) &&
                           mpfr_cmp(difference, sigma) <= 0;

        mpfr_abs(first_abs, first_value, MPFR_RNDN);
        mpfr_abs(second_abs, second_value, MPFR_RNDN);
        mpfr_set_ui(scale, 1, MPFR_RNDN);
        if (mpfr_cmp(first_abs, scale) > 0) {
            mpfr_set(scale, first_abs, MPFR_RNDN);
        }
        if (mpfr_cmp(second_abs, scale) > 0) {
            mpfr_set(scale, second_abs, MPFR_RNDN);
        }

        mpfr_mul(threshold, sigma, scale, MPFR_RNDN);
        *relative_within = !mpfr_nan_p(difference) &&
                           !mpfr_nan_p(threshold) &&
                           mpfr_cmp(difference, threshold) <= 0;
    }

    mpfr_clear(threshold);
    mpfr_clear(scale);
    mpfr_clear(second_abs);
    mpfr_clear(first_abs);
    mpfr_clear(first_value);
    mpfr_clear(second_value);
    mpfr_clear(difference);
    return *absolute_within || *relative_within;
}

//get each combination of samples, put into functions, get results, turn types, compare
static int compare_all_combinations(
    combination_array *combinations,
    size_t num_inputs,
    const loaded_function *first_function,
    user_output_type first_type,
    const loaded_function *second_function,
    user_output_type second_type,
    mpfr_srcptr sigma,
    mpz_t absolute_within_count,
    mpz_t relative_within_count)
{
    mpz_srcptr total = combination_array_num_comb(combinations);
    mpz_t index;
    function_result first_result;
    function_result second_result;
    int status = -1;

    if (total == NULL) {
        return -1;
    }

    mpz_set_ui(absolute_within_count, 0);
    mpz_set_ui(relative_within_count, 0);
    mpz_init_set_ui(index, 0);
    function_result_init(&first_result, first_type);
    function_result_init(&second_result, second_type);

    while (mpz_cmp(index, total) < 0) {
        combination inputs = get_the_ith_combination(combinations, index);
        if (inputs == NULL) {
            fprintf(stderr, "Error: failed to generate a combination.\n");
            goto cleanup;
        }

        function_result_reset(&first_result);
        function_result_reset(&second_result);

        int first_status = first_function->function(
            inputs, num_inputs, function_result_output(&first_result));
        int second_status = second_function->function(
            inputs, num_inputs, function_result_output(&second_result));

        if (first_status != 0 || second_status != 0) {
            gmp_fprintf(stderr,
                "Error: a target function failed at combination %Zd "
                "(first status %d, second status %d).\n",
                index, first_status, second_status);
            free_ith_combination(inputs);
            goto cleanup;
        }

        bool absolute_within;
        bool relative_within;
        results_within_sigma(
            &first_result,
            &second_result,
            sigma,
            &absolute_within,
            &relative_within);
        if (absolute_within) {
            mpz_add_ui(absolute_within_count, absolute_within_count, 1);
        }
        if (relative_within) {
            mpz_add_ui(relative_within_count, relative_within_count, 1);
        }

        free_ith_combination(inputs);
        mpz_add_ui(index, index, 1);
    }

    status = 0;

cleanup:
    function_result_clear(&first_result);
    function_result_clear(&second_result);
    mpz_clear(index);
    return status;
}

static int compare_all_combinations_with_spiral(
    combination_array *combinations,
    size_t num_inputs,
    const loaded_function *first_function,
    user_output_type first_type,
    spiral_target *spiral,
    mpfr_srcptr sigma,
    mpz_t absolute_within_count,
    mpz_t relative_within_count)
{
    mpz_srcptr total = combination_array_num_comb(combinations);
    mpz_t index;
    function_result first_result;
    function_result spiral_result;
    int status = -1;

    if (total == NULL) {
        return -1;
    }

    mpz_set_ui(absolute_within_count, 0);
    mpz_set_ui(relative_within_count, 0);
    mpz_init_set_ui(index, 0);
    function_result_init(&first_result, first_type);
    function_result_init(&spiral_result, USER_OUTPUT_MPFR);

    while (mpz_cmp(index, total) < 0) {
        combination inputs = get_the_ith_combination(combinations, index);
        if (inputs == NULL) {
            fprintf(stderr, "Error: failed to generate a combination.\n");
            goto cleanup;
        }

        function_result_reset(&first_result);
        function_result_reset(&spiral_result);

        int first_status = first_function->function(
            inputs, num_inputs, function_result_output(&first_result));
        if (first_status != 0) {
            gmp_fprintf(stderr,
                "Error: target function failed at combination %Zd "
                "(status %d).\n",
                index, first_status);
            free_ith_combination(inputs);
            goto cleanup;
        }

        //run the sample in spiral and get the result
        if (!run_spiral_sample(
                spiral,
                combinations,
                inputs,
                num_inputs,
                spiral_result.mpfr)) {
            gmp_fprintf(stderr,
                "Error: SPIRAL target failed at combination %Zd.\n",
                index);
            free_ith_combination(inputs);
            goto cleanup;
        }

        bool absolute_within;
        bool relative_within;
        results_within_sigma(
            &first_result,
            &spiral_result,
            sigma,
            &absolute_within,
            &relative_within);
        if (absolute_within) {
            mpz_add_ui(absolute_within_count, absolute_within_count, 1);
        }
        if (relative_within) {
            mpz_add_ui(relative_within_count, relative_within_count, 1);
        }

        free_ith_combination(inputs);
        mpz_add_ui(index, index, 1);
    }

    status = 0;

cleanup:
    function_result_clear(&first_result);
    function_result_clear(&spiral_result);
    mpz_clear(index);
    return status;
}

//Function that calculate and print out the amount and precentage of samples that stays within the provided sigma
static void print_one_comparison_result(const char *label, mpz_srcptr within_count, mpz_srcptr total)
{
    mpfr_t ratio;
    mpfr_t percentage;

    mpfr_init2(ratio, 256);
    mpfr_init2(percentage, 256);
    mpfr_set_z(ratio, within_count, MPFR_RNDN);
    mpfr_div_z(ratio, ratio, total, MPFR_RNDN);
    mpfr_mul_ui(percentage, ratio, 100, MPFR_RNDN);

    printf("%s\n", label);
    gmp_printf("  Within sigma:      %Zd\n", within_count);
    mpfr_printf("  Within ratio:      %.12Rf\n", ratio);
    mpfr_printf("  Within percentage: %.8Rf%%\n", percentage);

    mpfr_clear(ratio);
    mpfr_clear(percentage);
}

static void print_comparison_result(
    mpz_srcptr absolute_within_count,
    mpz_srcptr relative_within_count,
    mpz_srcptr total)
{
    gmp_printf("Total combinations: %Zd\n", total);
    print_one_comparison_result(
        "Absolute error result:",
        absolute_within_count,
        total);
    print_one_comparison_result(
        "Relative error result:",
        relative_within_count,
        total);
}

//the function that determines if user is going to use spiral as one end of the output
static bool parse_use_spiral(int argc, char **argv)
{
    if (argc == 1) {
        return false;
    }
    if (argc == 2 && strcmp(argv[1], "--spiral") == 0) {
        return true;
    }

    fprintf(stderr, "Usage: %s [--spiral]\n", argv[0]);
    fprintf(stderr, "  no flag   compare two dynamically loaded functions\n");
    fprintf(stderr, "  --spiral  compare one dynamically loaded function against SPIRAL\n");
    return false;
}

/* The main function that takes in arguments from user:
    in the format of compiled file name of compare.c

    the first dll file containing the function that needs to be compared, 
        a string of the name of the function in the dll that takes in the input according to the api
        a string represents the output type of the first function
           -specification for type: int, uint, float, double, mpfr, mpz.
    and the second compile function following another string represents output type of the second function
        a string of the name of the function in the dll that takes in the input according to the api
        a string represents the output type of the first function
           -specification for type: int, uint, float, double, mpfr, mpz.

    a string representing the sigma value(only numeric value allowed now)
    
    a series of strings representing each tiling function they want to use:
        -Simplified: 
            -unsigned int: uint precision(0<, <=64) num_of_samples 
            -signed int: int precision(0<, <=64) num_of_samples
            -float: float precision(32 or 64) num_of_samples
        -GMP,MPFR function:
            -unsigned int: umpz precision (0< <= 256) num_of_samples
            -signed int: mpz precision(0< <=256) num_of_samples
            -float: mpfr precision mantissa num_of_samples type_of_rounding（RNDN\RNDZ\RNDU\RNDD\RNDA）
*/
int main(int argc, char **argv)
{
    bool invalid_arguments = argc > 2 ||
                             (argc == 2 && strcmp(argv[1], "--spiral") != 0);
    bool use_spiral = parse_use_spiral(argc, argv);
    int exit_code = CMP_EXIT_PARSE;
    function_info first_function;
    function_info second_function;
    loaded_function first_loaded = {0};
    loaded_function second_loaded = {0};
    spiral_target spiral = {0};
    size_t sampling_count;
    sample_array_list *sal = NULL;
    combination_array *combinations = NULL;
    mpfr_t sigma;
    mpz_t absolute_within_count;
    mpz_t relative_within_count;
    mpfr_init2(sigma, 256);
    mpz_init(absolute_within_count);
    mpz_init(relative_within_count);
#ifndef _WIN32
    spiral.pid = -1;
    spiral.stdin_fd = -1;
    spiral.stdout_fd = -1;
#endif

    if (invalid_arguments) {
        exit_code = CMP_EXIT_USAGE;
        goto cleanup;
    }

    puts("Output types: int64, uint64, float, double, mpz, mpfr.");
    puts("DLL function signature: int name(void **inputs, size_t num_inputs, void *output);");
    if (use_spiral) {
        puts("SPIRAL mode enabled. The txt template uses printf-style placeholders for sampled inputs.");
    }

    if (read_function_info(
            "Please enter the first DLL path, function name, and output type: ",
            &first_function) != 0) {
        goto cleanup;
    }

    if (use_spiral) {
        if (read_spiral_info(&spiral) != 0) {
            goto cleanup;
        }
    } else {
        if (read_function_info(
                "Please enter the second DLL path, function name, and output type: ",
                &second_function) != 0) {
            goto cleanup;
        }
    }

    if (read_sigma(sigma) != 0) {
        goto cleanup;
    }

    if (read_sampling_count(&sampling_count) != 0) {
        goto cleanup;
    }

    sal = create_sample_array_list(sampling_count);
    if (sal == NULL) {
        exit_code = CMP_EXIT_RUNTIME;
        goto cleanup;
    }

    puts("Sampling formats:");
    puts("  uint <precision> <num_samples>");
    puts("  int <precision> <num_samples>");
    puts("  float <precision> <num_samples>");
    puts("  umpz <precision> <num_samples>");
    puts("  mpz <precision> <num_samples>");
    puts("  mpfr <precision> <mantissa> <num_samples> <rounding>");

    for (size_t i = 0; i < sampling_count; i++) {
        if (read_sampling(i, sampling_count, sal) != 0) {
            goto cleanup;
        }
    }

    printf("All inputs accepted.\n");
    printf("First target:  %s -> %s\n",
           first_function.path, first_function.function_name);
    if (use_spiral) {
        printf("Second target: SPIRAL %s using %s\n",
               spiral.executable, spiral.txt_path);
    } else {
        printf("Second target: %s -> %s\n",
               second_function.path, second_function.function_name);
    }

    combinations = combine_arrays(sal);
    if (combinations == NULL) {
        exit_code = CMP_EXIT_RUNTIME;
        goto cleanup;
    }

    if (!load_function(&first_function, &first_loaded)) {
        exit_code = CMP_EXIT_RUNTIME;
        goto cleanup;
    }

    if (use_spiral) {
        if (!start_spiral(&spiral)) {
            exit_code = CMP_EXIT_RUNTIME;
            goto cleanup;
        }
        if (compare_all_combinations_with_spiral(
                combinations,
                sampling_count,
                &first_loaded,
                first_function.output_type,
                &spiral,
                sigma,
                absolute_within_count,
                relative_within_count) != 0) {
            exit_code = CMP_EXIT_RUNTIME;
            goto cleanup;
        }
    } else {
        if (!load_function(&second_function, &second_loaded)) {
            exit_code = CMP_EXIT_RUNTIME;
            goto cleanup;
        }
        if (compare_all_combinations(
                combinations,
                sampling_count,
                &first_loaded,
                first_function.output_type,
                &second_loaded,
                second_function.output_type,
                sigma,
                absolute_within_count,
                relative_within_count) != 0) {
            exit_code = CMP_EXIT_RUNTIME;
            goto cleanup;
        }
    }

    print_comparison_result(
        absolute_within_count,
        relative_within_count,
        combination_array_num_comb(combinations));
    exit_code = CMP_EXIT_OK;

cleanup:
    stop_spiral(&spiral);
    unload_function(&second_loaded);
    unload_function(&first_loaded);
    if (combinations != NULL) {
        free_combination_array(combinations);
    }
    if (sal != NULL) {
        free_sample_array_list_content(sal);
    }
    mpz_clear(relative_within_count);
    mpz_clear(absolute_within_count);
    mpfr_clear(sigma);
    clear_spiral_target(&spiral);
    return exit_code;
}
