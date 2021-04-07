/*
 * This file contains compression and decompression functions written to
 * handle compression and decompression of 512 bit cache lines.
 * Authors: Jeffrey Luo, Shivani Prasad, Deanyone Su
 */

#include "pin.H"
#include <cmath>
#include <string>

/*
 * Compresses a 64 byte cache line into N single precision fp
 * Arguments are cache line being compressed and the single fp array that will
 * contain the output single fp from compressing the cache line.
 * PERFORMS ~67% worse than using double-floats (same space usage)
 */
VOID CacheSingleFPCompress(char *cache_line, float* fp_out, int N) {
    float fp_array[N];
    float this_fp;

    if (N > 16) {
        printf("Unsupported. Precision lost and consumes MORE space\n");
        assert(false);
    } else if (N == 16) { // no space savings
        INT32 this_int;
        for (int my_idx = 0; my_idx < N; ++my_idx) {
            memcpy(&this_int, cache_line + 4 * my_idx, 4);
            this_fp = static_cast<float>(this_int);
            fp_array[my_idx] = this_fp;

            printf("produced fp value: %f\n", fp_array[my_idx]);
        }
        memcpy(fp_out, &fp_array, N * sizeof(float));
    } else {
        printf("Unsupported\n");
        assert(false);
    }
}

/*
 * Decompresses N single precision fp into a 64 byte cache line
 * Arguments are the single fp array that is the compressed cache line and
 * a size 512 char array that contains the uncompressed cache line
 * PERFORMS ~67% worse than using double-floats (same space usage)
 */
VOID CacheSingleFPDecompress(float* fp_in, char *cache_line, int N) {
    float this_fp;

    if (N > 16) {
        printf("Unsupported. Precision lost and consumes MORE space\n");
        assert(false);
    } else if (N == 16) {
        INT32 this_int;
        for (int my_idx = 0; my_idx < N; ++my_idx) {
            this_fp = fp_in[my_idx];
            this_int = static_cast<INT32>(this_fp);
            printf("decompressed float value: %d\n", this_int);
            memcpy(cache_line + 4 * my_idx, &this_int, 4);
        }
    } else {
        printf("Unsupported\n");
        assert(false);
    }
}

/*
 * Compresses a 64 byte cache line into N double precision fp
 * Arguments are cache line being compressed and the double fp array that will
 * contain the output double fp from compressing the cache line.
 */
VOID CacheDoubleFPCompress(char *cache_line, double* fp_out, int N) {
    double fp_array[N];
    double this_fp;
    INT64 this_int;

    if (N > 8) {
        printf("Unsupported. Precision lost and consumes MORE space\n");
        assert(false);
    } else if (N == 8 || N == 4 || N == 2 || N == 1) {
        for (int my_idx = 0; my_idx < N; ++my_idx) {
            // parse cache line per 64 bits, convert to double, shift and sum
            for (int intra = 0; intra < 8/N; ++intra) {
                memcpy(&this_int, cache_line + 64/N * my_idx + intra * 8, 8);
                this_fp = static_cast<double>(this_int);
                if (intra == 0) {
                    fp_array[my_idx] = this_fp * pow(2.0, (intra * N * 16.0));
                } else {
                    fp_array[my_idx] += this_fp * pow(2.0, (intra * N * 16.0));
                }
            }
            // printf("produced double fp value: %.0f\n", fp_array[my_idx]);
        }
        memcpy(fp_out, &fp_array, N * sizeof(double));
    } else {
        printf("Unsupported\n");
        assert(false);
    }
}

int charIsOdd(char c) {
    if (c == '1' ||
        c == '3' ||
        c == '5' ||
        c == '7' ||
        c == '9') {

        return 1;
    } else {
        return 0;
    }
}

/*
 * Divides a string representation of a number by 2, rounds towards 0
 */
std::string stringDivTwo(std::string input) {
    std::string output;
    int additive = 0;

    for (size_t idx = 0; idx < input.length(); ++idx) {
        char digit = input[idx];
        if (digit == '-') {
            // do nothing, skip this value
        } else {
            int digit_val = (digit - '0') / 2 + additive;
            char new_char;
            _itoa(digit_val, &new_char, 10);
            output.append(1, new_char);
            additive = charIsOdd(digit) * 5;
        }
    }

    while (output.length() > 1 && output.front() == '0') {
        output.erase(0, 1); // remove leading 0s
    }

    return output;
}


/*
 * Decompresses N double precision fp into a 64 byte cache line
 * Arguments are the double fp array that is the compressed cache line and
 * a size 512 char array that contains the uncompressed cache line
 */
VOID CacheDoubleFPDecompress(double* fp_in, char *cache_line, int N) {
    double this_fp;

    if (N > 8) {
        printf("Unsupported. Precision lost and consumes MORE space\n");
        assert(false);
    } else if (N == 8) {
        INT64 this_int;
        for (int my_idx = 0; my_idx < N; ++my_idx) {
            this_fp = fp_in[my_idx];
            this_int = static_cast<INT64>(this_fp);
            // printf("decompressed float value: %zd\n", this_int);
            memcpy(cache_line + 8 * my_idx, &this_int, 8);
        }
    } else if (N == 4) {
        for (int my_idx = 0; my_idx < N; ++my_idx) {
            this_fp = fp_in[my_idx];

            int fp_str_len = snprintf(NULL, 0, "%.0f", this_fp);
            char* fp_cstr = (char *)malloc(fp_str_len + 1);
            snprintf(fp_cstr, fp_str_len + 1, "%.0f", this_fp);
            std::string fp_str = std::string(fp_cstr);
            free(fp_cstr);

            std::string fp_str_bin;

            bool is_neg = fp_str[0] == '-'; // remember to invert binary value later

            if (is_neg) {
                fp_str.erase(0, 1);
            }

            if (atoi(fp_str.c_str()) == 0) {
                fp_str_bin.append("0");
            } else {
                while ((fp_str.length() > 0) && atoi(fp_str.c_str()) != 0) {
                    if (charIsOdd(fp_str.back())) {
                        fp_str_bin.insert(0, "1");
                    } else {
                        fp_str_bin.insert(0, "0");
                    }

                    fp_str = stringDivTwo(fp_str);
                }
            }
            while (fp_str_bin.length() < 128) {
                fp_str_bin.insert(0, "0");
            }
            // printf("absolute binary of %d: %s\n", my_idx, fp_str_bin.c_str());
            if (is_neg) {
                bool first_one = false;
                for (int idx = fp_str_bin.length() - 1; idx >= 0; idx = idx - 1) {
                    if (first_one) {
                        if (fp_str_bin[idx] == '1') {
                            fp_str_bin[idx] = '0';
                        } else if (fp_str_bin[idx] == '0') {
                            fp_str_bin[idx] = '1';
                        } else {
                            printf("Unexpected character in binary string %c\n", fp_str_bin[idx]);
                            assert(false);
                        }
                    }
                    if (fp_str_bin[idx] == '1') {
                        first_one = true;
                    }
                }
            }
            // printf("two's complement binary of %d: %s\n", my_idx, fp_str_bin.c_str());
            std::string str_front = fp_str_bin.substr(0, 64);
            std::string str_back = fp_str_bin.substr(64, 64);
            // printf("str_front: %s, str_back: %s\n", str_front.c_str(), str_back.c_str());
            INT64 front_val = strtoll(str_front.c_str(), NULL, 2);
            INT64 back_val = strtoll(str_back.c_str(), NULL, 2);
            // printf("front_val: %zd, back_val: %zd\n", front_val, back_val);
            memcpy(cache_line + 16 * my_idx, &back_val, 8);
            memcpy(cache_line + 16 * my_idx + 8, &front_val, 8);
        }
    } else {
        printf("Unsupported\n");
        assert(false);
    }
}